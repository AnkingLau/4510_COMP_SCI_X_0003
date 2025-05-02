/*
 * sr.h - Selective Repeat Header File
 * Created: 1/April 2025
 * Last Modified: 1/May 2025
 * StudentID：a1890379
 * Description: Function declarations for Selective Repeat sender and receiver
 */

 #ifndef SR_H
 #define SR_H
 
 #include "emulator.h"
 
 // Function declarations for Selective Repeat sender and receiver
 
 // Sender side (A)
 void A_init(void);        // 2025-05-01: Initialize sender variables
 void A_output(struct msg message); //  Send packet if window allows
 void A_input(struct pkt packet);   //  Handle received ACK
 void A_timerinterrupt(void);       //  Handle timeout and retransmit
 
 // Receiver side (B)
 void B_init(void);        //  Initialize receiver variables
 void B_input(struct pkt packet);   // Handle received packet and send ACK
 
 #endif // SR_H
 