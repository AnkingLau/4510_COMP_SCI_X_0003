/*
 * sr.c - Selective Repeat Protocol Implementation
 * Created: 2025-04-01
 * Last Modified: 2025-05-02
 * StudentID:a1820379
 * Description: Selective Repeat protocol implementation adapted from Go-Back-N
 */

#include <stdio.h>
#include <string.h>
#include "sr.h"

/* ******************************************************************
   Go Back N protocol.  Adapted from J.F.Kurose
   ALTERNATING BIT AND GO-BACK-N NETWORK EMULATOR: VERSION 1.2

   Network properties:
   - one way network delay averages five time units (longer if there
   are other messages in the channel for GBN), but can be larger
   - packets can be corrupted (either the header or the data portion)
   or lost, according to user-defined probabilities
   - packets will be delivered in the order in which they were sent
   (although some can be lost).

   Modifications:
   - removed bidirectional GBN code and other code not used by prac.
   - fixed C style to adhere to current programming style
   - added GBN implementation
**********************************************************************/

/********************** Sender (A) **********************/
SendBufferSlot send_buffer[WINDOW_SIZE];
int base = 0;       // Window starting serial number
int next_seq = 0;   // Next to be sent serial number

/********************** Receiver (B)  **********************/
RecvBufferSlot recv_buffer[WINDOW_SIZE];
int expected_seq = 0;  // Expected received serial number

/*--------------------------------------------------------------------*/
/*---------------------------- Sender (A) ----------------------------*/
/*--------------------------------------------------------------------*/

void A_init() {
    // Initialize the sending window
    for (int i = 0; i < WINDOW_SIZE; i++) {
        send_buffer[i].acked = 1;  // Initial marked as' confirmed '
    }
    base = next_seq = 0;
}

void A_output(struct msg message) {
    // Check if the window is full
    if (next_seq >= base + WINDOW_SIZE) {
        if (TRACE > 0) printf("SR WARN: Window full!\n");
        return;
    }

    // Construct data packets (retaining the checksum logic of the original GBN)
    struct pkt packet;
    packet.seqnum = next_seq;
    packet.acknum = 0;  // Unidirectional transmission, ACKnum not used
    memcpy(packet.payload, message.data, 20);
    packet.checksum = compute_checksum(packet);

    // Store in the send buffer
    int idx = next_seq % WINDOW_SIZE;
    send_buffer[idx].packet = packet;
    send_buffer[idx].acked = 0;
    send_buffer[idx].timestamp = getsimtime();

    // Send packets
    tolayer3(A, packet);
    if (TRACE > 1) printf("SR SEND: seq=%d, base=%d\n", next_seq, base);

    // If it is the first package in the window, start the timer
    if (base == next_seq) {
        starttimer(A, TIMEOUT);
    }
    next_seq++;
}

void A_input(struct pkt packet) {
    // Verify data packets
    if (packet.checksum != compute_checksum(packet)) {
        if (TRACE > 0) printf("SR WARN: Corrupted ACK!\n");
        return;
    }

    // Process ACK (Selective Acknowledgment)
    int ack_seq = packet.acknum;
    if (ack_seq >= base && ack_seq < next_seq) {
        send_buffer[ack_seq % WINDOW_SIZE].acked = 1;
        if (TRACE > 1) printf("SR ACKED: seq=%d\n", ack_seq);
    }

    // Slide the window to the first unconfirmed package
    while (base < next_seq && send_buffer[base % WINDOW_SIZE].acked) {
        base++;
    }

    // Update timer status
    if (base == next_seq) {
        stoptimer(A);
    } else {
        starttimer(A, TIMEOUT);  // restart timer
    }
}

void A_timerinterrupt(void) {
    // Timeout retransmission of all unconfirmed packets
    starttimer(A, TIMEOUT);  // must be restart timer
    for (int i = base; i < next_seq; i++) {
        if (!send_buffer[i % WINDOW_SIZE].acked) {
            tolayer3(A, send_buffer[i % WINDOW_SIZE].packet);
            if (TRACE > 0) printf("SR RETRANS: seq=%d\n", i);
        }
    }
}

/*--------------------------------------------------------------------*/
/*---------------------------- Receiver (B) ----------------------------*/
/*--------------------------------------------------------------------*/

void B_init() {
    // Initialize the receiving window
    for (int i = 0; i < WINDOW_SIZE; i++) {
        recv_buffer[i].received = 0;
    }
    expected_seq = 0;
}

void B_input(struct pkt packet) {
    // Verify data packets
    if (packet.checksum != compute_checksum(packet)) {
        if (TRACE > 0) printf("SR WARN: Corrupted packet!\n");
        send_ack(B, expected_seq - 1);  // Send the most recent valid ACK
        return;
    }

    int seq = packet.seqnum;
    // Check if the serial number is within the receiving window
    if (seq >= expected_seq && seq < expected_seq + WINDOW_SIZE) {
        int idx = seq % WINDOW_SIZE;
        recv_buffer[idx].packet = packet;
        recv_buffer[idx].received = 1;
        if (TRACE > 1) printf("SR BUFFERED: seq=%d\n", seq);

        // Attempt to submit in sequence to the application layer
        while (recv_buffer[expected_seq % WINDOW_SIZE].received) {
            struct msg message;
            memcpy(message.data, 
                  recv_buffer[expected_seq % WINDOW_SIZE].packet.payload, 
                  20);
            tolayer5(B, message);
            recv_buffer[expected_seq % WINDOW_SIZE].received = 0;
            if (TRACE > 1) printf("SR DELIVER: seq=%d\n", expected_seq);
            expected_seq++;
        }
    }
    // Always send the latest cumulative ACK
    send_ack(B, expected_seq - 1);
}

/*--------------------------------------------------------------------*/
/*---------------------------- test tool ------------------------------*/
/*--------------------------------------------------------------------*/

int compute_checksum(struct pkt packet) {
    // Maintain the complement and algorithm of 1 in the original GBN
    int checksum = 0;
    for (int i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    checksum += packet.seqnum + packet.acknum;
    return ~checksum;  // negation
}

void send_ack(int calling_entity, int ack_seq) {
    struct pkt ack_pkt;
    ack_pkt.seqnum = 0;      // ACK packe not uses seqnum
    ack_pkt.acknum = ack_seq;
    memset(ack_pkt.payload, 0, 20);
    ack_pkt.checksum = compute_checksum(ack_pkt);
    tolayer3(calling_entity, ack_pkt);
    if (TRACE > 2) printf("SR SEND_ACK: ack=%d\n", ack_seq);
}
