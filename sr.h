/*
 * sr.h - Selective Repeat Header File
 * Created: 1/April 2025
 * Last Modified: 2/May 2025
 * StudentID：a1820379
 * Description: Function declarations for Selective Repeat sender and receiver
 */

#ifndef SR_H
#define SR_H


/* Initializes sender-side state variables (called once before simulation starts) */
extern void A_init(void);
extern void B_init(void);
extern void A_input(struct pkt);
extern void B_input(struct pkt);
extern void A_output(struct msg);
extern void A_timerinterrupt(void);

/* Included for future extension to bidirectional communication */
#define BIDIRECTIONAL 0       // 0 = A to B only, 1 = bidirectional
extern void B_output(struct msg);         // Not used in unidirectional mode
extern void B_timerinterrupt(void);       // Not used in unidirectional mode

#endif /* SR_H */

