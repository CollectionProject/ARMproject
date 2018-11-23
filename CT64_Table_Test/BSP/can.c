<<<<<<< HEAD
#include "can.h"
#include "bsp.h"
#include "def.h"
#include "ucos_ii.h"

extern OS_EVENT	* MboxCAN2ISRtoTSKCANopen;
u8 CAN1_TX_ok;

//CAN接受数据和队列定义
extern OS_EVENT  * QueueCAN1ISRtoTSKSC;    //2015.8.4
CanRxMsg   CanRxData[CAN_RECEIVE_LENGTH]; //接受数据数组  
CanRxMsg   *pCanRxBuf;                    //接受指针   
volatile unsigned char WriteFlg; 

void CAN_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
  CAN_InitTypeDef        CAN_InitStructure;
  CAN_FilterInitTypeDef  CAN_FilterInitStructure;
 
	//参数初始化
	pCanRxBuf        = CanRxData;	
	WriteFlg         = 0;
	
 /* GPIOB, GPIOD and AFIO clocks enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOB, ENABLE);

  /* CAN1 and CAN2 Periph clocks enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1|RCC_APB1Periph_CAN2, ENABLE); 
	
  /* CAN PIN CONFIG		*/
		//初始化GPIO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0| GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化PD0,PD1

  		//初始化GPIO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5| GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化PB5,PB6

	GPIO_PinAFConfig(GPIOD,GPIO_PinSource0,GPIO_AF_CAN1);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource1,GPIO_AF_CAN1);
	
  GPIO_PinAFConfig(GPIOB,GPIO_PinSource5,GPIO_AF_CAN2);
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_CAN2);

   /*CAN Interuption config */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x2; //设置CAN1的接收优先级为2
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	/* enabling CAN1 ERR interrupt */  
	NVIC_InitStructure.NVIC_IRQChannel=CAN1_SCE_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x1;  //设置CAN1的错误中断优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	 NVIC_InitStructure.NVIC_IRQChannel = CAN1_TX_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x3; //设置CAN1的发送优先级为3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
	 NVIC_Init(&NVIC_InitStructure);

   NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn;		   //设置CAN2接收的优先级为0
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0;
   NVIC_Init(&NVIC_InitStructure);

  /* CAN register init */
  CAN_DeInit(CAN1);
  CAN_DeInit(CAN2);
  CAN_StructInit(&CAN_InitStructure);

  /* CAN1 cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM =  ENABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = DISABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
  CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
  CAN_InitStructure.CAN_BS2 = CAN_BS2_7tq;
  CAN_InitStructure.CAN_Prescaler = 6;         //   CAN BAUD RATE:  42M /6*(1+6+7)  =500kbps
  CAN_Init(CAN1, &CAN_InitStructure);
  CAN_Init(CAN2, &CAN_InitStructure);

  /* CAN1 filter init */
  CAN_FilterInitStructure.CAN_FilterNumber = 0;
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh= (((u32)0x0030<<21)&0xFFFF0000)>>16;                         // ALLOWED ID 0x003X
  CAN_FilterInitStructure.CAN_FilterIdLow=(((u32)0x0000)<<21|CAN_ID_STD|CAN_RTR_DATA)&0x0000FFFF;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFE00;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);
  /* CAN2 filter init */
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterNumber = 14;
  CAN_FilterInit(&CAN_FilterInitStructure);

}

#if 1 //start by wang.jun 2015.12.21
void BSP_CAN_IntEnable(void)
{
	 /* IT Configuration for CAN1 */  
  //CAN_ITConfig(CAN1, CAN_IT_FMP0 | CAN_IT_TME, ENABLE);
	CAN_ITConfig(CAN1, CAN_IT_FMP0 | CAN_IT_TME|CAN_IT_LEC|CAN_IT_ERR, ENABLE);       // 
  /* IT Configuration for CAN2 */  
  CAN_ITConfig(CAN2, CAN_IT_FMP0, ENABLE);
}
#endif //end by wang.jun 2015.12.21


