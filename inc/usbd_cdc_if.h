#ifndef __USBD_CDC_IF_H__
#define __USBD_CDC_IF_H__

#include "usbd_cdc.h"

// 缓冲区设置
#define TX_BUF_SIZE  64 // 线性TX缓冲区大小
#define NUM_RX_BUFS 6 // FIFO中RX缓冲区的数量
#define RX_BUF_SIZE CDC_DATA_FS_MAX_PACKET_SIZE // RX缓冲区项的大小

// 接收缓冲：循环缓冲区 FIFO
typedef struct _usbrx_buf_
{
	// 接收缓冲：循环缓冲 FIFO
	uint8_t buf[NUM_RX_BUFS][RX_BUF_SIZE]; // 接收缓冲区
	uint32_t msglen[NUM_RX_BUFS];          // 各缓冲区消息长度
	uint8_t head;                          // 头指针，指向下一个可写空间
	uint8_t tail;                          // 尾指针，指向下一个可读空间

} usbrx_buf_t;  // USB接收缓冲区类型定义


// CDC Interface callback.
extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;


// Prototypes
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
void cdc_process(void);



#endif /* __USBD_CDC_IF_H__ */

