/*
 * sr.c - Selective Repeat Protocol Implementation
 * Created: 2025-04-01
 * Last Modified: 2025-05-02
 * StudentID:a1820379
 * Description: Selective Repeat protocol implementation adapted from Go-Back-N
 */

#include "emulator.h"
#include "sr.h"

#define RTT  16.0             /* Round Trip Time */
#define WINDOWSIZE 6          /* Sliding window size */
#define SEQSPACE 12           /* Sequence number space */
#define NOTINUSE (-1)         /* Unused field marker */

/* Checksum function used to verify data integrity */
int ComputeChecksum(struct pkt packet) {
    int checksum = packet.seqnum + packet.acknum;
    int i;
    for (i = 0; i < 20; i++) {
        checksum += (int)(packet.payload[i]);
    }
    return checksum;
}

/* Protocol constants */
int A = 0;
int B = 1;
int TRACE = 2;

/* Determines if a packet was corrupted */
bool IsCorrupted(struct pkt packet) {
    return packet.checksum != ComputeChecksum(packet);
}

/* Sender side buffers and counters */
static struct pkt send_buffer[SEQSPACE];
static bool acked[SEQSPACE];
static int base = 0;
static int nextseqnum = 0;

/* Receiver side buffers and tracking */
static struct pkt recv_buffer[SEQSPACE];
static bool received[SEQSPACE];
static int expected_base = 0;

/* Handles outbound messages from layer 5 */
void A_output(struct msg message) {
    if ((nextseqnum + SEQSPACE - base) % SEQSPACE < WINDOWSIZE) {
        struct pkt packet;
        int i;
        packet.seqnum = nextseqnum;
        packet.acknum = NOTINUSE;
        for (i = 0; i < 20; i++) {
            packet.payload[i] = message.data[i];
        }
        packet.checksum = ComputeChecksum(packet);

        send_buffer[nextseqnum] = packet;
        acked[nextseqnum] = false;

        tolayer3(A, packet);
        starttimer(A, RTT);

        if (TRACE > 0) {
            printf("A: Sent packet %d\n", nextseqnum);
        }

        nextseqnum = (nextseqnum + 1) % SEQSPACE;
    } else {
        if (TRACE > 0) {
            printf("A: Window full, cannot send\n");
        }
    }
}

/* Handles incoming ACKs at sender */
void A_input(struct pkt packet) {
    if (!IsCorrupted(packet) && acked[packet.acknum] == false) {
        acked[packet.acknum] = true;

        if (TRACE > 0) {
            printf("A: Received ACK %d\n", packet.acknum);
        }

        if (packet.acknum == base) {
            while (acked[base]) {
                stoptimer(A);
                base = (base + 1) % SEQSPACE;
            }
            if ((nextseqnum + SEQSPACE - base) % SEQSPACE > 0) {
                starttimer(A, RTT);
            }
        }
    }
}

/* Timeout handler: retransmit all unACKed packets */
void A_timerinterrupt(void) {
    int i;
    if (TRACE > 0) {
        printf("A: Timeout occurred, resending all unACKed packets\n");
    }

    for (i = 0; i < WINDOWSIZE; i++) {
        int seq = (base + i) % SEQSPACE;
        if (!acked[seq]) {
            tolayer3(A, send_buffer[seq]);
            if (TRACE > 0) {
                printf("A: Resending packet %d\n", seq);
            }
        }
    }
    starttimer(A, RTT);
}

/* Initializes sender state */
void A_init(void) {
    int i;
    for (i = 0; i < SEQSPACE; i++) {
        acked[i] = false;
    }
    base = 0;
    nextseqnum = 0;
}

/* Processes incoming packets at receiver */
void B_input(struct pkt packet) {
    struct pkt ackpkt;
    int i;
    if (!IsCorrupted(packet)) {
        if ((packet.seqnum >= expected_base && packet.seqnum < expected_base + WINDOWSIZE) ||
            (expected_base + WINDOWSIZE >= SEQSPACE &&
             (packet.seqnum < (expected_base + WINDOWSIZE) % SEQSPACE))) {
            recv_buffer[packet.seqnum] = packet;
            received[packet.seqnum] = true;

            if (TRACE > 0) {
                printf("B: Packet %d received and buffered\n", packet.seqnum);
            }

            while (received[expected_base]) {
                tolayer5(B, recv_buffer[expected_base].payload);
                received[expected_base] = false;
                expected_base = (expected_base + 1) % SEQSPACE;
            }
        } else {
            if (TRACE > 0) {
                printf("B: Packet %d out of window, discarded\n", packet.seqnum);
            }
        }
    } else {
        if (TRACE > 0) {
            printf("B: Corrupted packet received\n");
        }
    }

    ackpkt.seqnum = 0;
    ackpkt.acknum = packet.seqnum;
    for (i = 0; i < 20; i++) {
        ackpkt.payload[i] = '0';
    }
    ackpkt.checksum = ComputeChecksum(ackpkt);
    tolayer3(B, ackpkt);
}

/* Initializes receiver state */
void B_init(void) {
    int i;
    expected_base = 0;
    for (i = 0; i < SEQSPACE; i++) {
        received[i] = false;
    }
}

/* Empty B_output for compatibility with simulator */
void B_output(struct msg message) {
    /* Not used in simplex simulation */
}

/* Empty B_timerinterrupt for compatibility */
void B_timerinterrupt(void) {
    /* Not used in simplex simulation */
}

 
