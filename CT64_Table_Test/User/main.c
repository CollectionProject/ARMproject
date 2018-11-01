#include "ucos_ii.h"
#include "stm32f4xx.h"
#include "table_gpio.h"
#include "spi.h"
#include "can.h"
#include "objdictdef.h"
#include "bsp.h"
#include "iwdg.h"
#include "string.h"

GLOBALSTATE  		GlobalState         = ABNORMAL;

extern u8 horiFloatingStatus;
extern volatile u8 EMStop;

extern u8 hori_commu_flag;
extern u8 vert_commu_flag;

extern u8 m_HoriState;
extern u8 m_VertState;

u8 post_flag = 0;
u8 horiMotionStatus = 0;
u8 vertMotionStatus = 0;
u8 vertWriteCommandFlag = 0;
u8 horiWriteCommandFlag = 0;
u8 HoriFirstCommunicate = 0;
u8 VertFirstCommunicate = 0;
u8 HoriFirstPosInit = 0;
u8 VertFirstPosInit = 0;
u8 PostResult = 0x0f;
u8  HoriDisableTrigger = 0;
u8  VertDisableTrigger = 0;
u8  horiMotorStatusFlg;
u8  vertMotorStatusFlg;

u8 model_type=0;

u32 Hori_Fault_SDO=0;
u32 Vert_Fault_SDO=0;


extern proceed_info proceed_infos[];

static OS_STK startup_task_stk[STARTUP_TASK_STK_SIZE];
static OS_STK task_MonitorSC_stk[TASK_MONITORSC_STK_SIZE];
static OS_STK task_MonitorUART_stk [TASK_MONITORSC_STK_SIZE];
static OS_STK task_MonitorCANopen_stk[TASK_MONITORCANOPEN_STK_SIZE];
static OS_STK task_TableInit_stk[TABLEINIT_TASK_STK_SIZE];
static OS_STK task_Post_stk[POST_TASK_STK_SIZE];
static OS_STK task_HoriMotion_stk[HORIMOTION_TASK_STK_SIZE];
static OS_STK task_VertMotion_stk[VERTMOTION_TASK_STK_SIZE];
static OS_STK task_Disable_stk[DISABLE_TASK_STK_SIZE];
static OS_STK task_TableWatchDog_stk[TABLE_WATCHDOG_TASK_STK_SIZE];
static OS_STK task_HoriTableControl_stk[HORITABLECONTROL_TASK_STK_SIZE];
static OS_STK task_VertTableControl_stk[VERTTABLECONTROL_TASK_STK_SIZE];

#define N_MESSAGES    30    

OS_EVENT  * QueueHoriTableControl;
OS_EVENT  * QueueVertTableControl;
OS_EVENT  * QueueCAN1ISRtoTSKSC;    //2015.8.4
OS_EVENT	* MboxCAN2ISRtoTSKCANopen;
OS_EVENT  * MboxHoriMotion;
OS_EVENT  * MboxVertMotion;
OS_EVENT  * MboxDisableMotion;

OS_EVENT 	* SemHoriCommunication;
OS_EVENT 	* SemVertCommunication;
OS_EVENT 	* SemHoriReachHome;
OS_EVENT 	* SemVertReachHome;
OS_EVENT  * SemWaitSDO;
OS_EVENT  * SemWaitSDO2;
//互斥量
OS_EVENT   *CanTransmitMutexSem;                       //CAN发送互斥量
OS_EVENT   *Can2TransmitMutexSem;
////////////////////////////////////
OS_EVENT  * SemSystemPost;
////////////////////////////////////
void* CAN1MsgGrp[N_MESSAGES];      //2015.8.4
void* MsgGrp[CAN_RECEIVE_LENGTH];
void* HorimsgGrp[N_MESSAGES];
void* VertmsgGrp[N_MESSAGES];

static u8 Hori_Msg_cnt_temp,Vert_Msg_cnt_temp = 0;
static HORI_MSG hori_mbox;
static VERT_MSG vert_mbox;
static DISABLE_MSG disable_mbox;
static u8 sdo_res[8] = {0};
static u8 sdo_flag=0;

