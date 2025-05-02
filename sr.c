/*
 * sr.c - Selective Repeat Protocol Implementation
 * Created: 2025-04-01
 * Last Modified: 2025-05-02
 * StudentID:a1820379
 * Description: Selective Repeat protocol implementation adapted from Go-Back-N
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "emulator.h"
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

#define RTT  16.0  /* Round Trip Time */
#define WINDOWSIZE 6  /* Max number of unacked packets in window */
#define SEQSPACE 12    /* Sequence number space (must be > WINDOWSIZE) */
#define NOTINUSE (-1)  /* Placeholder for unused acknum field */

/* Compute checksum for a packet using its fields and payload */
int ComputeChecksum(struct pkt packet) {
  int checksum = 0;
  int i;
  for (i = 0; i < 20; i++)
    checksum += (int)(packet.payload[i]);
  checksum += packet.seqnum + packet.acknum;
  return checksum;
}

/* Returns true if the packet is corrupted */
bool IsCorrupted(struct pkt packet) {
  return packet.checksum != ComputeChecksum(packet);
}

/********* Sender (A) *********/

static struct pkt A_window[WINDOWSIZE];  /* Window buffer */
static bool A_ack_received[SEQSPACE];    /* ACK tracking */
static int A_base = 0;                   /* Base of sliding window */
static int A_nextseq = 0;                /* Next sequence number to use */

/* Called when layer 5 passes a message to be sent to B */
void A_output(struct msg message) {
  if ((A_nextseq + SEQSPACE - A_base) % SEQSPACE < WINDOWSIZE) {
    struct pkt packet;
    int i;
    for (i = 0; i < 20; i++)
      packet.payload[i] = message.data[i];
    packet.seqnum = A_nextseq;
    packet.acknum = NOTINUSE;
    packet.checksum = ComputeChecksum(packet);

    A_window[A_nextseq % WINDOWSIZE] = packet;
    A_ack_received[A_nextseq] = false;

    tolayer3(A, packet);
    starttimer(A, RTT);

    if (TRACE > 0)
      printf("A_output: Sent packet %d\n", A_nextseq);
    A_nextseq = (A_nextseq + 1) % SEQSPACE;
  } else {
    if (TRACE > 0)
      printf("A_output: Window full, message dropped\n");
  }
}

/* Called when ACK arrives at sender */
void A_input(struct pkt packet) {
  if (!IsCorrupted(packet)) {
    if (!A_ack_received[packet.acknum]) {
      A_ack_received[packet.acknum] = true;
      if (TRACE > 0)
        printf("A_input: ACK %d received\n", packet.acknum);

      if (packet.acknum == A_base) {
        while (A_ack_received[A_base]) {
          stoptimer(A);
          A_base = (A_base + 1) % SEQSPACE;
        }
        if (A_base != A_nextseq)
          starttimer(A, RTT);
      }
    }
  } else {
    if (TRACE > 0)
      printf("A_input: Corrupted ACK received\n");
  }
}

/* Called when sender timer expires */
void A_timerinterrupt(void) {
  int i;
  if (TRACE > 0)
    printf("A_timerinterrupt: Timer expired, checking for retransmissions\n");

  for (i = 0; i < WINDOWSIZE; i++) {
    int seq = (A_base + i) % SEQSPACE;
    if (!A_ack_received[seq]) {
      tolayer3(A, A_window[seq % WINDOWSIZE]);
      if (TRACE > 0)
        printf("A_timerinterrupt: Resent packet %d\n", seq);
    }
  }
  starttimer(A, RTT);
}

/* Initializes sender state */
void A_init(void) {
  int i;
  for (i = 0; i < SEQSPACE; i++)
    A_ack_received[i] = false;
  A_base = 0;
  A_nextseq = 0;
}

/********* Receiver (B) *********/

static struct pkt B_buffer[SEQSPACE];    /* Packet buffer */
static bool B_received[SEQSPACE];        /* Track received seq nums */
static int B_expected = 0;               /* Lowest in-order seqnum expected */

/* Called when a packet arrives at receiver */
void B_input(struct pkt packet) {
  struct pkt ackpkt;
  int i;

  if (!IsCorrupted(packet)) {
    if (!B_received[packet.seqnum]) {
      B_received[packet.seqnum] = true;
      B_buffer[packet.seqnum] = packet;

      if (TRACE > 0)
        printf("B_input: Received new packet %d\n", packet.seqnum);

      while (B_received[B_expected]) {
        tolayer5(B, B_buffer[B_expected].payload);
        B_received[B_expected] = false;
        B_expected = (B_expected + 1) % SEQSPACE;
      }
    } else {
      if (TRACE > 0)
        printf("B_input: Duplicate packet %d received\n", packet.seqnum);
    }
  } else {
    if (TRACE > 0)
      printf("B_input: Corrupted packet received\n");
  }

  ackpkt.seqnum = 0;
  ackpkt.acknum = packet.seqnum;
  for (i = 0; i < 20; i++)
    ackpkt.payload[i] = '0';
  ackpkt.checksum = ComputeChecksum(ackpkt);
  tolayer3(B, ackpkt);
  if (TRACE > 0)
    printf("B_input: Sent ACK %d\n", packet.seqnum);
}

/* Initializes receiver state */
void B_init(void) {
  int i;
  for (i = 0; i < SEQSPACE; i++)
    B_received[i] = false;
  B_expected = 0;
}

