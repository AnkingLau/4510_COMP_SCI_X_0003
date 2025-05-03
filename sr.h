#ifndef SR_H
#define SR_H

/* 移除对emulator.h的重复包含（已在emulator.c中包含） */
struct msg;  // 前向声明（避免重定义）
struct pkt;  // 前向声明

/* 协议参数定义（移除注释以符合C90） */
#define WINDOW_SIZE 8       /* Window size */
#define MAX_SEQ 256         /* Sequence number range */
#define TIMEOUT 16 0        /* Timeout value */

/* SR专用数据结构 */
typedef struct {
    struct pkt packet;      /* Data packet copy */
    int acked;              /* ACK status */
    double timestamp;       /* Send timestamp */
} SendBufferSlot;

typedef struct {
    struct pkt packet;      /* Received packet */
    int received;           /* Receive status */
} RecvBufferSlot;

/* 函数声明 */
void A_output(struct msg message);
void A_input(struct pkt packet);
void A_timerinterrupt(void);
void A_init(void);
void B_input(struct pkt packet);
void B_init(void);

/* 工具函数 */
int compute_checksum(struct pkt packet);
void send_ack(int calling_entity, int ack_seq);

#endif