CanRxMsg  CanMsgFromSSC[N_MESSAGES];
CanRxMsg  CanMsgFromSSC1[N_MESSAGES];

static void systick_init(void)
{
	RCC_ClocksTypeDef rcc_clocks;
	RCC_GetClocksFreq(&rcc_clocks);
	SysTick_Config(rcc_clocks.HCLK_Frequency / OS_TICKS_PER_SEC);
}


static void task_TableWatchDog(void *p_arg)
{
	CanRxMsg   CanRxData;
	int i=0;
	for(;;)
	{
		//TableIWDG_Reload();
		//延长10秒
		OSTimeDly(5000);
		OSTimeDly(5000);
		
		if(GlobalState==POSTPASS)
		{
			for(i=0;i<90;i++)
			{
				CanRxData.StdId = 0x31;
				CanRxData.Data[0]=0x01;
				CanRxData.Data[2]=0x50; //水平移动距离1800
				CanRxData.Data[3]=0x46;
				CanRxData.Data[4]=0x98; //水平移动速度150
				CanRxData.Data[5]=0x3A;
				OSQPost (QueueCAN1ISRtoTSKSC, (void *)&CanRxData);
				OSTimeDly(200);
			}
			
			for(i=0;i<90;i++)
			{
				CanRxData.StdId = 0x31;
				CanRxData.Data[0]=0x01;
				CanRxData.Data[2]=0x00; //水平移动距离1800
				CanRxData.Data[3]=0x00;
				CanRxData.Data[4]=0x98; //水平移动速度150
				CanRxData.Data[5]=0x3A;
				OSQPost (QueueCAN1ISRtoTSKSC, (void *)&CanRxData);
				OSTimeDly(200);
			}
			
			for(i=0;i<90;i++)
			{
				CanRxData.StdId = 0x31;
				CanRxData.Data[0]=0x01;
				CanRxData.Data[2]=0x50; //水平移动距离1800
				CanRxData.Data[3]=0x46;
				CanRxData.Data[4]=0x98; //水平移动速度150
				CanRxData.Data[5]=0x3A;
				OSQPost (QueueCAN1ISRtoTSKSC, (void *)&CanRxData);
				OSTimeDly(200);
			}
			
			for(i=0;i<90;i++)
			{
				CanRxData.StdId = 0x31;
				CanRxData.Data[0]=0x01;
				CanRxData.Data[2]=0x00; //水平移动距离1800
				CanRxData.Data[3]=0x00;
				CanRxData.Data[4]=0x98; //水平移动速度150
				CanRxData.Data[5]=0x3A;
				OSQPost (QueueCAN1ISRtoTSKSC, (void *)&CanRxData);
				OSTimeDly(200);
			}
			
			if(model_type!=0x02)
			{
				for(i=0;i<75;i++)
				{
					CanRxData.StdId = 0x32;
					CanRxData.Data[0]=0x01;
					CanRxData.Data[2]=0xc8; //垂直移动高度660
					CanRxData.Data[3]=0x19;
					CanRxData.Data[4]=0xb8; //垂直移动速度35
					CanRxData.Data[5]=0x0b;
					OSQPost (QueueCAN1ISRtoTSKSC, (void *)&CanRxData);
					OSTimeDly(200);
				}
				
				for(i=0;i<75;i++)
				{
					CanRxData.StdId = 0x32;
					CanRxData.Data[0]=0x01;
					CanRxData.Data[2]=0x60; //垂直移动距离880
					CanRxData.Data[3]=0x22;
					CanRxData.Data[4]=0xb8; //垂直移动速度35
					CanRxData.Data[5]=0x0b;
					OSQPost (QueueCAN1ISRtoTSKSC, (void *)&CanRxData);
					OSTimeDly(200);
				}
			}
		}
	}
}

static void task_Post(void *p_arg)
{
	u8  PostResultTemp = 0;

	for(;;)
	{
		post_flag=1;
		HoriFirstCommunicate = 0x00;
		VertFirstCommunicate = 0x00;

		//Table post
		PostResultTemp = Post(model_type);
		PostResult = PostResultTemp;
		while(1)
		{
			OSTimeDly(5000);
		}
	}
}

