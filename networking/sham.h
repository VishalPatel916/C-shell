#ifndef SHAM_H
#define SHAM_H

#include <stdint.h>

#define PAYLOAD_SIZE 1024
#define SLIDING_WIN 10   // Sender's window size in packets
#define TIMEOUT_US 500000 // 500ms retransmission timeout

// SHAM protocol flags
#define SYN 0x01  // Synchronize
#define ACK 0x02  // Acknowledge
#define FIN 0x04  // Finish

struct sham_header {
    uint32_t seq_num;      // Sequence number
    uint32_t ack_num;      // Acknowledgment number
    uint16_t flags;        // Control flags (SYN, ACK, FIN)
    uint16_t window_size;  // Flow control window size (receiver's buffer)
};

#endif