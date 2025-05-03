#include <stdio.h>
#include <string.h>
#include "sr.h"

/*--------------------- 全局变量（C90规范） ---------------------*/
static struct {
    struct pkt packet;
    int acked;
    double timestamp;
} send_buffer[WINDOW_SIZE];

static struct {
    struct pkt packet;
    int received;
} recv_buffer[WINDOW_SIZE];

static int base = 0;
static int next_seq = 0;
static int expected_seq = 0;

/*--------------------- 工具函数 ---------------------*/
int compute_checksum(struct pkt packet) {
    int checksum = 0;
    int i;  /* C90要求循环变量外置 */
    for (i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    checksum += packet.seqnum + packet.acknum;
    return ~checksum;
}

void send_ack(int calling_entity, int ack_seq) {
    struct pkt ack_pkt;
    ack_pkt.acknum = ack_seq;
    ack_pkt.seqnum = 0;
    memset(ack_pkt.payload, 0, 20);
    ack_pkt.checksum = compute_checksum(ack_pkt);
    tolayer3(calling_entity, ack_pkt);
}

/*--------------------- 发送方逻辑 ---------------------*/
void A_init(void) {
    int i;
    for (i = 0; i < WINDOW_SIZE; i++) {
        send_buffer[i].acked = 1;
    }
}

void A_output(struct msg message) {
    struct pkt packet;
    int idx = next_seq % WINDOW_SIZE;  /* 变量集中声明 */

    if (next_seq >= base + WINDOW_SIZE) return;

    packet.seqnum = next_seq;
    memcpy(packet.payload, message.data, 20);
    packet.checksum = compute_checksum(packet);

    send_buffer[idx].packet = packet;
    send_buffer[idx].acked = 0;
    send_buffer[idx].timestamp = getsimtime();

    tolayer3(A, packet);
    if (base == next_seq) starttimer(A, TIMEOUT);
    next_seq++;
}

/*--------------------- 接收方逻辑 ---------------------*/
void B_input(struct pkt packet) {
    int seq = packet.seqnum;
    char message_data[20];  /* 适配tolayer5参数类型 */
    int idx = seq % WINDOW_SIZE;

    if (corrupt(packet)) {
        send_ack(B, expected_seq - 1);
        return;
    }

    if (seq >= expected_seq && seq < expected_seq + WINDOW_SIZE) {
        recv_buffer[idx].packet = packet;
        recv_buffer[idx].received = 1;

        while (recv_buffer[expected_seq % WINDOW_SIZE].received) {
            memcpy(message_data, 
                  recv_buffer[expected_seq % WINDOW_SIZE].packet.payload, 
                  20);
            tolayer5(B, message_data);
            recv_buffer[expected_seq % WINDOW_SIZE].received = 0;
            expected_seq++;
        }
    }
    send_ack(B, expected_seq - 1);
}

/*--------------------- 空函数实现 ---------------------*/
void B_output(struct msg message) {
    /* 单向传输无需实现 */
}

void B_timerinterrupt(void) {
    /* 单向传输无需实现 */
}

/* 确保文件末尾有空行 */