void CAN1_SendByte(u8 CanID,u8 *CanSendByte)
{
  CanTxMsg TxMessage;
  TxMessage.ExtId = 0x01;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 8;
  TxMessage.StdId = CanID;
  TxMessage.Data[0] = CanSendByte[0];
	TxMessage.Data[1] = CanSendByte[1];
	TxMessage.Data[2] = CanSendByte[2];
	TxMessage.Data[3] = CanSendByte[3];
	TxMessage.Data[4] = CanSendByte[4];
	TxMessage.Data[5] = CanSendByte[5];
	TxMessage.Data[6] = CanSendByte[6];
	TxMessage.Data[7] = CanSendByte[7];
  do
	{
		CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
		OSTimeDly(1);
	}
	while (!CAN1_TX_ok);
}



CAN_ERROR CANopen2CAN(Message* m, CanTxMsg* canmsg)
{
	u8 i;

	canmsg->StdId = m->cob_id.w;
	
	if (m->rtr == REQUETE)
		canmsg->RTR  = CAN_RTR_REMOTE;
	else
		canmsg->RTR  = CAN_RTR_DATA;
	
	canmsg->DLC   = m->len;
	canmsg->IDE   = CAN_ID_STD;
	
	for (i =0 ;i < m->len ; i++)
	 canmsg->Data[i] = m->data[i]; 

	return CAN_OK;
}

CAN_ERROR CAN2CANopen(CanTxMsg* canmsg, Message* m)
{
   u8 i;

   m->cob_id.w  = canmsg->StdId;
	 
	 if (canmsg->RTR == CAN_RTR_REMOTE)
		 m->rtr = REQUETE;
	 else
		 m->rtr = DONNEES;
	 
   m->len       = canmsg->DLC;
	 
   for (i =0 ;i < canmsg->DLC ;i++)
     m->data[i] = canmsg->Data[i];

   return CAN_OK;
}
 
void CAN1_RX0_IRQHandler(void)
{
	OSIntEnter();

	CAN_Receive(CAN1, CAN_FIFO0, pCanRxBuf);
	
	OSQPost (QueueCAN1ISRtoTSKSC, (void *)pCanRxBuf);		
	
  if( WriteFlg == (CAN_RECEIVE_LENGTH-1))
	{ 
      WriteFlg = 0;
  }		
	else
	{
		  WriteFlg++;
	}
	
	pCanRxBuf = &CanRxData[WriteFlg];
	OSIntExit();
}

void CAN1_SCE_IRQHandler(void)
{
	u32 ErrorReg = CAN1->ESR;
	
	CAN_ClearITPendingBit(CAN1,CAN_IT_LEC);	
}

void CAN1_TX_IRQHandler(void)
{	
	OSIntEnter();
	if (CAN1->TSR&CAN_TSR_RQCP0)
	{
	  if (CAN1->TSR&CAN_TSR_TXOK0)
		  CAN1_TX_ok = 0x01;
	  else
		  CAN1_TX_ok = 0x00;
		CAN1->TSR|=CAN_TSR_ABRQ0;
	} 
	if (CAN1->TSR&CAN_TSR_RQCP1)
	{
		if (CAN1->TSR&CAN_TSR_TXOK1)
		  CAN1_TX_ok = 0x01;
	  else
		  CAN1_TX_ok = 0x00;
		CAN1->TSR|=CAN_TSR_ABRQ1;
	}
	if (CAN1->TSR&CAN_TSR_RQCP2)
	{
		if (CAN1->TSR&CAN_TSR_TXOK2)
		  CAN1_TX_ok = 0x01;
	  else
		  CAN1_TX_ok = 0x00;
		CAN1->TSR|=CAN_TSR_ABRQ2;
	}
	OSIntExit();
}

CanRxMsg RxMessage2;
void CAN2_RX0_IRQHandler(void)
{
	OSIntEnter();	
  CAN_Receive(CAN2, CAN_FIFO0, &RxMessage2);
	if (RxMessage2.StdId != 0x80)
		OSMboxPost(MboxCAN2ISRtoTSKCANopen, (void *)&RxMessage2);
	OSIntExit();
}






=======
#include "can.h"
#include "bsp.h"
#include "def.h"
#include "ucos_ii.h"

extern OS_EVENT	* MboxCAN2ISRtoTSKCANopen;
u8 CAN1_TX_ok;

//CAN接受数据和队列定义
extern OS_EVENT  * QueueCAN1ISRtoTSKSC;    //2015.8.4
CanRxMsg   CanRxData[CAN_RECEIVE_LENGTH]; //接受数据数组  
CanRxMsg   *pCanRxBuf;                    //接受指针   
volatile unsigned char WriteFlg; 

