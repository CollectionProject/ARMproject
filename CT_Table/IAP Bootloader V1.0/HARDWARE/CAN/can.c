#include "can.h"
#include "delay.h" 
#include "string.h"

u8 block_start = 0;
u8 block_end =0;
u8 total_end =0;
u32 addr_offset =0;
u16 total_bytes = 0;
u8 CAN1_TX_ok;

//CAN�������ݺͶ��ж���
CanRxMsg   *pCanRxBuf;                    //����ָ��   
CanRxMsg   CanRxData[CAN_RECEIVE_LENGTH]; //������������
volatile unsigned char WriteFlg; 


u8 CAN_RX_BUF[CAN_REC_LEN] __attribute__ ((at(0X20001000)));//���ջ���,���USART_REC_LEN���ֽ�,��ʼ��ַΪ0X20001000.    
u32 CAN_RX_CNT=0;			//���յ��ֽ���	

void CAN_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
	NVIC_InitTypeDef  NVIC_InitStructure;
  CAN_InitTypeDef        CAN_InitStructure;
  CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	
	pCanRxBuf = CanRxData;
	WriteFlg =0;
 /* GPIOB, GPIOD and AFIO clocks enable */
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

  /* CAN1 and CAN2 Periph clocks enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE); 
	
  /* CAN PIN CONFIG		*/
		//��ʼ��GPIO
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0| GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
	GPIO_Init(GPIOD, &GPIO_InitStructure);//��ʼ��PD0,PD1

	GPIO_PinAFConfig(GPIOD,GPIO_PinSource0,GPIO_AF_CAN1);
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource1,GPIO_AF_CAN1);

   /*CAN Interuption config */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x2; //����CAN1�Ľ������ȼ�Ϊ2
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	/* enabling CAN1 ERR interrupt */  
	NVIC_InitStructure.NVIC_IRQChannel=CAN1_SCE_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x1;  //����CAN1�Ĵ����ж����ȼ�Ϊ1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	NVIC_InitStructure.NVIC_IRQChannel = CAN1_TX_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0x3; //����CAN1�ķ������ȼ�Ϊ3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0x0;
	NVIC_Init(&NVIC_InitStructure);

  /* CAN register init */
  CAN_DeInit(CAN1);
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
	
		 /* IT Configuration for CAN1 */  
	CAN_ITConfig(CAN1, CAN_IT_FMP0 | CAN_IT_TME|CAN_IT_LEC|CAN_IT_ERR, ENABLE);
	
}


void CAN1_SendByte(u8 CanID,u8 len,u8 *CanSendByte)
{
  CanTxMsg TxMessage;
  TxMessage.ExtId = 0x01;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = len;
  TxMessage.StdId = CanID;
	memcpy(TxMessage.Data,CanSendByte,len);
//  do
//	{
//		CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
//		delay_ms(1);
//	}
//	while (!CAN1_TX_ok);
	CAN_Transmit(CAN1, &TxMessage);
	delay_ms(1);
}

void CAN1_RX0_IRQHandler(void)
{
	CAN_Receive(CAN1, CAN_FIFO0, pCanRxBuf);
	
	if(pCanRxBuf->StdId == RX_DAT_ID)
	{
		memcpy(&CAN_RX_BUF[CAN_RX_CNT],pCanRxBuf->Data,pCanRxBuf->DLC);
		CAN_RX_CNT = CAN_RX_CNT + pCanRxBuf->DLC;
	}
	else if(pCanRxBuf->StdId == RX_CMD_ID)
	{
		if(pCanRxBuf->Data[0] == RX_CMD_BLOCK_START)
		{
			addr_offset = *(u32 *)&pCanRxBuf->Data[1];
			CAN_RX_CNT = addr_offset;
			block_start = 1;
		}
		else if(pCanRxBuf->Data[0] == RX_CMD_BLOCK_END)
		{
			total_bytes = (*(u32 *)&pCanRxBuf->Data[1])&0x00FFFFFF;
			block_end =1;
		}
		else if(pCanRxBuf->Data[0] == RX_CMD_TOTAL_END)
		{
			total_end = 1;
		}
	}
	
	if( WriteFlg == (CAN_RECEIVE_LENGTH-1))
	{ 
      WriteFlg = 0;
  }		
	else
	{
		  WriteFlg++;
	}
	
	pCanRxBuf = &CanRxData[WriteFlg];
}

void CAN1_SCE_IRQHandler(void)
{
	u32 ErrorReg = CAN1->ESR;
	CAN_ClearITPendingBit(CAN1,CAN_IT_LEC);	
}

void CAN1_TX_IRQHandler(void)
{	
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
}


void start_download(void)
{
	u8 buf[8]={0};

	buf[0] = 0x10;
	CAN1_SendByte(0x4F,1,buf);
}

void end_download(void)
{
	u8 buf[8]={0};

	buf[0] = 0x11;
	CAN1_SendByte(0x4F,1,buf);
}

void start_block_trans(void)
{
	u8 buf[8]={0};

	buf[0] = 0x20;
	CAN1_SendByte(0x4F,1,buf);
}

static u32   CRC32[256];
static u8 init =0;
//��ʼ����
static void init_table()
{
    int   i,j;
    u32   crc;
    for(i = 0;i < 256;i++)
    {
         crc = i;
        for(j = 0;j < 8;j++)
        {
            if(crc & 1)
            {
                 crc = (crc >> 1) ^ 0xEDB88320;
            }
            else
            {
                 crc = crc >> 1;
            }
        }
         CRC32[i] = crc;
    }
}

//crc32ʵ�ֺ���
u32 crc32(unsigned char *buf, unsigned int start, unsigned int len)
{
    u32 ret = 0xFFFFFFFF;
    int   i;
    if( !init )
    {
         init_table();
         init = 1;
    }
    for(i = 0; i < len;i++)
    {
         ret = CRC32[((ret & 0xFF) ^ buf[i+start])] ^ (ret >> 8);
    }
     ret = ~ret;
    return ret;
}


void end_block_trans(u32 offset,u32 lenofbyte)
{
	u8 buf[8]={0};
	u32 CRC_Value = 0;

	buf[0] = 0x21;

	CRC_Value = crc32(CAN_RX_BUF,offset,lenofbyte);
	
	memcpy(&buf[1],&CRC_Value,4);
	
	CAN1_SendByte(0x4F,5,buf);	
}





