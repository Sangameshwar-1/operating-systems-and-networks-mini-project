#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include "sham.h"

// Global variables
static FILE *log_file = NULL;
static bool logging_enabled = false;
static double loss_rate = 0.0;
static bool chat_mode = false;

// Sliding window
static struct packet_window window[SHAM_WINDOW_SIZE];
static uint32_t window_base = 0;
static uint32_t next_seq_num = 0;
static uint16_t peer_window = 65535;

// Initialize logging
void init_logging(const char *log_filename) {
    char *log_env = getenv("RUDP_LOG");
    if (log_env && strcmp(log_env, "1") == 0) {
        logging_enabled = true;
        log_file = fopen(log_filename, "w");
        if (!log_file) {
            perror("Failed to open log file");
            logging_enabled = false;
        }
    }
}

// Close logging
void close_logging(void) {
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}

// Log event with timestamp
void log_event(const char *format, ...) {
    if (!logging_enabled || !log_file) return;
    
    char time_buffer[30];
    struct timeval tv;
    time_t curtime;
    
    gettimeofday(&tv, NULL);
    curtime = tv.tv_sec;
    
    strftime(time_buffer, 30, "%Y-%m-%d %H:%M:%S", localtime(&curtime));
    fprintf(log_file, "[%s.%06ld] [LOG] ", time_buffer, tv.tv_usec);
    
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    
    fprintf(log_file, "\n");
    fflush(log_file);
}

// Check if timeout occurred
bool is_timeout(struct timeval *sent_time, int timeout_ms) {
    struct timeval now;
    gettimeofday(&now, NULL);
    
    long elapsed_ms = (now.tv_sec - sent_time->tv_sec) * 1000 + 
                      (now.tv_usec - sent_time->tv_usec) / 1000;
    
    return elapsed_ms >= timeout_ms;
}

// Send packet
int send_packet(int sockfd, struct sockaddr_in *dest_addr, struct sham_packet *pkt, uint32_t data_len) {
    size_t total_len = SHAM_HEADER_SIZE + data_len;
    ssize_t sent = sendto(sockfd, pkt, total_len, 0, 
                          (struct sockaddr*)dest_addr, sizeof(*dest_addr));
    if (sent < 0) {
        perror("sendto failed");
        return -1;
    }
    return 0;
}

// Receive packet with timeout
int recv_packet_timeout(int sockfd, struct sham_packet *pkt, struct sockaddr_in *src_addr, 
                       uint32_t *data_len, int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    
    int ret = select(sockfd + 1, &read_fds, NULL, NULL, &tv);
    if (ret <= 0) {
        return 0; // Timeout or error
    }
    
    socklen_t addr_len = sizeof(*src_addr);
    ssize_t recv_len = recvfrom(sockfd, pkt, SHAM_PACKET_SIZE, 0,
                                (struct sockaddr*)src_addr, &addr_len);
    if (recv_len < 0) {
        return -1;
    }
    
    if (recv_len < (ssize_t)SHAM_HEADER_SIZE) {
        return -1;
    }
    
    *data_len = recv_len - SHAM_HEADER_SIZE;
    return 1;
}

// Perform 3-way handshake (client side)
int perform_handshake(int sockfd, struct sockaddr_in *server_addr) {
    struct sham_packet pkt;
    uint32_t data_len;
    
    // Send SYN
    uint32_t initial_seq = 100;
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.seq_num = initial_seq;
    pkt.header.flags = SHAM_SYN;
    pkt.header.window_size = 65535;
    
    log_event("SND SYN SEQ=%u", initial_seq);
    send_packet(sockfd, server_addr, &pkt, 0);
    
    // Wait for SYN-ACK
    if (recv_packet_timeout(sockfd, &pkt, server_addr, &data_len, SHAM_TIMEOUT_MS) <= 0) {
        fprintf(stderr, "Timeout waiting for SYN-ACK\n");
        return -1;
    }
    
    if (!(pkt.header.flags & SHAM_SYN) || !(pkt.header.flags & SHAM_ACK)) {
        fprintf(stderr, "Expected SYN-ACK packet\n");
        return -1;
    }
    
    uint32_t server_seq = pkt.header.seq_num;
    log_event("RCV SYN-ACK SEQ=%u ACK=%u", server_seq, pkt.header.ack_num);
    peer_window = pkt.header.window_size;
    
    // Send ACK
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.seq_num = initial_seq + 1;
    pkt.header.ack_num = server_seq + 1;
    pkt.header.flags = SHAM_ACK;
    pkt.header.window_size = 65535;
    
    log_event("SND ACK SEQ=%u ACK=%u", pkt.header.seq_num, pkt.header.ack_num);
    send_packet(sockfd, server_addr, &pkt, 0);
    
    next_seq_num = initial_seq + 1;
    window_base = initial_seq + 1;
    
    return 0;
}

