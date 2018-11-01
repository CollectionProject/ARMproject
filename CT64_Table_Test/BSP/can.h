#ifndef __CAN_H
#define __CAN_H

#include "CAN_Cfg.h"
#include "stm32f4xx.h"

#define CAN_RECEIVE_LENGTH       30    //CAN接受数据队列长度

/* CAN message object structure                                              */
typedef struct
{
   u32 id;                 /* 29 bit identifier                               */
   u8 data[8];            /* Data field                                      */
   u8 len;                /* Length of data field in bytes                   */
   u8 ch;                 /* Object channel                                  */
   u8 format;             /* 0 - STANDARD, 1- EXTENDED IDENTIFIER            */
   u8 type;               /* 0 - DATA FRAME, 1 - REMOTE FRAME                */
}  CAN_msg;


void CAN_Config(void);
void CAN1_SendByte(u8,u8 *);
CAN_ERROR CANopen2CAN(Message* m, CanTxMsg* canmsg);
CAN_ERROR CAN2CANopen(CanTxMsg* canmsg, Message* m);

void BSP_CAN_IntEnable(void);

#endif

