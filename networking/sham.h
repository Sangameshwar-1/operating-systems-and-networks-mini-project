#ifndef SHAM_H
#define SHAM_H

#include <stdint.h>
#include <stdbool.h>

// S.H.A.M. Protocol Flags
#define SHAM_SYN  0x1  // Synchronize - initiate connection
#define SHAM_ACK  0x2  // Acknowledge
#define SHAM_FIN  0x4  // Finish - terminate connection

// Protocol Constants
#define SHAM_DATA_SIZE 1024        // Maximum data payload per packet
#define SHAM_WINDOW_SIZE 10        // Sliding window size (packets)
#define SHAM_TIMEOUT_MS 500        // Retransmission timeout (milliseconds)
#define SHAM_MAX_RETRIES 10        // Maximum retransmission attempts
#define SHAM_HEADER_SIZE sizeof(struct sham_header)   
#define SHAM_PACKET_SIZE (SHAM_HEADER_SIZE + SHAM_DATA_SIZE)  

// S.H.A.M. Header Structure
struct sham_header {
    uint32_t seq_num;      // Sequence number (byte-based)
    uint32_t ack_num;      // Acknowledgment number (next expected byte)
    uint16_t flags;        // Control flags (SYN, ACK, FIN)
    uint16_t window_size;  // Flow control window size (bytes)
} __attribute__((packed));

// S.H.A.M. Packet Structure
struct sham_packet {
    struct sham_header header;
    uint8_t data[SHAM_DATA_SIZE];
} __attribute__((packed));

// Packet Window Entry (for tracking in-flight packets)
struct packet_window {
    struct sham_packet packet;
    uint32_t data_len;
    struct timeval send_time;
    int retries;
    bool acked;
};

// Connection State
typedef enum {
    STATE_CLOSED,
    STATE_SYN_SENT,
    STATE_SYN_RECEIVED,
    STATE_ESTABLISHED,
    STATE_FIN_WAIT_1,
    STATE_FIN_WAIT_2,
    STATE_CLOSE_WAIT,
    STATE_LAST_ACK,
    STATE_TIME_WAIT,
    STATE_CLOSING
} connection_state_t;

// Function prototypes for logging
void init_logging(const char *log_filename);
void close_logging(void);
void log_event(const char *format, ...);

// Utility functions
uint32_t get_current_time_ms(void);
void set_timeout(struct timeval *tv, int timeout_ms);
bool is_timeout(struct timeval *sent_time, int timeout_ms);

#endif // SHAM_H