// Send file with sliding window
int send_file(int sockfd, struct sockaddr_in *server_addr, const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("Failed to open file");
        return -1;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    printf("Sending file: %s (%ld bytes)\n", filename, file_size);
    
    // Initialize window
    memset(window, 0, sizeof(window));
    
    bool done_sending = false;
    uint8_t buffer[SHAM_DATA_SIZE];
    
    while (window_base < next_seq_num + (uint32_t)file_size || !done_sending) {
        // Send new packets within window
        while (!done_sending && (next_seq_num - window_base) < SHAM_WINDOW_SIZE * SHAM_DATA_SIZE) {
            size_t bytes_read = fread(buffer, 1, SHAM_DATA_SIZE, f);
            if (bytes_read == 0) {
                done_sending = true;
                break;
            }
            
            int win_idx = ((next_seq_num - window_base) / SHAM_DATA_SIZE) % SHAM_WINDOW_SIZE;
            
            memset(&window[win_idx].packet, 0, sizeof(struct sham_packet));
            window[win_idx].packet.header.seq_num = next_seq_num;
            window[win_idx].packet.header.flags = 0;
            window[win_idx].packet.header.window_size = 65535;
            memcpy(window[win_idx].packet.data, buffer, bytes_read);
            window[win_idx].data_len = bytes_read;
            window[win_idx].acked = false;
            window[win_idx].retries = 0;
            gettimeofday(&window[win_idx].send_time, NULL);
            
            log_event("SND DATA SEQ=%u LEN=%u", next_seq_num, (uint32_t)bytes_read);
            send_packet(sockfd, server_addr, &window[win_idx].packet, bytes_read);
            
            next_seq_num += bytes_read;
        }
        
        // Wait for ACKs
        struct sham_packet ack_pkt;
        uint32_t ack_data_len;
        int recv_ret = recv_packet_timeout(sockfd, &ack_pkt, server_addr, &ack_data_len, 100);
        
        if (recv_ret > 0 && (ack_pkt.header.flags & SHAM_ACK)) {
            log_event("RCV ACK=%u", ack_pkt.header.ack_num);
            
            // Update window base (cumulative ACK)
            if (ack_pkt.header.ack_num > window_base) {
                window_base = ack_pkt.header.ack_num;
            }
            
            peer_window = ack_pkt.header.window_size;
            log_event("FLOW WIN UPDATE=%u", peer_window);
        }
        
        // Check for timeouts and retransmit
        for (int i = 0; i < SHAM_WINDOW_SIZE; i++) {
            uint32_t pkt_seq = window_base + i * SHAM_DATA_SIZE;
            if (pkt_seq >= next_seq_num) break;
            if (pkt_seq < window_base) continue;
            
            int win_idx = ((pkt_seq - window_base) / SHAM_DATA_SIZE) % SHAM_WINDOW_SIZE;
            
            if (!window[win_idx].acked && is_timeout(&window[win_idx].send_time, SHAM_TIMEOUT_MS)) {
                if (window[win_idx].retries >= SHAM_MAX_RETRIES) {
                    fprintf(stderr, "Max retries exceeded\n");
                    fclose(f);
                    return -1;
                }
                
                log_event("TIMEOUT SEQ=%u", window[win_idx].packet.header.seq_num);
                log_event("RETX DATA SEQ=%u LEN=%u", 
                         window[win_idx].packet.header.seq_num, window[win_idx].data_len);
                
                send_packet(sockfd, server_addr, &window[win_idx].packet, window[win_idx].data_len);
                gettimeofday(&window[win_idx].send_time, NULL);
                window[win_idx].retries++;
            }
        }
        
        // Break if all acknowledged
        if (done_sending && window_base >= next_seq_num) {
            break;
        }
    }
    
    fclose(f);
    printf("File sent successfully\n");
    return 0;
}