static void task_HoriMotion(void *p_arg)
{
	u8 	err;
  HORI_MSG* msg;	
	
	for(;;)
	{
		msg = (HORI_MSG*)OSMboxPend(MboxHoriMotion, 0 , &err);
		
		if(err == OS_ERR_NONE)
		{
				OSMutexPend(Can2TransmitMutexSem,0,&err);
				horizontal_moto_contr(msg->h_acce, msg->h_dece, msg->h_speed, msg->h_pos);
				OSMutexPost(Can2TransmitMutexSem);
				horiWriteCommandFlag = 0x01;
				horiMotorStatusFlg   = 0;
		}
	}
}

static void task_VertMotion(void *p_arg)
{
	u8 	err;
  VERT_MSG* msg;	
	
	for(;;)
	{
		msg = (VERT_MSG*)OSMboxPend(MboxVertMotion, 0 , &err);
		
		if(err == OS_ERR_NONE)
		{	
				OSMutexPend(Can2TransmitMutexSem,0,&err);
				vertical_moto_contr(msg->v_speed, msg->v_pos);
				OSMutexPost(Can2TransmitMutexSem);
				vertWriteCommandFlag = 0x01;
				vertMotorStatusFlg   = 0; 
		}
	}
}

static void task_HoriTableControl(void *p_arg)
{
		u8 	err;
		CanRxMsg * CmdMsg;
		static u32 horiSpeed, horiPosition, lasthoriSpeed, lasthoriPosition;
//		u16 horiMotorStatus;

		for(;;)
		{
				CmdMsg = (CanRxMsg*)OSQPend(QueueHoriTableControl,250, &err);
				if(GlobalState==POSTPASS)
				{
						if(err==OS_ERR_NONE)
						{
								if(horiFloatingStatus)
								{
									H_CLUTCH = SET_H_CLUTCH;
									horiFloatingStatus =0;
									OSTimeDly(50);
								}
								
								if(EMStop || (CmdMsg->Data[0]==0x00))
								{
										HoriDisableTrigger   = 0x00;     
										disable_mbox.motor_id = 0x01; 
										disable_mbox.stop_mode= 0x01;
										OSMboxPost(MboxDisableMotion, (void *)&disable_mbox);
										lasthoriPosition    = 60000;
								}
								else if(CmdMsg->Data[0]==0x01)
								{
										horiSpeed    = *(u16*)&CmdMsg->Data[4];
										horiPosition = *(u16*)&CmdMsg->Data[2];
										if((lasthoriSpeed != horiSpeed)||(lasthoriPosition != horiPosition))
										{
												hori_mbox.h_speed  = horiSpeed;
												hori_mbox.h_pos    = horiPosition;
												OSMboxPost(MboxHoriMotion, (void *)&hori_mbox);
												horiMotorStatusFlg = 0x01;
										}
										lasthoriSpeed       = horiSpeed;
										lasthoriPosition    = horiPosition;
								}
						}
						else
						{
							if(m_HoriState)
							{
								HoriDisableTrigger     = 0x00;     
								disable_mbox.motor_id  = 0x01; 
								disable_mbox.stop_mode = 0x01;
								OSMboxPost(MboxDisableMotion, (void *)&disable_mbox);
								lasthoriPosition    = 60000;
							}
						}
			 }
		}
}

