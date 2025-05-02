/*
 * sr.h - Selective Repeat Header File
 * Created: 1/April 2025
 * Last Modified: 2/May 2025
 * StudentID：a1820379
 * Description: Function declarations for Selective Repeat sender and receiver
 */

 #ifndef SR_H
 #define SR_H
 
 #include "emulator.h"  // Must include emulator.h files
 
 /* Protocol parameter definition */
 #define WINDOW_SIZE 8       // Window size
 #define MAX_SEQ 256         // Range of serial numbers（2^8）
 #define TIMEOUT 16.0        // time out
 
 /* add SR data struct */
 typedef struct {
     struct pkt packet;      // data packets
     int acked;              // confirmed（1=Confirmed）
     double timestamp;       // Send timestamp (for timeout determination)
 } SendBufferSlot;
 
 typedef struct {
     struct pkt packet;      // Receive cache packets
     int received;           // Received status (1=received)
 } RecvBufferSlot;
 
 /* Function declaration (keeping the original GBN interface unchanged) */
 void A_output(struct msg message);
 void A_input(struct pkt packet);
 void A_timerinterrupt(void);
 void A_init(void);
 void B_input(struct pkt packet);
 void B_init(void);
 
 /* Add new tool functions */
 int compute_checksum(struct pkt packet);  // Keep the original checksum calculation
 void send_ack(int calling_entity, int ack_seq);  // Encapsulate ACK sending
 
 #endif

