/*
 * sr.h - Selective Repeat Header File
 * Created: 1/April 2025
 * Last Modified: 1/May 2025
 * StudentID：a1890379
 * Description: Function declarations for Selective Repeat sender and receiver
 */

#ifndef SR_H
#define SR_H

/* Forward declarations to avoid type visibility errors */
struct msg;
struct pkt;

/* Sender side (A) */
void A_init(void);
void A_output(struct msg message);
void A_input(struct pkt packet);
void A_timerinterrupt(void);

/* Receiver side (B) */
void B_init(void);
void B_input(struct pkt packet);
void B_output(struct msg message);
void B_timerinterrupt(void);

#endif