void CAN_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
  CAN_InitTypeDef        CAN_InitStructure;
  CAN_FilterInitTypeDef  CAN_FilterInitStructure;
 
	//参数初始化
	pCanRxBuf        = CanRxData;	
	WriteFlg         = 0;
	
 /* GPIOB, GPIOD and AFIO clocks enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD|RCC_AHB1Periph_GPIOB, ENABLE);

  /* CAN1 and CAN2 Periph clocks enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1|RCC_APB1Periph_CAN2, ENABLE); 
	
  /* CAN PIN CONFIG		*/
		//初始化GPIO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0| GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化PD0,PD1

  		//初始化GPIO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5| GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
	GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化PB5,PB6

	GPIO_PinAFConfig(GPIOD,GPIO_PinSource0,GPIO_AF_CAN1);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource1,GPIO_AF_CAN1);
	
  GPIO_PinAFConfig(GPIOB,GPIO_PinSource5,GPIO_AF_CAN2);
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource6,GPIO_AF_CAN2);

   /*CAN Interuption config */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x2; //设置CAN1的接收优先级为2
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	/* enabling CAN1 ERR interrupt */  
	NVIC_InitStructure.NVIC_IRQChannel=CAN1_SCE_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x1;  //设置CAN1的错误中断优先级为1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	 NVIC_InitStructure.NVIC_IRQChannel = CAN1_TX_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x3; //设置CAN1的发送优先级为3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
	 NVIC_Init(&NVIC_InitStructure);

   NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX0_IRQn;		   //设置CAN2接收的优先级为0
   NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x0;
   NVIC_Init(&NVIC_InitStructure);

  /* CAN register init */
  CAN_DeInit(CAN1);
  CAN_DeInit(CAN2);
  CAN_StructInit(&CAN_InitStructure);

  /* CAN1 cell init */
  CAN_InitStructure.CAN_TTCM = DISABLE;
  CAN_InitStructure.CAN_ABOM =  ENABLE;
  CAN_InitStructure.CAN_AWUM = DISABLE;
  CAN_InitStructure.CAN_NART = DISABLE;
  CAN_InitStructure.CAN_RFLM = DISABLE;
  CAN_InitStructure.CAN_TXFP = DISABLE;
  CAN_InitStructure.CAN_Mode = CAN_Mode_Normal;
  CAN_InitStructure.CAN_SJW = CAN_SJW_1tq;
  CAN_InitStructure.CAN_BS1 = CAN_BS1_6tq;
  CAN_InitStructure.CAN_BS2 = CAN_BS2_7tq;
  CAN_InitStructure.CAN_Prescaler = 6;         //   CAN BAUD RATE:  42M /6*(1+6+7)  =500kbps
  CAN_Init(CAN1, &CAN_InitStructure);
  CAN_Init(CAN2, &CAN_InitStructure);

  /* CAN1 filter init */
  CAN_FilterInitStructure.CAN_FilterNumber = 0;
  CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;
  CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;
  CAN_FilterInitStructure.CAN_FilterIdHigh= (((u32)0x0030<<21)&0xFFFF0000)>>16;                         // ALLOWED ID 0x003X
  CAN_FilterInitStructure.CAN_FilterIdLow=(((u32)0x0000)<<21|CAN_ID_STD|CAN_RTR_DATA)&0x0000FFFF;
  CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0xFE00;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterFIFOAssignment = 0;
  CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;
  CAN_FilterInit(&CAN_FilterInitStructure);
  /* CAN2 filter init */
	CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;
  CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;
  CAN_FilterInitStructure.CAN_FilterNumber = 14;
  CAN_FilterInit(&CAN_FilterInitStructure);

}

#if 1 //start by wang.jun 2015.12.21
void BSP_CAN_IntEnable(void)
{
	 /* IT Configuration for CAN1 */  
  //CAN_ITConfig(CAN1, CAN_IT_FMP0 | CAN_IT_TME, ENABLE);
	CAN_ITConfig(CAN1, CAN_IT_FMP0 | CAN_IT_TME|CAN_IT_LEC|CAN_IT_ERR, ENABLE);       // 
  /* IT Configuration for CAN2 */  
  CAN_ITConfig(CAN2, CAN_IT_FMP0, ENABLE);
}
#endif //end by wang.jun 2015.12.21