static void task_VertTableControl(void *p_arg)
{
	  u8 	err;
	  CanRxMsg * CmdMsg;
	  static u32 vertSpeed, vertPosition, lastvertSpeed, lastvertPosition;
    u16 vertMotorStatus;	
	
	  for(;;)
	  {
				CmdMsg = (CanRxMsg*)OSQPend(QueueVertTableControl,250, &err);
				if(GlobalState==POSTPASS)
				{
						if(err==OS_ERR_NONE)
						{
								vertMotorStatus = ReadOD(	VERT_STATUS_REG);		
							
								if(((vertMotorStatus)>> 3) & 0x01)
								{
									if(VERT_LS_BOTTOM ||VERT_LS_TOP)
									{
										handleMotionError(0x01);
										OSTimeDly(50);
									}
								}
								
								if(EMStop || (CmdMsg->Data[0]==0x00))
								{
										VertDisableTrigger   = 0x00;     
										disable_mbox.motor_id = 0x02; 
										disable_mbox.stop_mode= 0x01;
										OSMboxPost(MboxDisableMotion, (void *)&disable_mbox);
									  lastvertPosition    = 60000;
								}
								else if(CmdMsg->Data[0]==0x01)
								{
										vertSpeed    = *(u16*)&CmdMsg->Data[4];
										vertPosition = *(u16*)&CmdMsg->Data[2];
										if((lastvertSpeed != vertSpeed)||(lastvertPosition != vertPosition))
										{
												vert_mbox.v_speed  = vertSpeed;
												vert_mbox.v_pos    = vertPosition;
												OSMboxPost(MboxVertMotion, (void *)&vert_mbox);
												vertMotorStatusFlg = 0x01;
										}
										lastvertSpeed       = vertSpeed;
										lastvertPosition    = vertPosition;
								}
						}
						else
						{
							if(m_VertState)
							{
								VertDisableTrigger     = 0x00;     
								disable_mbox.motor_id  = 0x02; 
								disable_mbox.stop_mode = 0x01;
								OSMboxPost(MboxDisableMotion, (void *)&disable_mbox);
							  lastvertPosition    = 60000;
							}
						}
			 }	
	  }
}


static void task_Disable(void *p_arg)
{
	u8 	err;
  DISABLE_MSG* msg;	
	
	for(;;)
	{
		msg = (DISABLE_MSG*)OSMboxPend(MboxDisableMotion, 0 , &err);
		
		if(err == OS_ERR_NONE)
		{
				if(model_type==0x02)
				{
					if(msg->motor_id == 0x01)
					{
						OSMutexPend(Can2TransmitMutexSem,0,&err);
						handleMotionDisable ((msg->motor_id - 0x01), msg->stop_mode);	
						OSMutexPost(Can2TransmitMutexSem);
					}
				}
				else
				{
					OSMutexPend(Can2TransmitMutexSem,0,&err);
					handleMotionDisable ((msg->motor_id - 0x01), msg->stop_mode);	
					OSMutexPost(Can2TransmitMutexSem);
				}
			
				if(msg->motor_id == 0x01)  
				{              
					HoriDisableTrigger = 0x01;
				}
				else if (msg->motor_id == 0x02)
				{
					VertDisableTrigger = 0x01;
				}
				else
				{
					HoriDisableTrigger = 0x00;
					VertDisableTrigger = 0x00;
				}	
		}
	}
}


static void task_MonitorUART(void *p_arg)
{
	u8 err;
	RES_TABLE_STATE resMessage;
	for (;;)
	{		
		  Judge_Gloab_State(PostResult);
		  resMessage = ResponseData(model_type);
      OSMutexPend(CanTransmitMutexSem,0,&err);			
			HandleRespData(resMessage);
      OSMutexPost(CanTransmitMutexSem);
		  OSTimeDly(100);
	}
}