// Perform 4-way termination
void perform_termination(int sockfd, struct sockaddr_in *server_addr) {
    struct sham_packet pkt;
    uint32_t data_len;
    
    // Send FIN
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.seq_num = next_seq_num;
    pkt.header.flags = SHAM_FIN;
    pkt.header.window_size = 65535;
    
    log_event("SND FIN SEQ=%u", next_seq_num);
    send_packet(sockfd, server_addr, &pkt, 0);
    
    // Wait for ACK
    recv_packet_timeout(sockfd, &pkt, server_addr, &data_len, SHAM_TIMEOUT_MS);
    log_event("RCV ACK FOR FIN");
    
    // Wait for server's FIN
    recv_packet_timeout(sockfd, &pkt, server_addr, &data_len, SHAM_TIMEOUT_MS);
    log_event("RCV FIN SEQ=%u", pkt.header.seq_num);
    
    // Send final ACK
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.ack_num = pkt.header.seq_num + 1;
    pkt.header.flags = SHAM_ACK;
    pkt.header.window_size = 65535;
    
    log_event("SND ACK=%u", pkt.header.ack_num);
    send_packet(sockfd, server_addr, &pkt, 0);
}

// Handle chat mode
void handle_chat_mode(int sockfd, struct sockaddr_in *server_addr) {
    fd_set read_fds;
    struct sham_packet pkt;
    uint32_t data_len;
    
    printf("Chat mode started. Type /quit to exit.\n");
    
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sockfd, &read_fds);
        
        struct timeval tv = {1, 0};
        int ret = select(sockfd + 1, &read_fds, NULL, NULL, &tv);
        
        if (ret < 0) {
            perror("select");
            break;
        }
        
        // Check stdin
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            char line[SHAM_DATA_SIZE];
            if (fgets(line, sizeof(line), stdin)) {
                if (strncmp(line, "/quit", 5) == 0) {
                    perform_termination(sockfd, server_addr);
                    break;
                }
                
                // Send message
                memset(&pkt, 0, sizeof(pkt));
                pkt.header.seq_num = next_seq_num;
                pkt.header.flags = 0;
                size_t len = strlen(line);
                memcpy(pkt.data, line, len);
                send_packet(sockfd, server_addr, &pkt, len);
                next_seq_num += len;
            }
        }
        
        // Check socket
        if (FD_ISSET(sockfd, &read_fds)) {
            if (recv_packet_timeout(sockfd, &pkt, server_addr, &data_len, 0) > 0) {
                if (pkt.header.flags & SHAM_FIN) {
                    break;
                }
                if (data_len > 0) {
                    printf("Peer: %.*s", (int)data_len, pkt.data);
                }
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port> <input_file> <output_file_name> [loss_rate]\n", argv[0]);
        fprintf(stderr, "   or: %s <server_ip> <server_port> --chat [loss_rate]\n", argv[0]);
        return 1;
    }
    
    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    char *input_file = NULL;
    
    // Parse arguments
    if (argc > 3 && strcmp(argv[3], "--chat") == 0) {
        chat_mode = true;
        if (argc > 4) loss_rate = atof(argv[4]);
    } else if (argc >= 5) {
        input_file = argv[3];
        if (argc > 5) loss_rate = atof(argv[5]);
    } else {
        fprintf(stderr, "Invalid arguments\n");
        return 1;
    }
    
    srand(time(NULL));
    init_logging("client_log.txt");
    
    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    
    // Setup server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP\n");
        close(sockfd);
        return 1;
    }
    
    // Perform handshake
    if (perform_handshake(sockfd, &server_addr) < 0) {
        fprintf(stderr, "Handshake failed\n");
        close(sockfd);
        close_logging();
        return 1;
    }
    
    printf("Connection established\n");
    
    // Send file or enter chat mode
    if (chat_mode) {
        handle_chat_mode(sockfd, &server_addr);
    } else {
        if (send_file(sockfd, &server_addr, input_file) < 0) {
            close(sockfd);
            close_logging();
            return 1;
        }
        perform_termination(sockfd, &server_addr);
    }
    
    close(sockfd);
    close_logging();
    return 0;
}
