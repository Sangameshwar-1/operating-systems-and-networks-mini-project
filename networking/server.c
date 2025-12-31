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
#include <openssl/md5.h>
#include "sham.h"

// Global variables
static FILE *log_file = NULL;
static bool logging_enabled = false;
static double loss_rate = 0.0;
static bool chat_mode = false;

// Buffer for received data
static uint8_t recv_buffer[10 * 1024 * 1024]; // 10MB buffer
static uint32_t recv_buffer_size = 0;
static uint32_t next_expected_seq = 0;
static uint16_t receiver_window = 65535; // Initial window size

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

// Simulate packet loss
bool should_drop_packet(void) {
    if (loss_rate <= 0.0) return false;
    double rand_val = (double)rand() / RAND_MAX;
    return rand_val < loss_rate;
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

// Receive packet
int recv_packet(int sockfd, struct sham_packet *pkt, struct sockaddr_in *src_addr, uint32_t *data_len) {
    socklen_t addr_len = sizeof(*src_addr);
    ssize_t recv_len = recvfrom(sockfd, pkt, SHAM_PACKET_SIZE, 0,
                                (struct sockaddr*)src_addr, &addr_len);
    if (recv_len < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0; // Timeout
        }
        perror("recvfrom failed");
        return -1;
    }
    
    if (recv_len < (ssize_t)SHAM_HEADER_SIZE) {
        return -1; // Invalid packet
    }
    
    *data_len = recv_len - SHAM_HEADER_SIZE;
    return 1;
}

// Handle 3-way handshake (server side)
int handle_handshake(int sockfd, struct sockaddr_in *client_addr) {
    struct sham_packet pkt;
    uint32_t data_len;
    
    // Wait for SYN (retry on timeout)
    log_event("Waiting for SYN...");
    int ret;
    while ((ret = recv_packet(sockfd, &pkt, client_addr, &data_len)) == 0) {
        // Keep waiting on timeout
        continue;
    }
    if (ret < 0) {
        return -1;
    }
    
    if (!(pkt.header.flags & SHAM_SYN)) {
        fprintf(stderr, "Expected SYN packet\n");
        return -1;
    }
    
    uint32_t client_seq = pkt.header.seq_num;
    log_event("RCV SYN SEQ=%u", client_seq);
    
    // Send SYN-ACK
    uint32_t server_seq = 5000; // Initial server sequence number
    memset(&pkt, 0, sizeof(pkt));
    pkt.header.seq_num = server_seq;
    pkt.header.ack_num = client_seq + 1;
    pkt.header.flags = SHAM_SYN | SHAM_ACK;
    pkt.header.window_size = receiver_window;
    
    log_event("SND SYN-ACK SEQ=%u ACK=%u", server_seq, pkt.header.ack_num);
    send_packet(sockfd, client_addr, &pkt, 0);
    
    // Wait for ACK (retry on timeout)
    while ((ret = recv_packet(sockfd, &pkt, client_addr, &data_len)) == 0) {
        // Keep waiting on timeout
        continue;
    }
    if (ret < 0) {
        return -1;
    }
    
    if (!(pkt.header.flags & SHAM_ACK)) {
        fprintf(stderr, "Expected ACK packet\n");
        return -1;
    }
    
    log_event("RCV ACK FOR SYN");
    
    next_expected_seq = pkt.header.seq_num;
    return 0;
}

