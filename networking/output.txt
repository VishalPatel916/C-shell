#ifndef SHAM_H
#define SHAM_H

#include <stdint.h>

#define PAYLOAD_SIZE 1024
#define SLIDING_WIN 5
#define TIMEOUT_SEC 2
#define MAX_RETRIES 3

// SHAM protocol flags
#define SYN 0x01  // Synchronize
#define ACK 0x02  // Acknowledge
#define FIN 0x04  // Finish
#define RST 0x08  // Reset

struct sham_header {
    uint32_t seq_num;      // Sequence number
    uint32_t ack_num;      // Acknowledgment number
    uint8_t flags;         // Control flags (SYN, ACK, FIN, RST)
    uint16_t window_size;  // Advertised window size
    uint16_t checksum;     // Checksum for error detection
};

// Function declarations
uint16_t calculate_checksum(void *data, size_t length);
void log_message(FILE *log_file, const char *message_type, const char *format, ...);

#endif