static void task_MonitorSC(void *p_arg)
{
	CanRxMsg* msg;
	u8 err;
	u8 sdo_temp[8]={0};
	u8 sdo_resp_temp[8] = {0};
	CanTxMsg TxMessage;
	TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
	
	BSP_CAN_IntEnable();	
	
	TxMessage.StdId = 0x43;
	TxMessage.DLC = 5;
	TxMessage.Data[0] = 0xFF;
	TxMessage.Data[1] = VERSION_MAJOR;
	TxMessage.Data[2] = VERSION_MINOR;
	TxMessage.Data[3] = VERSION_Y;
	TxMessage.Data[4] = VERSION_W;
	CAN_Transmit(CAN1, &TxMessage);
	OSTimeDly(100);

	for (;;)
	{		
		/*SC CAN control*/
		msg = (CanRxMsg*)OSQPend(QueueCAN1ISRtoTSKSC, 0 , &err);   //2015.8.4 
		
		switch(msg->StdId)
		{
			case 0x30:
				TxMessage.StdId = 0x40;
			  TxMessage.DLC = 1;
				TxMessage.Data[0] = 0x30;
				switch(msg->Data[0])
				{
					case 0:
						if((GlobalState!=POST) &&(msg->DLC==1))
						{
							CAN_SendTableTotal(V_BREAK_TOTAL,1);
							post_flag=1;
							PostResult = 0x0f;
							OSSemPost(SemSystemPost);
						}
						break;
					case 1:
						handleMotionError(0);
						OSTimeDly(50);
						handleMotionError(1);
						OSTimeDly(50);
						break;
					case 2:
						if(GlobalState==POSTPASS)
						{
							ShutOffMotor(0x00);
							OSTimeDly(5);
							CAN_SendTableTotal(H_FLOAT_TOTAL,1);
							H_CLUTCH = CLR_H_CLUTCH;
							H_BRAKE  = CLR_H_BRAKE;
							horiFloatingStatus = 1;
						}
						break;
					case 3:
						H_CLUTCH = SET_H_CLUTCH;
						horiFloatingStatus =0;
						break;
				}
				CAN_Transmit(CAN1, &TxMessage);
				OSTimeDly(1);
				break;
			case 0x31:
					TxMessage.StdId = 0x41;
					TxMessage.DLC = 1;
					TxMessage.Data[0] = 0x31;
					CAN_Transmit(CAN1, &TxMessage);
					//OSTimeDly(1);
					memcpy(&CanMsgFromSSC[Hori_Msg_cnt_temp],msg,sizeof(CanRxMsg));
					OSQPost(QueueHoriTableControl,(void*)&CanMsgFromSSC[Hori_Msg_cnt_temp++]);
					if (Hori_Msg_cnt_temp == N_MESSAGES)
							Hori_Msg_cnt_temp = 0x00;
				break;
			case 0x32:
					TxMessage.StdId = 0x42;
					TxMessage.DLC = 1;
					TxMessage.Data[0] = 0x32;
					CAN_Transmit(CAN1, &TxMessage);
					OSTimeDly(1);
					memcpy(&CanMsgFromSSC1[Vert_Msg_cnt_temp],msg,sizeof(CanRxMsg));
					OSQPost(QueueVertTableControl,(void*)&CanMsgFromSSC1[Vert_Msg_cnt_temp++]);
					if (Vert_Msg_cnt_temp == N_MESSAGES)
							Vert_Msg_cnt_temp = 0x00;
				break;
			case 0x33:
				TxMessage.StdId = 0x43;
				switch(msg->Data[0])
				{
					case 0xFF:
						TxMessage.DLC = 5;
						TxMessage.Data[0] = 0xFF;
						TxMessage.Data[1] = VERSION_MAJOR;
						TxMessage.Data[2] = VERSION_MINOR;
						TxMessage.Data[3] = VERSION_Y;
						TxMessage.Data[4] = VERSION_W;	
						break;
					case 0xFD:
						TxMessage.DLC = 8;
						TxMessage.Data[0] = 0xFD;
						TxMessage.Data[1] = *(u8 *)SERIAL_NO_ADDR;
						TxMessage.Data[2] = *(u8 *)(SERIAL_NO_ADDR+1);
						TxMessage.Data[3] = *(u8 *)(SERIAL_NO_ADDR+2);
						TxMessage.Data[4] = *(u8 *)(SERIAL_NO_ADDR+3);
						TxMessage.Data[5] = *(u8 *)(SERIAL_NO_ADDR+4);
						TxMessage.Data[6] = *(u8 *)(SERIAL_NO_ADDR+5);
						TxMessage.Data[7] = *(u8 *)(SERIAL_NO_ADDR+6);
						break;
					case 0xFC:
						TxMessage.DLC = 1;
						TxMessage.Data[0] = 0xFC;
					  OSSchedLock();
						FLASH_Unlock();		
						FLASH_DataCacheCmd(DISABLE);
						FLASH_EraseSector(FLASH_Sector_6,VoltageRange_3);
					  FLASH_ProgramByte(SERIAL_NO_ADDR  ,msg->Data[1]);
						FLASH_ProgramByte(SERIAL_NO_ADDR+1,msg->Data[2]);
						FLASH_ProgramByte(SERIAL_NO_ADDR+2,msg->Data[3]);
						FLASH_ProgramByte(SERIAL_NO_ADDR+3,msg->Data[4]);
						FLASH_ProgramByte(SERIAL_NO_ADDR+4,msg->Data[5]);
						FLASH_ProgramByte(SERIAL_NO_ADDR+5,msg->Data[6]);
						FLASH_ProgramByte(SERIAL_NO_ADDR+6,msg->Data[7]);
						FLASH_DataCacheCmd(ENABLE);				
						FLASH_Lock();
						OSSchedUnlock();
						break;
					case 0xFB:
						TxMessage.DLC = 1;
						TxMessage.Data[0] = 0xFB;
						if(READ_MODEL_TYPE>0x03 || READ_MODEL_TYPE!=msg->Data[1])
						{
							OSSchedLock();
							FLASH_Unlock();		
							FLASH_DataCacheCmd(DISABLE);
							FLASH_EraseSector(FLASH_Sector_10,VoltageRange_3);//Flash_Sector_Select(MODEL_TYPE_ADDR)
							FLASH_ProgramByte(MODEL_TYPE_ADDR  ,msg->Data[1]);
							FLASH_DataCacheCmd(ENABLE);				
							FLASH_Lock();
							OSSchedUnlock();
							model_type=READ_MODEL_TYPE;
						}
						break;
					}
				CAN_Transmit(CAN1, &TxMessage);
				OSTimeDly(1);
				break;
			case 0x36:
					memcpy(sdo_temp,msg->Data,8);
					writeSDO(1,sdo_temp,8);
				  sdo_flag=1;
					OSSemPend(SemWaitSDO,2000,&err);
					if(err==OS_ERR_NONE)
					{
						memset(sdo_resp_temp,0,8);
						memcpy(sdo_resp_temp,sdo_res,8);
						CAN1_SendByte(0x46,sdo_resp_temp);
					}
					else
					{
						memset(sdo_resp_temp,0,8);
						sdo_resp_temp[0] = 0xFF;
						CAN1_SendByte(0x46,sdo_resp_temp);
					}
					break;
			case 0x37:
					memcpy(sdo_temp,msg->Data,8);
					writeSDO(2,sdo_temp,8);
				  sdo_flag=1;
					OSSemPend(SemWaitSDO2,2000,&err);
					if(err==OS_ERR_NONE)
					{
						memset(sdo_resp_temp,0,8);
						memcpy(sdo_resp_temp,sdo_res,8);
						CAN1_SendByte(0x47,sdo_resp_temp);
					}
					else
					{
						memset(sdo_resp_temp,0,8);
						sdo_resp_temp[0] = 0xFF;
						CAN1_SendByte(0x47,sdo_resp_temp);
					}
					break;	
			case 0x3F:
				TxMessage.StdId = 0x4F;
				switch(msg->Data[0])
				{
					case 0x00:
						//将反馈消息任务恢复
						TxMessage.DLC = 1;
						TxMessage.Data[0] = 0x00;
						OSTaskResume(MONITORUART_TSK_PRIO);
						break;
					case 0x01:
						//将反馈消息任务挂起
						TxMessage.DLC = 1;
						TxMessage.Data[0] = 0x01;
						OSTaskSuspend(MONITORUART_TSK_PRIO);
						break;
					case 0x10:
						//升级开始
						__set_FAULTMASK(1);      // 关闭所有中端
						NVIC_SystemReset();			 // 重启
						break;
					default:
						TxMessage.Data[0] = 0xEE; //不能识别的命令字
						break;
					}
				CAN_Transmit(CAN1, &TxMessage);
				OSTimeDly(1);
				break;
		}
	}
}


