#ifndef SR_H
#define SR_H

/* 基础结构体定义（必须与emulator.h完全一致） */
struct msg { char data[20]; };
struct pkt { int seqnum; int acknum; int checksum; char payload[20]; };

/* 协议参数 */
#define WINDOW_SIZE 8
#define MAX_SEQ 256
#define TIMEOUT 16.0
#define BIDIRECTIONAL 0  /* 明确声明单向传输 */

/* 仿真器函数声明 */
extern int corrupt(struct pkt);
extern void starttimer(int, double);
extern void stoptimer(int);
extern void tolayer3(int, struct pkt);
extern void tolayer5(int, char[20]);
extern double getsimtime(void);

/* 必须实现的函数 */
void A_output(struct msg message);
void A_input(struct pkt packet);
void A_timerinterrupt(void);
void A_init(void);
void B_input(struct pkt packet);
void B_init(void);

/* 测试环境要求的空函数 */
void B_output(struct msg message);
void B_timerinterrupt(void);

#endif
/* 确保最后有空行 */