// Handle data reception
void handle_data_transfer(int sockfd, struct sockaddr_in *client_addr, const char *output_filename) {
    struct sham_packet pkt;
    uint32_t data_len;
    
    while (1) {
        int ret = recv_packet(sockfd, &pkt, client_addr, &data_len);
        if (ret <= 0) {
            if (ret == 0) continue; // Timeout, keep waiting
            break;
        }
        
        // Check for FIN
        if (pkt.header.flags & SHAM_FIN) {
            log_event("RCV FIN SEQ=%u", pkt.header.seq_num);
            
            // Send ACK for FIN
            struct sham_packet ack_pkt;
            memset(&ack_pkt, 0, sizeof(ack_pkt));
            ack_pkt.header.ack_num = pkt.header.seq_num + 1;
            ack_pkt.header.flags = SHAM_ACK;
            ack_pkt.header.window_size = receiver_window;
            log_event("SND ACK FOR FIN");
            send_packet(sockfd, client_addr, &ack_pkt, 0);
            
            // Send our FIN
            memset(&ack_pkt, 0, sizeof(ack_pkt));
            ack_pkt.header.seq_num = next_expected_seq;
            ack_pkt.header.flags = SHAM_FIN;
            ack_pkt.header.window_size = receiver_window;
            log_event("SND FIN SEQ=%u", next_expected_seq);
            send_packet(sockfd, client_addr, &ack_pkt, 0);
            
            // Wait for final ACK
            recv_packet(sockfd, &pkt, client_addr, &data_len);
            log_event("RCV ACK=%u", pkt.header.ack_num);
            
            break;
        }
        
        // Simulate packet loss
        if (should_drop_packet() && data_len > 0) {
            log_event("DROP DATA SEQ=%u", pkt.header.seq_num);
            continue;
        }
        
        // Handle data packet
        if (data_len > 0) {
            log_event("RCV DATA SEQ=%u LEN=%u", pkt.header.seq_num, data_len);
            
            // Store data if in-order
            if (pkt.header.seq_num == next_expected_seq) {
                memcpy(recv_buffer + recv_buffer_size, pkt.data, data_len);
                recv_buffer_size += data_len;
                next_expected_seq += data_len;
                
                // Send ACK
                struct sham_packet ack_pkt;
                memset(&ack_pkt, 0, sizeof(ack_pkt));
                ack_pkt.header.ack_num = next_expected_seq;
                ack_pkt.header.flags = SHAM_ACK;
                ack_pkt.header.window_size = receiver_window;
                log_event("SND ACK=%u WIN=%u", next_expected_seq, receiver_window);
                send_packet(sockfd, client_addr, &ack_pkt, 0);
            }
        }
    }
    
    // Write received data to file
    if (output_filename && recv_buffer_size > 0) {
        FILE *f = fopen(output_filename, "wb");
        if (f) {
            fwrite(recv_buffer, 1, recv_buffer_size, f);
            fclose(f);
            
            // Calculate and print MD5
            unsigned char md5_hash[MD5_DIGEST_LENGTH];
            MD5(recv_buffer, recv_buffer_size, md5_hash);
            
            printf("MD5: ");
            for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
                printf("%02x", md5_hash[i]);
            }
            printf("\n");
        }
    }
}

// Handle chat mode
void handle_chat_mode(int sockfd, struct sockaddr_in *client_addr) {
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
                    // Send FIN
                    memset(&pkt, 0, sizeof(pkt));
                    pkt.header.seq_num = next_expected_seq;
                    pkt.header.flags = SHAM_FIN;
                    send_packet(sockfd, client_addr, &pkt, 0);
                    break;
                }
                
                // Send message
                memset(&pkt, 0, sizeof(pkt));
                pkt.header.seq_num = next_expected_seq;
                pkt.header.flags = 0;
                size_t len = strlen(line);
                memcpy(pkt.data, line, len);
                send_packet(sockfd, client_addr, &pkt, len);
                next_expected_seq += len;
            }
        }
        
        // Check socket
        if (FD_ISSET(sockfd, &read_fds)) {
            if (recv_packet(sockfd, &pkt, client_addr, &data_len) > 0) {
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
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port> [--chat] [loss_rate]\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    
    // Parse optional arguments
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "--chat") == 0) {
            chat_mode = true;
        } else {
            loss_rate = atof(argv[i]);
        }
    }
    
    srand(time(NULL));
    init_logging("server_log.txt");
    
    // Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }
    
    // Set socket timeout
    struct timeval tv = {2, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Bind socket
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        return 1;
    }
    
    printf("Server listening on port %d\n", port);
    
    // Handle connection
    struct sockaddr_in client_addr;
    if (handle_handshake(sockfd, &client_addr) < 0) {
        fprintf(stderr, "Handshake failed\n");
        close(sockfd);
        close_logging();
        return 1;
    }
    
    printf("Connection established\n");
    
    // Handle data transfer or chat
    if (chat_mode) {
        handle_chat_mode(sockfd, &client_addr);
    } else {
        handle_data_transfer(sockfd, &client_addr, "received_file");
    }
    
    close(sockfd);
    close_logging();
    return 0;
}
