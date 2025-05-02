/*
 * sr.c - Selective Repeat Protocol Implementation
 * StudentID:a1820379
 * Description: Selective Repeat protocol implementation adapted from Go-Back-N
 */

 #include <stdlib.h>
 #include <stdio.h>
 #include <stdbool.h>
 #include "emulator.h"
 #include "sr.h"
 
 #define RTT  16.0 // 2025-04-01: Set round trip timeout value
 #define WINDOWSIZE 6 //  Set send/receive window size
 #define SEQSPACE 12  //  Sequence space must be >= 2 * WINDOWSIZE
 #define NOTINUSE (-1)
 
 //  Compute checksum for a packet
 int ComputeChecksum(struct pkt packet) {
     int checksum = packet.seqnum + packet.acknum;
     for (int i = 0; i < 20; i++) {
         checksum += (int)(packet.payload[i]);
     }
     return checksum;
 }
 
 // Check if a packet is corrupted
 bool IsCorrupted(struct pkt packet) {
     return packet.checksum != ComputeChecksum(packet);
 }
 
 // Sender variables
 static struct pkt send_buffer[SEQSPACE];
 static bool acked[SEQSPACE];
 static float timers[SEQSPACE];
 static int base = 0;
 static int nextseqnum = 0;
 
 // Receiver variables
 static struct pkt recv_buffer[SEQSPACE];
 static bool received[SEQSPACE];
 static int expected_base = 0;
 
 // 2025-04-05 Handle message from layer 5 at sender
 void A_output(struct msg message) {
     if ((nextseqnum + SEQSPACE - base) % SEQSPACE < WINDOWSIZE) {
         struct pkt packet;
         packet.seqnum = nextseqnum;
         packet.acknum = NOTINUSE;
         for (int i = 0; i < 20; i++) packet.payload[i] = message.data[i];
         packet.checksum = ComputeChecksum(packet);
 
         send_buffer[nextseqnum] = packet;
         acked[nextseqnum] = false;
 
         tolayer3(A, packet);
         starttimer(A, RTT);
 
         if (TRACE > 0)
             printf("A: Sent packet %d\n", nextseqnum);
 
         nextseqnum = (nextseqnum + 1) % SEQSPACE;
     } else {
         if (TRACE > 0)
             printf("A: Window full, cannot send\n");
     }
 }
 
 // 2025-04-10: Handle incoming ACK at sender
 void A_input(struct pkt packet) {
     if (!IsCorrupted(packet) && acked[packet.acknum] == false) {
         acked[packet.acknum] = true;
 
         if (TRACE > 0)
             printf("A: Received ACK %d\n", packet.acknum);
 
         if (packet.acknum == base) {
             while (acked[base]) {
                 stoptimer(A);
                 base = (base + 1) % SEQSPACE;
             }
             if ((nextseqnum + SEQSPACE - base) % SEQSPACE > 0)
                 starttimer(A, RTT);
         }
     }
 }
 
 // 2025-04-15: Timeout handler at sender
 void A_timerinterrupt(void) {
     if (TRACE > 0)
         printf("A: Timeout occurred, resending all unACKed packets\n");
 
     for (int i = 0; i < WINDOWSIZE; i++) {
         int seq = (base + i) % SEQSPACE;
         if (!acked[seq]) {
             tolayer3(A, send_buffer[seq]);
             if (TRACE > 0)
                 printf("A: Resending packet %d\n", seq);
         }
     }
     starttimer(A, RTT);
 }
 
 // 2025-04-15: Initialize sender state
 void A_init(void) {
     for (int i = 0; i < SEQSPACE; i++) {
         acked[i] = false;
     }
     base = 0;
     nextseqnum = 0;
 }
 
 // 2025-04-12: Handle packet at receiver
 void B_input(struct pkt packet) {
     struct pkt ackpkt;
     if (!IsCorrupted(packet)) {
         if ((packet.seqnum >= expected_base && packet.seqnum < expected_base + WINDOWSIZE) ||
             (expected_base + WINDOWSIZE >= SEQSPACE &&
              (packet.seqnum < (expected_base + WINDOWSIZE) % SEQSPACE))) {
             recv_buffer[packet.seqnum] = packet;
             received[packet.seqnum] = true;
 
             if (TRACE > 0)
                 printf("B: Packet %d received and buffered\n", packet.seqnum);
 
             while (received[expected_base]) {
                 tolayer5(B, recv_buffer[expected_base].payload);
                 received[expected_base] = false;
                 expected_base = (expected_base + 1) % SEQSPACE;
             }
         } else {
             if (TRACE > 0)
                 printf("B: Packet %d out of window, discarded\n", packet.seqnum);
         }
     } else {
         if (TRACE > 0)
             printf("B: Corrupted packet received\n");
     }
 
     ackpkt.seqnum = 0;
     ackpkt.acknum = packet.seqnum;
     for (int i = 0; i < 20; i++) ackpkt.payload[i] = '0';
     ackpkt.checksum = ComputeChecksum(ackpkt);
     tolayer3(B, ackpkt);
 }
 
 // 2025-05-01: Initialize receiver state
 void B_init(void) {
     expected_base = 0;
     for (int i = 0; i < SEQSPACE; i++)
         received[i] = false;
 }
 