static void task_MonitorCANopen(void *p_arg)
{
	CanRxMsg* msg_CAN2;
	CanTxMsg  msg_temp;
  Message msg_rece;
	u8 i;
  u8 fc1;
	u8 err;
	
  for (; ;)
  {
    {
			/*This block is only to store CANopen msg to buffer*/
      msg_CAN2 = (CanRxMsg*)OSMboxPend(MboxCAN2ISRtoTSKCANopen, 0 , &err);
			
			/* Get the fct code from cobID and enter the special handle function */
			msg_temp.RTR = msg_CAN2->RTR;
			msg_temp.IDE = msg_CAN2->IDE;
			msg_temp.DLC = msg_CAN2->DLC;
			msg_temp.StdId = msg_CAN2->StdId;
			
			for(i=0;i<8;i++)
			{
				msg_temp.Data[i] = msg_CAN2->Data[i];
			}
			
			CAN2CANopen(&msg_temp, &msg_rece);
			
			fc1 = GET_FUNCTION_CODE(msg_rece);

			if ((msg_CAN2->StdId == 0x701)&&(msg_CAN2->Data[0] != 0x0f) && (HoriFirstCommunicate == 0x00))
			{
	 			OSSemPost(SemHoriCommunication);
				HoriFirstCommunicate = 0x01;
			}
			
			if ((msg_CAN2->StdId == 0x702)&&(msg_CAN2->Data[0] != 0x0f) && (VertFirstCommunicate == 0x00))
			{
				OSSemPost(SemVertCommunication);
				VertFirstCommunicate = 0x01;
			}
			
			if ((msg_CAN2->StdId == 0x281)&&              //05
				((msg_CAN2->Data[0] == 0x37) && 
			(((msg_CAN2->Data[1] & 0x04) >>2) & 0x01) && 
			((msg_CAN2->Data[1] & 0x01)!= 0x01) ) &&
			(HoriFirstPosInit == 0x01))
			{
				OSSemPost(SemHoriReachHome);
				HoriFirstPosInit = 0x00;
			}
			
			if(msg_CAN2->StdId == 0x281)
			{
				hori_commu_flag = 0;
			}
			
			
			if ((msg_CAN2->StdId == 0x182)&&              //03
				((msg_CAN2->Data[0] == 0x37) && 
			(((msg_CAN2->Data[1] & 0x04) >>2) & 0x01) && 
			((msg_CAN2->Data[1] & 0x01)!= 0x01) ) &&
			(VertFirstPosInit == 0x01))
			{
				OSSemPost(SemVertReachHome);
				VertFirstPosInit = 0x00;
			}		
			
			if(msg_CAN2->StdId == 0x182)
			{
				vert_commu_flag = 0;
			}
			
			if((msg_CAN2->StdId == 0x581)&&(sdo_flag==1))
			{
				sdo_flag=0;
				memcpy(sdo_res,msg_CAN2->Data,8);
				OSSemPost(SemWaitSDO);
			}
			
			if(msg_CAN2->Data[1]==0x01 && msg_CAN2->Data[2]==0x10 &&msg_CAN2->StdId == 0x581)
			{
				hori_commu_flag = 0;
				Hori_Fault_SDO = *(u32 *)&msg_CAN2->Data[4];
			}
			
			if((msg_CAN2->StdId == 0x582)&&(sdo_flag==1))
			{
				sdo_flag=0;
				memcpy(sdo_res,msg_CAN2->Data,8);
				OSSemPost(SemWaitSDO2);
			}
			
			if(msg_CAN2->Data[1]==0x01 && msg_CAN2->Data[2]==0x10 &&msg_CAN2->StdId == 0x582)
			{
				vert_commu_flag = 0;
				Vert_Fault_SDO = *(u32 *)&msg_CAN2->Data[4];
			}
			
 			if(proceed_infos[fc1].process_function != NULL)
         proceed_infos[fc1].process_function(0,&msg_rece);
		}	
  }
}



