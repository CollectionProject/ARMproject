#ifndef __CAN_H
#define __CAN_H

#include "stm32f4xx.h"

#define CAN_RECEIVE_LENGTH       30    //CAN接受数据队列长度
#define CAN_REC_LEN  			120*1024 //定义最大接收字节数 120K

#define RX_CMD_ID 0x3F
#define RX_DAT_ID 0x3E

#define RX_CMD_TOTAL_START 0x10
#define RX_CMD_TOTAL_END 0x11
#define RX_CMD_BLOCK_START 0x20
#define RX_CMD_BLOCK_END   0x21

#define TX_RSP_ID 0x4F

/* CAN message object structure                                              */
typedef struct
{
   u32 id;                 /* 29 bit identifier                               */
   u8 data[8];            /* Data field                                      */
   u8 len;                /* Length of data field in bytes                   */
	
}  CAN_msg;


void CAN_Config(void);
void CAN1_SendByte(u8 CanID,u8 len,u8 *CanSendByte);

void BSP_CAN_IntEnable(void);

void start_download(void);
void end_download(void);
void start_block_trans(void);
void end_block_trans(u32 offset,u32 lenofbyte);
#endif

