#include <stdio.h>
#include <string.h>
#include "emulator.h"  /* 主包含emulator.h */

/* 全局变量移到文件顶部（C90要求） */
SendBufferSlot send_buffer[WINDOW_SIZE];
int base = 0;       /* Window start */
int next_seq = 0;    /* Next sequence number */
RecvBufferSlot recv_buffer[WINDOW_SIZE];
int expected_seq = 0; /* Expected sequence number */

/*-------------------------- 发送方逻辑 --------------------------*/
void A_init() {
    int i;  /* C90需在循环外声明变量 */
    for (i = 0; i < WINDOW_SIZE; i++) {
        send_buffer[i].acked = 1;
    }
    base = next_seq = 0;
}

void A_output(struct msg message) {
    struct pkt packet;
    int idx;

    if (next_seq >= base + WINDOW_SIZE) {
        if (TRACE > 0) printf("SR WARN: Window full!\n");
        return;
    }

    /* 构造数据包 */
    packet.seqnum = next_seq;
    packet.acknum = 0;
    memcpy(packet.payload, message.data, 20);
    packet.checksum = compute_checksum(packet);

    /* 存入缓冲区 */
    idx = next_seq % WINDOW_SIZE;
    send_buffer[idx].packet = packet;
    send_buffer[idx].acked = 0;
    send_buffer[idx].timestamp = getsimtime();

    /* 发送并启动定时器 */
    tolayer3(A, packet);
    if (base == next_seq) {
        starttimer(A, TIMEOUT);
    }
    next_seq++;
}

/*-------------------------- 接收方逻辑 --------------------------*/
void B_input(struct pkt packet) {
    int seq = packet.seqnum;
    int idx;
    struct msg message;

    if (corrupt(packet)) {
        send_ack(B, expected_seq - 1);
        return;
    }

    if (seq >= expected_seq && seq < expected_seq + WINDOW_SIZE) {
        idx = seq % WINDOW_SIZE;
        recv_buffer[idx].packet = packet;
        recv_buffer[idx].received = 1;

        /* 按序提交 */
        while (recv_buffer[expected_seq % WINDOW_SIZE].received) {
            memcpy(message.data, 
                  recv_buffer[expected_seq % WINDOW_SIZE].packet.payload, 
                  20);
            tolayer5(B, message.data);  /* 修正参数类型为char* */
            recv_buffer[expected_seq % WINDOW_SIZE].received = 0;
            expected_seq++;
        }
    }
    send_ack(B, expected_seq - 1);
}

/*-------------------------- 工具函数 --------------------------*/
int compute_checksum(struct pkt packet) {
    int i, checksum = 0;
    for (i = 0; i < 20; i++) {
        checksum += packet.payload[i];
    }
    checksum += packet.seqnum + packet.acknum;
    return ~checksum;
}