static void task_TableInit(void *p_arg)
{
	INT8U err;
	
	err = OSTaskCreate(task_MonitorSC, (void *)0, &task_MonitorSC_stk[TASK_MONITORSC_STK_SIZE-1], MONITORSC_TSK_PRIO);							//monitor SC
	
	err = OSTaskCreate(task_TableWatchDog, (void *)0, &task_TableWatchDog_stk[TABLE_WATCHDOG_TASK_STK_SIZE-1], TABLEWATCHDOG_TASK_PRIO);
	
	err = OSTaskCreate(task_HoriMotion, (void *)0, &task_HoriMotion_stk[HORIMOTION_TASK_STK_SIZE-1], HORIMOTION_TASK_PRIO);
	
	err = OSTaskCreate(task_VertMotion, (void *)0, &task_VertMotion_stk[VERTMOTION_TASK_STK_SIZE-1], VERTMOTION_TASK_PRIO);
	
	err = OSTaskCreate(task_Disable, (void *)0, &task_Disable_stk[DISABLE_TASK_STK_SIZE-1], DISABLE_TASK_PRIO);
	
	err = OSTaskCreate(task_MonitorUART, (void *)0, &task_MonitorUART_stk[TASK_MONITORSC_STK_SIZE-1], MONITORUART_TSK_PRIO);				//monitor UART
	
	err = OSTaskCreate(task_HoriTableControl, (void *)0, &task_HoriTableControl_stk[HORITABLECONTROL_TASK_STK_SIZE-1], HORITABLECONTROL_TASK_PRIO);
	
	err = OSTaskCreate(task_VertTableControl, (void *)0, &task_VertTableControl_stk[VERTTABLECONTROL_TASK_STK_SIZE-1], VERTTABLECONTROL_TASK_PRIO);	
	
	OSTimeDly(5000);
	
	err	=	OSTaskCreate(task_Post, (void *)0, &task_Post_stk[POST_TASK_STK_SIZE-1], POST_TASK_PRIO);
	
	if (OS_ERR_NONE != err)
		while(1);
	
	OSTaskDel(OS_PRIO_SELF);
}