void CAN1_SendByte(u8 CanID,u8 *CanSendByte)
{
  CanTxMsg TxMessage;
  TxMessage.ExtId = 0x01;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 8;
  TxMessage.StdId = CanID;
  TxMessage.Data[0] = CanSendByte[0];
	TxMessage.Data[1] = CanSendByte[1];
	TxMessage.Data[2] = CanSendByte[2];
	TxMessage.Data[3] = CanSendByte[3];
	TxMessage.Data[4] = CanSendByte[4];
	TxMessage.Data[5] = CanSendByte[5];
	TxMessage.Data[6] = CanSendByte[6];
	TxMessage.Data[7] = CanSendByte[7];
  do
	{
		CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
		OSTimeDly(1);
	}
	while (!CAN1_TX_ok);
}



CAN_ERROR CANopen2CAN(Message* m, CanTxMsg* canmsg)
{
	u8 i;

	canmsg->StdId = m->cob_id.w;
	
	if (m->rtr == REQUETE)
		canmsg->RTR  = CAN_RTR_REMOTE;
	else
		canmsg->RTR  = CAN_RTR_DATA;
	
	canmsg->DLC   = m->len;
	canmsg->IDE   = CAN_ID_STD;
	
	for (i =0 ;i < m->len ; i++)
	 canmsg->Data[i] = m->data[i]; 

	return CAN_OK;
}

CAN_ERROR CAN2CANopen(CanTxMsg* canmsg, Message* m)
{
   u8 i;

   m->cob_id.w  = canmsg->StdId;
	 
	 if (canmsg->RTR == CAN_RTR_REMOTE)
		 m->rtr = REQUETE;
	 else
		 m->rtr = DONNEES;
	 
   m->len       = canmsg->DLC;
	 
   for (i =0 ;i < canmsg->DLC ;i++)
     m->data[i] = canmsg->Data[i];

   return CAN_OK;
}
 
void CAN1_RX0_IRQHandler(void)
{
	OSIntEnter();

	CAN_Receive(CAN1, CAN_FIFO0, pCanRxBuf);
	
	OSQPost (QueueCAN1ISRtoTSKSC, (void *)pCanRxBuf);		
	
  if( WriteFlg == (CAN_RECEIVE_LENGTH-1))
	{ 
      WriteFlg = 0;
  }		
	else
	{
		  WriteFlg++;
	}
	
	pCanRxBuf = &CanRxData[WriteFlg];
	OSIntExit();
}

void CAN1_SCE_IRQHandler(void)
{
	u32 ErrorReg = CAN1->ESR;
	
	CAN_ClearITPendingBit(CAN1,CAN_IT_LEC);	
}

void CAN1_TX_IRQHandler(void)
{	
	OSIntEnter();
	if (CAN1->TSR&CAN_TSR_RQCP0)
	{
	  if (CAN1->TSR&CAN_TSR_TXOK0)
		  CAN1_TX_ok = 0x01;
	  else
		  CAN1_TX_ok = 0x00;
		CAN1->TSR|=CAN_TSR_ABRQ0;
	} 
	if (CAN1->TSR&CAN_TSR_RQCP1)
	{
		if (CAN1->TSR&CAN_TSR_TXOK1)
		  CAN1_TX_ok = 0x01;
	  else
		  CAN1_TX_ok = 0x00;
		CAN1->TSR|=CAN_TSR_ABRQ1;
	}
	if (CAN1->TSR&CAN_TSR_RQCP2)
	{
		if (CAN1->TSR&CAN_TSR_TXOK2)
		  CAN1_TX_ok = 0x01;
	  else
		  CAN1_TX_ok = 0x00;
		CAN1->TSR|=CAN_TSR_ABRQ2;
	}
	OSIntExit();
}

CanRxMsg RxMessage2;
void CAN2_RX0_IRQHandler(void)
{
	OSIntEnter();	
  CAN_Receive(CAN2, CAN_FIFO0, &RxMessage2);
	if (RxMessage2.StdId != 0x80)
		OSMboxPost(MboxCAN2ISRtoTSKCANopen, (void *)&RxMessage2);
	OSIntExit();
}






>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
