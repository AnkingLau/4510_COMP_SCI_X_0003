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

#define RTT  16.0       /* round trip time.  MUST BE SET TO 16.0 when submitting assignment */
#define WINDOWSIZE 6    /* the maximum number of buffered unacked packet
                          MUST BE SET TO 6 when submitting assignment */
#define SEQSPACE 12     /* sequence space must be >= 2*WINDOWSIZE for SR */
#define NOTINUSE (-1)   /* used to fill header fields that are not being used */

/* generic procedure to compute the checksum of a packet.  Used by both sender and receiver */
int ComputeChecksum(struct pkt packet)
{
  int checksum = 0;
  int i;
  checksum = packet.seqnum;
  checksum += packet.acknum;
  for ( i=0; i<20; i++ )
    checksum += (int)(packet.payload[i]);
  return checksum;
}

bool IsCorrupted(struct pkt packet)
{
  return packet.checksum != ComputeChecksum(packet);
}

/********* Sender (A) variables and functions ************/

static struct pkt A_buffer[SEQSPACE];     /* buffer for storing unacked packets */
static bool A_acknowledged[SEQSPACE];     /* tracking ack status of each seqnum */
static float A_timer[SEQSPACE];           /* simulated per-packet timers */
static int A_base;                        /* base of the sliding window */
static int A_nextseqnum;                  /* next sequence number to send */

void A_output(struct msg message)
{
  if ((A_nextseqnum + SEQSPACE - A_base) % SEQSPACE < WINDOWSIZE) {
    struct pkt sendpkt;
    int i;
    sendpkt.seqnum = A_nextseqnum;
    sendpkt.acknum = NOTINUSE;
    for ( i=0; i<20 ; i++ )
      sendpkt.payload[i] = message.data[i];
    sendpkt.checksum = ComputeChecksum(sendpkt);

    A_buffer[A_nextseqnum] = sendpkt;
    A_acknowledged[A_nextseqnum] = false;
    A_timer[A_nextseqnum] = get_sim_time() + RTT;

    tolayer3(A, sendpkt);
    starttimer(A, RTT);

    A_nextseqnum = (A_nextseqnum + 1) % SEQSPACE;
  } else {
    if (TRACE > 0)
      printf("----A: New message arrives, send window is full\n");
  }
}

void A_input(struct pkt packet)
{
  if (!IsCorrupted(packet)) {
    if (TRACE > 0)
      printf("----A: uncorrupted ACK %d is received\n", packet.acknum);
    if (!A_acknowledged[packet.acknum]) {
      A_acknowledged[packet.acknum] = true;
    }
    while (A_acknowledged[A_base]) {
      A_base = (A_base + 1) % SEQSPACE;
    }
  } else {
    if (TRACE > 0)
      printf("----A: corrupted ACK is received, do nothing!\n");
  }
}

void A_timerinterrupt(void)
{
  stoptimer(A);
  for (int i = 0; i < SEQSPACE; i++) {
    if (!A_acknowledged[i] && A_timer[i] <= get_sim_time()) {
      if (TRACE > 0)
        printf("---A: Resending packet %d due to timeout\n", A_buffer[i].seqnum);
      tolayer3(A, A_buffer[i]);
      A_timer[i] = get_sim_time() + RTT;
      starttimer(A, RTT);
    }
  }
}

void A_init(void)
{
  A_base = 0;
  A_nextseqnum = 0;
  for (int i = 0; i < SEQSPACE; i++) {
    A_acknowledged[i] = false;
    A_timer[i] = 0.0;
  }
}

/********* Receiver (B)  variables and procedures ************/

static int B_expectedseqnum;                 /* next sequence number expected */
static struct pkt B_buffer[SEQSPACE];        /* buffer for out-of-order packets */
static bool B_received[SEQSPACE];            /* tracking received status */

void B_input(struct pkt packet)
{
  struct pkt ackpkt;
  int i;

  if (!IsCorrupted(packet)) {
    if (TRACE > 0)
      printf("----B: Received packet %d\n", packet.seqnum);

    if (!B_received[packet.seqnum]) {
      B_received[packet.seqnum] = true;
      B_buffer[packet.seqnum] = packet;

      /* Send ACK */
      ackpkt.acknum = packet.seqnum;
      ackpkt.seqnum = 0;
      for (i = 0; i < 20; i++) ackpkt.payload[i] = '0';
      ackpkt.checksum = ComputeChecksum(ackpkt);
      tolayer3(B, ackpkt);

      while (B_received[B_expectedseqnum]) {
        tolayer5(B, B_buffer[B_expectedseqnum].payload);
        B_received[B_expectedseqnum] = false;
        B_expectedseqnum = (B_expectedseqnum + 1) % SEQSPACE;
      }
    } else {
      /* Duplicate packet, resend ACK */
      ackpkt.acknum = packet.seqnum;
      ackpkt.seqnum = 0;
      for (i = 0; i < 20; i++) ackpkt.payload[i] = '0';
      ackpkt.checksum = ComputeChecksum(ackpkt);
      tolayer3(B, ackpkt);
    }
  } else {
    if (TRACE > 0)
      printf("----B: Corrupted packet received. Ignored.\n");
  }
}

void B_init(void)
{
  B_expectedseqnum = 0;
  for (int i = 0; i < SEQSPACE; i++) {
    B_received[i] = false;
  }
}

void B_output(struct msg message) {}
void B_timerinterrupt(void) {}