static void startup_task(void *p_arg)
{
	u8 err;
	systick_init();     /* Initialize the SysTick. */

#if (OS_TASK_STAT_EN > 0)
    OSStatInit();      /* Determine CPU capacity. */
#endif
	
	/* TODO: create application tasks here */

	if(READ_MODEL_TYPE>0x03)
		model_type=0;
	else
	{
		model_type=READ_MODEL_TYPE;
	}

	err = OSTaskCreate(task_MonitorCANopen, (void *)0, &task_MonitorCANopen_stk[TASK_MONITORCANOPEN_STK_SIZE-1], MONITORCANOPEN_TSK_PRIO);//monitor motor side   

  err = OSTaskCreate(task_TableInit, (void *)0, &task_TableInit_stk[TABLEINIT_TASK_STK_SIZE-1], TABLEINIT_TSK_PRIO);//Table init

	if (OS_ERR_NONE != err)
		while(1)
			;
	OSTaskDel(OS_PRIO_SELF);
}
	


int main(void)
{
	u8  err;
	
	//SCB ->VTOR = FLASH_BASE | 0x10000;

	//BSP_Table_IWDGInit(3500);
	Table_GPIO_Init();
	Exit_Configuration();
	CAN_Config();
	
	OSInit();
	
	QueueHoriTableControl     = OSQCreate(&HorimsgGrp[0],N_MESSAGES);
  QueueVertTableControl     = OSQCreate(&VertmsgGrp[0],N_MESSAGES);
  QueueCAN1ISRtoTSKSC       = OSQCreate(&CAN1MsgGrp[0],CAN_RECEIVE_LENGTH);  //2015.8.4	
	MboxCAN2ISRtoTSKCANopen   = OSMboxCreate((void *)0);
	
	MboxHoriMotion            = OSMboxCreate((void *)0);
	MboxVertMotion            = OSMboxCreate((void *)0);
	
	MboxDisableMotion         = OSMboxCreate((void *)0);
		
	SemHoriCommunication      = OSSemCreate(0);
	SemVertCommunication      = OSSemCreate(0);
		
	SemHoriReachHome          = OSSemCreate(0);
	SemVertReachHome          = OSSemCreate(0);
	
	
	SemSystemPost            = OSSemCreate(0);
	
	SemWaitSDO            = OSSemCreate(0);
	SemWaitSDO2            = OSSemCreate(0);
	
	
		//互斥量， CAN发数据
	CanTransmitMutexSem      = OSMutexCreate(CAN_TRANSMIT_MUTEX_PRIO,&err);	  //2015.7.29
	Can2TransmitMutexSem     = OSMutexCreate(CAN2_TRANSMIT_MUTEX_PRIO,&err);
	
	OSTaskCreate(startup_task, (void *)0,  &startup_task_stk[STARTUP_TASK_STK_SIZE - 1], STARTUP_TASK_PRIO);
	OSStart();
	return 0;
}


