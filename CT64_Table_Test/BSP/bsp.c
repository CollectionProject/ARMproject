<<<<<<< HEAD
  /******************************************************************************
  * @file    bsp.c
  * @author  Vic. WangQingqiang
  * @version V2.0.0
  * @date    2018.3.26
  * @brief   This file provides bed controller hardware related functions.
  ******************************************************************************/
#include "can.h"
#include "bsp.h"
#include "table_gpio.h"
#include "math.h"
#include "objacces.h"
#include "nmtMaster.h"
#include "sdo.h"
#include "init.h"
#include "ucos_ii.h"														
extern OS_EVENT  * QueueCAN1ISRtoTSKSC;    //2015.8.4
extern OS_EVENT * SemHoriCommunication;
extern OS_EVENT * SemVertCommunication;
extern OS_EVENT * SemHoriReachHome;
extern OS_EVENT * SemVertReachHome;
extern GLOBALSTATE  		GlobalState;
extern u8 CAN1_TX_ok;
extern volatile u8 EMStop;
extern u8 post_flag;
extern u8 vertWriteCommandFlag;
extern u8 horiWriteCommandFlag;
extern u8 horiMotorStatusFlg;
extern u8 vertMotorStatusFlg;
extern u8 HoriDisableTrigger;
extern u8 VertDisableTrigger;
extern u8 HoriFirstPosInit ;
extern u8 VertFirstPosInit;
extern u32 Hori_Fault_SDO;
extern u32 Vert_Fault_SDO;
extern u8 model_type;

u32 vertPoleCounterPowerOn = 0x13AF095;//0x13E35A6;//0x1ac7788//0x13ACC95;
u8  horiFloatingStatus=0;
u8  HoriCommunicateOK;
u8  VertCommunicateOK;
u8 connectedNode[3];

u8 hori_commu_flag=0;
u8 vert_commu_flag=0;

volatile u8 m_HoriState=0;
volatile u8 m_VertState=0;

static u16 HoriPosTemp=0;
static u16 VertPosTemp=9900;
static u32 HoriPosDelta=0;
static u32 VertPosDelta=0;
static u16 errcode;
static u8  EMStop_before;
u32 GetCurrentPoleCount(void)               //获得当前推杆的相对编码器counter 值，并加上上电初始偏移量
{
	u32 curPoleCount;
	curPoleCount = ReadOD(VERT_POS_REG);
	curPoleCount += vertPoleCounterPowerOn;
	
	return (curPoleCount);
};

double HeightToPoleLength(double Height)   //根据高度计算推杆长度，返回值mm ,公式提供：孙雪超
{
	double y;
	y= sqrt(530489.0921 - 327579.8355 * cos(asin((Height - 315.0)/780.00) + 0.34821));
	return y;
}

double PoleLengthToHeight(double polelength)   //根据推杆长度计算高度，返回值mm ,公式提供：孙雪超
{
	double y;
	y = 780.00 * sin(acos(1.6194-0.00000305*polelength*polelength)-0.348172) + 315.0;
	return y;
}

u32 GetCurrentHorizontalPosition(void)  // return value in 0.01 mm
{
	u32 curHorizontalCount;

	curHorizontalCount = ReadOD(HORI_POS_REG);
	
	if(curHorizontalCount>0xFFF0BDC0)   //如果大于-1000000counts，正常范围是4800~-604800
	{
		curHorizontalCount = (u32)((double)(0xffffffff - curHorizontalCount + 1)/(double)horizontal_incr_position_25MM);
	}
	else
	{
		curHorizontalCount = (u32)((double)curHorizontalCount/(double)horizontal_incr_position_25MM);
	}
	
	return (u32)(curHorizontalCount);	
}

void Judge_Gloab_State(u8 PostResult)
{
	  if(post_flag ==0x01)
		{
				if (PostResult == 0xff)
				{
					GlobalState = POSTPASS;
					errcode &=0xFFFFFFF0;
				}
				else if ((PostResult&0x0F)^0x0F)
				{	
					GlobalState = POSTFAIL;
					errcode &=0xFFFFFFF0;
					if(((~PostResult)>>3)&0x01)
					{
						errcode |= 0x02;
					}
					if(((~PostResult)>>2)&0x01)
					{
						errcode |= 0x01;
					}
					if(((~PostResult)>>1)&0x01)
					{
						errcode |= 0x02;
					}
					if(((~PostResult)>>0)&0x01)
					{
						errcode |= 0x01;
					}
				}
				else
				{
					GlobalState = POST;	
					errcode &=0xFFFFFFF0;
				}
		}
		else
		{
			GlobalState = ABNORMAL;	
		}
					
}
	
void CAN_SendRespFramePart1(ID_44 message)
{
	u8 can_cnt=0;
  CanTxMsg TxMessage;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 8;
  TxMessage.StdId = 0x44;
  TxMessage.Data[0] = (u8)(message.HoriPos&0xFF);
  TxMessage.Data[1] = (u8)((message.HoriPos>>8)&0xFF);
  TxMessage.Data[2] = (u8)(message.HoriSpeed&0xFF);
  TxMessage.Data[3] = (u8)((message.HoriSpeed>>8)&0xFF);
  TxMessage.Data[4] = (u8)(message.VertPos&0xFF);
  TxMessage.Data[5] = (u8)((message.VertPos>>8)&0xFF);
  TxMessage.Data[6] = (u8)(message.VertSpeed&0xFF);
  TxMessage.Data[7] = (u8)((message.VertSpeed>>8)&0xFF);
  do
	{
		CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
		OSTimeDly(1);
		if(can_cnt==200)
			break;
		else
			can_cnt++;
	}
	while (!CAN1_TX_ok);
}

void CAN_SendRespFramePart2(ID_45 message)
{
	u8 can_cnt=0;
  CanTxMsg TxMessage;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 8;
  TxMessage.StdId = 0x45;
	
  TxMessage.Data[0] = message.GlobalState;
  TxMessage.Data[1] = message.HoriState;
  TxMessage.Data[2] = message.VertState;
  TxMessage.Data[3] = message.Float_State;
  TxMessage.Data[4] = message.EMS_Stop_State;
  TxMessage.Data[5] = message.IO_State;
  TxMessage.Data[6] = message.errcode&0xFF;
  TxMessage.Data[7] = (message.errcode>>8)&0xFF;
  do
	{
		CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
		OSTimeDly(1);
		if(can_cnt==200)
			break;
		else
			can_cnt++;
	}
	while (!CAN1_TX_ok);
}

void CAN_SendRespFramePart3(ID_49 message)
{
		u8 can_cnt=0;
  CanTxMsg TxMessage;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 8;
  TxMessage.StdId = 0x49;
	
  TxMessage.Data[0] = message.Hori_err&0xFF;
  TxMessage.Data[1] = (message.Hori_err>>8)&0xFF;
  TxMessage.Data[2] = (message.Hori_err>>16)&0xFF;
  TxMessage.Data[3] = (message.Hori_err>>24)&0xFF;
  TxMessage.Data[4] = message.Vert_err;
  TxMessage.Data[5] = (message.Vert_err>>8)&0xFF;
  TxMessage.Data[6] = (message.Vert_err>>16)&0xFF;
  TxMessage.Data[7] = (message.Vert_err>>24)&0xFF;
  do
	{
		CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
		OSTimeDly(1);
		if(can_cnt==200)
			break;
		else
			can_cnt++;
	}
	while (!CAN1_TX_ok);
}

void HandleRespData(RES_TABLE_STATE msg)
{
	CAN_SendRespFramePart1(msg.id44_res);
	CAN_SendRespFramePart2(msg.id45_res);
}

void CAN_SendTableTotal(u8 type,u32 delta)
{
	u8 can_cnt=0;
	CanTxMsg TxMessage;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 5;
	TxMessage.StdId = 0x48;
	
	if(type<4)
	{
		TxMessage.Data[0] = delta&0xFF;
		TxMessage.Data[1] = (delta>>8)&0xFF;
		TxMessage.Data[2] = (delta>>16)&0xFF;
		TxMessage.Data[3] = (delta>>24)&0xFF;
		TxMessage.Data[4] = type;
		do
		{
			CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
			OSTimeDly(1);
			if(can_cnt==200)
				break;
			else
				can_cnt++;
		}
		while (!CAN1_TX_ok);
	}
}

/***********************************************************/
RES_TABLE_STATE ResponseData(u8 type)
{
	u32 curVertSpeed;
	u32 curVertStandardSpeed;
	u32 curHoriSpeed;
	u32 curHoriStandardSpeed;
	u8 IO_state;
	u16 horiMotorStatus=0;
	u16 vertMotorStatus;
	u8 horiMotionStatus=0;
	u8 vertMotionStatus=0;

	u32 RealVertCounts = 0;
	u8 HoriVert_SDO[8] = {0x40,0x01,0x10,0x00,0x00,0x00,0x00,0x00};
	ID_49 ID49;

	RES_TABLE_STATE ResMsgTemp;
	double motor_pos_offset;
	
	
	{//读取水平驱动器位置,单位0.1mm
		ResMsgTemp.id44_res.HoriPos = (u16)(GetCurrentHorizontalPosition()/10);
		//读取水平驱动器速度,单位0.01mm/s
		curHoriSpeed = ReadOD(HORI_SPEED_REG);
		if (curHoriSpeed > 0x80000000)//若是一个负数
		{
			curHoriStandardSpeed = 0xffffffff - curHoriSpeed + 1;
		}
		else
			curHoriStandardSpeed = curHoriSpeed;
		
		ResMsgTemp.id44_res.HoriSpeed = (u16)((double)curHoriStandardSpeed/(double)horizontal_encoder_speed_25MM);
	}
	
	{//读取当前水平电机是否出错
		horiMotorStatus = ReadOD(HORI_STATUS_REG);		
		if(((horiMotorStatus)>> 3) & 0x01)
		{//出错了
			errcode &=0xFFFFFFEF;
			errcode |=0x00000010;
		}
		else
		{
			errcode &=0xFFFFFFEF;
		}	
	}
	
	{//读取当前水平电机状态，0=停止，1=运动
			//horiMotionStatus在是在发送一次命令之后并且运动到位才有效
		horiMotionStatus = (horiMotorStatus)>>10 & 0x01;
			//horiMotorStatusFlg是在发送给驱动器位置，速度等参数时置1，在驱动器接受完之后置0。
		if(horiMotorStatusFlg==0)      
		{
				//满足4个条件之一表示运动停止。1.运动完成 2.有停止命令发送给驱动器 3.碰到左限位开关 4.碰到右限位开关
				if(horiMotionStatus||!HORI_LS_LEFT||!HORI_LS_RIGHT)
				{
						m_HoriState = 0;
						//HoriDisableTrigger清零表示可以接收下一次停止命令
						HoriDisableTrigger = 0;
						//horiWriteCommandFlag清零表示可以接收下一次命令
						horiWriteCommandFlag =0;
						if(GlobalState==POSTPASS)
						{
							HoriPosDelta += abs(HoriPosTemp-ResMsgTemp.id44_res.HoriPos);
							if(HoriPosDelta>1000)
							{
								CAN_SendTableTotal(HORI_TOTAL,HoriPosDelta);
								HoriPosDelta = 0;
							}
							HoriPosTemp = ResMsgTemp.id44_res.HoriPos;
						}
				}
				else
				{
					m_HoriState = 1;
				}
		}
	}
	
	if(type!=0x02)
	{
		{//读取当前垂直电机是否出错
			vertMotorStatus = ReadOD(VERT_STATUS_REG);	
			if(((vertMotorStatus)>> 3) & 0x01)
			{
				errcode &=0xFFFFFFDF;
				errcode |=0x00000020;
			}
			else
			{
				errcode &=0xFFFFFFDF;
			}
		}
		
		{//读取垂直驱动器位置速度
			//读取垂直驱动器位置，单位0.1mm
			RealVertCounts   =  GetCurrentPoleCount();
			motor_pos_offset = PoleLengthToHeight(((double)RealVertCounts)/(double)30000);

			ResMsgTemp.id44_res.VertPos = (u32)(motor_pos_offset*10);
			//读取垂直驱动器速度
			curVertSpeed = ReadOD(VERT_SPEED_REG);
			if (curVertSpeed > 0x900000)//   31mm/s 一般速度10mm/s
			{
				curVertStandardSpeed = 0xffffffff - curVertSpeed + 1;
			}
			else
				curVertStandardSpeed = curVertSpeed;
			
			ResMsgTemp.id44_res.VertSpeed = (u16)((((double)curVertStandardSpeed)/(double)200000)*(double)235);//new vertical motor
		}
		
		{//读取当前垂直电机状态，0=停止，1=运动
			vertMotionStatus = (vertMotorStatus)>>10 & 0x01;
			if(vertMotorStatusFlg==0)      
			{
					if(vertMotionStatus||!VERT_LS_TOP||!VERT_LS_BOTTOM)
					{
							m_VertState = 0;
							VertDisableTrigger = 0;
							vertWriteCommandFlag =0;
							if(GlobalState==POSTPASS)
							{
								VertPosDelta += abs(VertPosTemp-ResMsgTemp.id44_res.VertPos);
								if(VertPosDelta>1000)
								{
									CAN_SendTableTotal(VERT_TOTAL,VertPosDelta);
									VertPosDelta = 0;
								}
								VertPosTemp = ResMsgTemp.id44_res.VertPos;
							}
					}
					else
					{
						m_VertState = 1;
					}
			}
		}
		if(GlobalState == POSTPASS)
		{
			if(vert_commu_flag!=0)
			{
				errcode |= 0x08;
			}
			else
			{
				errcode &= (~0x08);
			}
		}
		vert_commu_flag =1;
	}
	else
	{
		ResMsgTemp.id44_res.VertPos = 8500;
		ResMsgTemp.id44_res.VertSpeed = 0;
	}
	
	{//读取脚踏开关状态
		if (!FOOT_DOWN)              // foot down active ,low 
		{
 		if(FOOT_UP)                  // foot up inactive ,high
 		  IO_state|=0x01;
 		else
 			IO_state&=0xFE;
	  }
		
		if (!FOOT_UP)         
		{
 		if (FOOT_DOWN)
 		  IO_state|=0x02;
 		else
 			IO_state&=0xFD;
	  }
		
		if(FOOT_XRAY)
			IO_state|=0x04;
		else
			IO_state&=0xFB;
	}
	
	{//读取限位开关的状态
		if(!HORI_LS_LEFT)
			IO_state|=0x10;
		else
			IO_state&=0xEF;
		
		if(!HORI_LS_RIGHT)
			IO_state|=0x20;
		else
			IO_state&=0xDF;
		
		if(!VERT_LS_BOTTOM)
			IO_state|=0x40;
		else
			IO_state&=0xBF;
		
		if(!VERT_LS_TOP)
			IO_state|=0x80;
		else
			IO_state&=0x7F;
	}
	//通过急停判断刹车状态
	{
		if(EMStop>EMStop_before)
		{
			CAN_SendTableTotal(V_BREAK_TOTAL,1);
			EMStop_before = EMStop;
		}
	}
	
	{
		if((errcode&0x33) !=0 )
		{
			hori_commu_flag =1;
			writeSDO(0x01,HoriVert_SDO,8);
			OSTimeDly(100);
			if(hori_commu_flag!=0)
			{
				errcode |= 0x04;
			}
			else
			{
				errcode &= (~0x04);
			}
			ID49.Hori_err =  ReadOD(HORI_ERROR_REG);
			vert_commu_flag =1;
			writeSDO(0x02,HoriVert_SDO,8);
			OSTimeDly(100);
			if(vert_commu_flag!=0)
			{
				errcode |= 0x08;
			}
			else
			{
				errcode &= (~0x08);
			}
			ID49.Vert_err =  ReadOD(VERT_ERROR_REG);
			CAN_SendRespFramePart3(ID49);
		}
		if(GlobalState == POSTPASS)
		{
			if(hori_commu_flag!=0)
			{
				errcode |= 0x04;
			}
			else
			{
				errcode &= (~0x04);
			}
		}	
		hori_commu_flag =1;
	}
	
	{//写入ID 45
		ResMsgTemp.id45_res.GlobalState      		= GlobalState;
		ResMsgTemp.id45_res.Float_State			    = horiFloatingStatus;
		ResMsgTemp.id45_res.EMS_Stop_State			= EMStop;
		ResMsgTemp.id45_res.VertState						= m_VertState;
		ResMsgTemp.id45_res.HoriState						= m_HoriState;
		ResMsgTemp.id45_res.IO_State					  = IO_state;
		ResMsgTemp.id45_res.errcode             = errcode;
	}
	return ResMsgTemp;
}
/***********************************************************/

u32 ReadOD(u8 name)
{
	u8 size;
	u32* data;
	
	switch(name)
	{
		case 0x01 : //read hori state register
		{
			size = 2;
			getODentry(0x6000, 1, (void * *)&data, &size, 0);
			break;
		}
		case 0x02 ://read hori I/O register
		{
			size = 2;
			getODentry(0x6000, 2, (void * *)&data, &size, 0);
			break;
		}
		case 0x03 ://read vert state register
		{
			size = 2;
			getODentry(0x6001, 1, (void * *)&data, &size, 0);
			break;
		}
		case 0x04 ://read vert i/o register
		{
			size = 2;
			getODentry(0x6001, 2, (void * *)&data, &size, 0);
			break;
		}
		case 0x05 ://read hori position counts
		{
			size = 4;
			getODentry(0x6004, 1, (void * *)&data, &size, 0);
			break;
		}
		case 0x06 ://read hori speed counts/s
		{
			size = 4;
			getODentry(0x6004, 2, (void * *)&data, &size, 0);
			break;
		}
		case 0x07 ://read vert position counts
		{
			size = 4;
			getODentry(0x6005, 1, (void * *)&data, &size, 0);
			break;
		}
		case 0x08 ://read vert speed counts/s
		{
			size = 4;
			getODentry(0x6005, 2, (void * *)&data, &size, 0);
			break;
		}
		case 0x09 ://read hori fault reg add by wangqiangqiang (fault code) 2018/6/6
		{
			//size =4;
			//getODentry(0x6002, 1, (void * *)&data, &size, 0);
			data = &Hori_Fault_SDO;
			break;
		}
		case 0x0A://read vert fault reg add by wangqiangqiang (fault code) 2018/6/6
		{
			//size =4;
			//getODentry(0x6003, 1, (void * *)&data, &size, 0);
			data = &Vert_Fault_SDO;
			break;
		}
		default:
			break;
	}
	
	return *data;
}

void writeSDO(u8 nodeId, u8* data, u8 len)
{
	u8 i;
	Message m;
  CanTxMsg canmsg;
	/* message copy for sending */
    m.cob_id.w = 0x600 + nodeId;
    m.rtr = DONNEES; 
    //the length of SDO must be 8
    m.len = len;

	for (i = 0 ; i < len ; i++)
    {
      m.data[i] =  data[i];
    }
    
    CANopen2CAN(&m, &canmsg);
		CAN_Transmit(CAN2, &canmsg);
}

u8 handleMotionDisable (u8 ControllerID, u8 StopMode)
{
	u8  motioncommand[8];
	
	switch (ControllerID)
  {
    case 0x00:
		{
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x5a;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			
			if (StopMode == 0x00)//abrupt to stop
				motioncommand[4] = 0x07;
			else
				motioncommand[4] = 0x05;
			
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x01, 0);

			motioncommand[0] = 0x23;
			motioncommand[1] = 0x84;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x00;
			motioncommand[5] = 0x19;
			motioncommand[6] = 0x00;
			motioncommand[7] = 0x00;
			writeSDO(0x01, motioncommand, 8);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x01, 0);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x85;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x00;
			motioncommand[5] = 0x19;
			motioncommand[6] = 0x00;
			motioncommand[7] = 0x00;
			writeSDO(0x01, motioncommand, 8);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x01, 0);
//			OSTimeDly(10);
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x0b;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x01, 0);
//			OSTimeDly(10);
			break;
		}
		case 0x01:
		{
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x5a;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			
			if (StopMode == 0x00)//abrupt to stop
				motioncommand[4] = 0x07;
			else
				motioncommand[4] = 0x05;
			
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x02, 0);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x85;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x80;
			motioncommand[5] = 0x38;
			motioncommand[6] = 0x01;
			motioncommand[7] = 0x00;
			writeSDO(0x02, motioncommand, 8);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x02, 0);
//			OSTimeDly(10);
				/* set the stop switch and kill the motion */
			//temp = 0x0100;
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x0b;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x02, 0);
//			OSTimeDly(10);

			break;
		}
		default:
			break;
  }
 return 0;
}

u8 handleMotionError (u8 ControllerID)
{
	u8  motioncommand[8];

  switch (ControllerID)
  {
    case 0x00:
		{
	  motioncommand[0] = 0x2b;
		motioncommand[1] = 0x40;
		motioncommand[2] = 0x60;
		motioncommand[3] = 0x00;
		motioncommand[4] = 0x80;
		motioncommand[5] = 0x00;
		writeSDO(0x01, motioncommand, 6);      
	  //err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
	  waitingWriteToSlaveDict(0x01, 0);
	  break;
		}
		case 0x01:
		{
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x80;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);		
			//err  = writeNetworkDict(0, 0x02, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x02, 0x00);
			break;
		}
		default:
			break;
  }
 return 0;
}

u8 ShutOffMotor(u8 ControllerID)
{
	u8  motioncommand[8];
	
	motioncommand[0] = 0x2b;
	motioncommand[1] = 0x40;
	motioncommand[2] = 0x60;
	motioncommand[3] = 0x00;
	motioncommand[4] = 0x04;
	motioncommand[5] = 0x00;

  switch (ControllerID)
  {
    case 0x00:
		{	  
			writeSDO(0x01, motioncommand, 6);      
			waitingWriteToSlaveDict(0x01, 0);
			break;
		}
		case 0x01:
		{
			writeSDO(0x02, motioncommand, 6);		
			waitingWriteToSlaveDict(0x02, 0x00);
			break;
		}
		default:
			break;
  }
 return 0;
}

/*****************************************************************************
 * work as motor control function
 * motorID    : horizontal motor(0x01) or vertical motor(0x02) 
 * operMode   : profile position mode(0x01) or velocity mode(0x03)
 * profileType: actived in position mode(interrupt--0x01, noninterrupt--0x10)
 * *m         : motor motion information(position, velocity, acce, dece)
******************************************************************************/

u8 MotionControl(u8 motorID, u8 operMode, u8 profileType, u32 Position, u32 Speed, u32 Acce, u32 Dece)
{

	u8  motioncommand[8];
	u8  pos8, pos16, pos24, pos32;
	u8  speed8, speed16, speed24, speed32;
	u8  acce8, acce16, acce24, acce32;
	u8  dece8, dece16, dece24, dece32;
	
	pos8  = (u8)Position;
	pos16 = (u8)(Position>>8);
	pos24 = (u8)(Position>>16);
	pos32 = (u8)(Position>>24);
	
	speed8  = (u8)Speed;
	speed16 = (u8)(Speed>>8);
	speed24 = (u8)(Speed>>16);
	speed32 = (u8)(Speed>>24);
	
	acce8  = (u8)Acce;
	acce16 = (u8)(Acce>>8);
	acce24 = (u8)(Acce>>16);
	acce32 = (u8)(Acce>>24);
	
	dece8  = (u8)Dece;
	dece16 = (u8)(Dece>>8);
	dece24 = (u8)(Dece>>16);
	dece32 = (u8)(Dece>>24);
	
	
	switch(operMode)
	{
		case 0x01:
		  {
			  /* set mode of operation by SDO*/
				//temp = 0x01;
				motioncommand[0] = 0x2f;
				motioncommand[1] = 0x60;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = 0x01;
				writeSDO(motorID, motioncommand, 5);
				//err  = writeNetworkDict(0, motorID, 0x6060, 0x00, 0x01, &temp);
				waitingWriteToSlaveDict(motorID, 0);
				/* set motion profile type--Trapezoidal*/
				//os_dly_wait(10);
				//temp = 0x00;
				motioncommand[0] = 0x2b;
				motioncommand[1] = 0x86;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = 0x00;
				motioncommand[5] = 0x00;
				writeSDO(motorID, motioncommand, 6);
				//err  = writeNetworkDict(0, motorID, 0x6086, 0x00, 0x02, &temp);
				waitingWriteToSlaveDict(motorID, 0);
//				OSTimeDly(10);
				/* set target position*/
				motioncommand[0] = 0x23;
				motioncommand[1] = 0x7a;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = pos8;
				motioncommand[5] = pos16;
				motioncommand[6] = pos24;
				motioncommand[7] = pos32;
				writeSDO(motorID, motioncommand, 8);
				//err  = writeNetworkDict(0, motorID, 0x607A, 0x00, 0x04, &Position);
				waitingWriteToSlaveDict(motorID, 0);
				//os_dly_wait(10);
				/* set target velocity*/
				motioncommand[0] = 0x23;
				motioncommand[1] = 0x81;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = speed8;
				motioncommand[5] = speed16;
				motioncommand[6] = speed24;
				motioncommand[7] = speed32;
				writeSDO(motorID, motioncommand, 8);
				//err  = writeNetworkDict(0, motorID, 0x6081, 0x00, 0x04, &Speed);
				waitingWriteToSlaveDict(motorID, 0);
//				OSTimeDly(10);
				/* set profile acceleration*/
				//temp = 0x0FA0;
				motioncommand[0] = 0x23;
				motioncommand[1] = 0x83;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = acce8;
				motioncommand[5] = acce16;
				motioncommand[6] = acce24;
				motioncommand[7] = acce32;
				writeSDO(motorID, motioncommand, 8);
				//err  = writeNetworkDict(0, motorID, 0x6083, 0x00, 0x04, &Acce);
				waitingWriteToSlaveDict(motorID, 0);
				//os_dly_wait(10);
				/* set profile deceleration*/
				motioncommand[0] = 0x23;
				motioncommand[1] = 0x84;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = dece8;
				motioncommand[5] = dece16;
				motioncommand[6] = dece24;
				motioncommand[7] = dece32;
				writeSDO(motorID, motioncommand, 8);
				//err  = writeNetworkDict(0, motorID, 0x6084, 0x00, 0x04, &Dece);
				waitingWriteToSlaveDict(motorID, 0);
//				OSTimeDly(10);				
				
				/* enable controller1*/
				//temp = 0x0f;
				//motioncommand[8] = {0x2b, 0x40, 0x60, 0x00, 0x0f, 0x00};
				motioncommand[0] = 0x2b;
				motioncommand[1] = 0x40;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = 0x0f;
				motioncommand[5] = 0x00;
				writeSDO(motorID, motioncommand, 6);
				//err  = writeNetworkDict(0, motorID, 0x6040, 0x00, 0x02, &temp);
				waitingWriteToSlaveDict(motorID, 0);
				
				if (profileType == 0x10) //select noninterrupt operational mode
				{
					/* start motor*/
					//temp = 0x005F;
					//motioncommand[8] = {0x2b, 0x40, 0x60, 0x00, 0x5f, 0x00};
					motioncommand[0] = 0x2b;
					motioncommand[1] = 0x40;
					motioncommand[2] = 0x60;
					motioncommand[3] = 0x00;
					motioncommand[4] = 0x1f;
					motioncommand[5] = 0x00;
					writeSDO(motorID, motioncommand, 6);
					//err  = writeNetworkDict(0, motorID, 0x6040, 0x00, 0x02, &temp);
					waitingWriteToSlaveDict(motorID, 0);
//					OSTimeDly(10);
				}
				else if (profileType == 0x01) //select interrupt operational mode
				{
					/* enable controller*/
					//temp = 0x007F;
					//motioncommand[8] = {0x2b, 0x40, 0x60, 0x00, 0x7f, 0x00};
					motioncommand[0] = 0x2b;
					motioncommand[1] = 0x40;
					motioncommand[2] = 0x60;
					motioncommand[3] = 0x00;
					motioncommand[4] = 0x3f;
					motioncommand[5] = 0x00;
					writeSDO(motorID, motioncommand, 6);
					//err  = writeNetworkDict(0, motorID, 0x6040, 0x00, 0x02, &temp);
					waitingWriteToSlaveDict(motorID, 0);
//					OSTimeDly(10);
				}
			}
			break;
		case 0x03:
			break;
		default:
			break;
	}
//}
	return 0;
}


void horizontal_moto_contr(u32 horizontal_acce,u32 horizontal_dece,u32 horizontal_speed,u32 horizontal_position)//
{
	double position_float;
	u32 position_desired;
	
	//the reducer ratio is 7:1, the incremental encoder on motor side is 2000 counts/r
	position_float = horizontal_position * 10;//unit 0.01mm
	
	if(position_float > 0)//move toward right side of new origin position
	{
		position_desired = (u32)((double)position_float * (double)horizontal_incr_position_25MM);
		position_desired = 0xffffffff - position_desired + 1;
	}
	else//move toward left side of new origin position
	{		
		position_desired = (u32)((double)(0.0 - position_float) *(double)horizontal_incr_position_25MM);
	}
	
	horizontal_speed = (u32)((double)horizontal_speed*(double)horizontal_encoder_speed_25MM);
	horizontal_acce  = 0x1900;     //单位为0.01mm/s^2，假设设定加速度为200mm/s^2，那么实际配置到驱动器值为20000*0.3202~=6400，即为0x1900
	horizontal_dece  = 0x1900;     //0x2580

	MotionControl(0x01, 0x01, 0x01, position_desired, horizontal_speed, horizontal_acce, horizontal_dece);//80000counts/sec2 ~=35mm/s2, Acce/Dcce ----units: 10counts/sec2

}

void vertical_moto_contr(u32 vertical_speed,u32 vertical_position)//
{
  u32 position_float; //units: degree
	u32 position_desired;
	u32 speed_desired;


	position_float = (u32)((double)30000 * HeightToPoleLength(vertical_position/10));//new vertical motor, unit: counts   /*2015.7.11*/  		
	speed_desired  = (u32)(((double)vertical_speed/(double)235)*(double)200000);//new vertical motor


	if (position_float > vertPoleCounterPowerOn)// table uping
		position_desired = (u32)(position_float - vertPoleCounterPowerOn);                                 //13ACB89
	else// table downing
		position_desired = (u32)(0xffffffff - vertPoleCounterPowerOn + position_float + 1);	 //0xffffffff- 0x13e35a6 = 0xFEC1CA59
	

 	MotionControl(0x02, 0x01, 0x01, position_desired, speed_desired, 0x9c40, 0x9c40);//new vertical motor0x2A30, 0x2A30    0x9c40
}

void setHoriPDOMapping()
{
	/* Configuring the first PDO receive, defined at index 0x1400 and 0x1600 */
  {
    /* 0x6004 stores the hori position(subindex1) value and hori speed(subindex2) value */
		s_mappedVar tabMappedVar[8] = { {0x6004,1,32}, {0x6004,2,32} }; 
    masterMappingPDO(0x1400, 0x381, tabMappedVar, 2);
  }
  /* Configuring the first PDO transmit, defined at index 0x1800 and 0x1A00 */
  {
    s_mappedVar tabMappedVar[8] = { {0x6064,0,32},{0x2231,0,32}};
    slaveMappingPDO(0x01, 0x1803, 0x381, tabMappedVar, 2, TRANS_EVERY_N_SYNC(1));
	}

	/* Configuring the second PDO receive, defined at index 0x1400 and 0x1600 */
  {
    /* 0x6000 stores the hori status word(subindex1) value and hori operation mode(subindex2) value */
		s_mappedVar tabMappedVar[8] = { {0x6000,1,16}, {0x6000,2,16} }; 
    masterMappingPDO(0x1401, 0x281, tabMappedVar, 2);
  }
  /* Configuring the second PDO transmit, defined at index 0x1800 and 0x1A00 */
  {
    s_mappedVar tabMappedVar[8] = { {0x6041,0,16}, {0x2190,0,16} };
    slaveMappingPDO(0x01, 0x1801, 0x281, tabMappedVar, 2, TRANS_EVERY_N_SYNC(2));
  }   
//add by wangqingqiang (add one more PDO trans)	2018/6/6	
	/* Configuring the second PDO receive, defined at index 0x1400 and 0x1600 */
//   {
//     /* 0x6000 stores the hori status word(subindex1) value and hori operation mode(subindex2) value */
// 		s_mappedVar tabMappedVar[8] = { {0x6002,1,32}, {0x6002,2,16} }; 
//     masterMappingPDO(0x1404, 0x181, tabMappedVar, 2);
//   }
//   /* Configuring the second PDO transmit, defined at index 0x1800 and 0x1A00 */
//   {
//     s_mappedVar tabMappedVar[8] = { {0x1002,0,32}, {0x21B4,0,16} };
//     slaveMappingPDO(0x01, 0x1802, 0x181, tabMappedVar, 2, TRANS_EVENT);
//   }
 	
}


	void setVertPDOMapping()
	{
		/* Configuring the first PDO receive, defined at index 0x1400 and 0x1600 */
		{
			/* 0x6005 stores the vert position(subindex1) value and vert speed(subindex2) value */
			s_mappedVar tabMappedVar[8] = { {0x6005,1,32},{0x6005,2,32}};
			masterMappingPDO(0x1402, 0x382, tabMappedVar, 2);
		}
		/* Configuring the first PDO transmit, defined at index 0x1800 and 0x1A00 */
		{
			s_mappedVar tabMappedVar[8] = { {0x6064,0,32},{0x606C,0,32}};
			slaveMappingPDO(0x02, 0x1803, 0x382, tabMappedVar, 2, TRANS_EVERY_N_SYNC(1));
			//slaveMappingPDO(0x02, 0x1802, 0x382, tabMappedVar, 2, TRANS_EVERY_N_SYNC(1));
		}
		
						
		/* Configuring the second PDO receive, defined at index 0x1400 and 0x1600 */
		{
			/* 0x6001 stores the vert status word(subindex1) value and vert operation mode(subindex2) value */
			s_mappedVar tabMappedVar[8] = { {0x6001,1,16}, {0x6001,2,16} }; 
			masterMappingPDO(0x1403, 0x182, tabMappedVar, 2);
		}
		/* Configuring the second PDO transmit, defined at index 0x1800 and 0x1A00 */
		{
			s_mappedVar tabMappedVar[8] = { {0x6041,0,16}, {0x2190,0,16} };
			slaveMappingPDO(0x02, 0x1801, 0x182, tabMappedVar, 2, TRANS_EVERY_N_SYNC(2));
			//slaveMappingPDO(0x02, 0x1800, 0x182, tabMappedVar, 2, TRANS_EVERY_N_SYNC(2));
		}
//add by wangqingqiang (add one more PDO trans)	2018/6/6	
// 		{
// 			/* 0x6000 stores the hori status word(subindex1) value and hori operation mode(subindex2) value */
// 			s_mappedVar tabMappedVar[8] = { {0x6003,1,32}, {0x6003,2,16} }; 
// 			masterMappingPDO(0x1405, 0x282, tabMappedVar, 2);
// 		}
// 		/* Configuring the second PDO transmit, defined at index 0x1800 and 0x1A00 */
// 		{
// 			s_mappedVar tabMappedVar[8] = { {0x1002,0,32}, {0x21B4,0,16} };
// 			slaveMappingPDO(0x02, 0x1802, 0x282, tabMappedVar, 2, TRANS_EVENT);
// 		}
	}


u8 CANopenNetworkInit(u8 type)
{
  u8 i; 
	u8 err = 0xff;
  
  /* arrays initialisation, etc */
  initCANopenMain();

  /* arrays initialisation, etc */
  initCANopenMaster();  

  /* Defining the node Id */
  setNodeId(0x03);

  /* Put the master in operational mode */
  setState(Operational);

  /* Init the table of connected nodes */
  for (i = 0 ; i < 3 ; i++)
    connectedNode[i] = 0;  
  
  /* Whose node have answered ? */
  {
		/* Sending a request Node guard to node 1 */
		masterReadNodeState(0, 0x01);
		/* Waiting for a second the response */
    OSTimeDly(100);
		connectedNode[1] = stateNode(1);
		
		OSSemPend(SemHoriCommunication,0x2710,&err);
		if (err == OS_ERR_NONE)
		{
			HoriCommunicateOK = 0xff;
			/* Configure the SDO master to communicate with node 1 */
			configure_master_SDO(0x1280, 0x01);
			/* Configure the PDO to communicate with node 1 */
			setHoriPDOMapping();
		}
		else
			HoriCommunicateOK = 0xfe;     // 11111110
		OSTimeDly(10);
  }
 
	if(type != 2)
  {
		/* Sending a message to the node 2, only as example */
		masterReadNodeState(0, 0x02);
		/* Waiting for a second the response */
		OSTimeDly(100);
		connectedNode[2] = stateNode(2);
		err = 0xff;
		OSSemPend(SemVertCommunication,0x2710,&err);
		if (err == OS_ERR_NONE)
		{
			VertCommunicateOK = 0xff;
			/* Configure the SDO master to communicate with node 2 */
			configure_master_SDO(0x1281, 0x02);
			/* Configure the PDO to communicate with node 2 */
			setVertPDOMapping();			
		}
		else
			VertCommunicateOK = 0xfd;         // 11111101
  }
	else
	{
		VertCommunicateOK = 0xff;
	}

	if(HoriCommunicateOK == 0xFF && VertCommunicateOK ==0xff)
	{
		//set CAN network synch time
		slaveSYNCEnable(0x01, 0x80, 0x2710, 0x01);
		
		/* start all nodes into operational mode */
		masterSendNMTstateChange(0, 0x00, NMT_Start_Node);
	}
	
	return (HoriCommunicateOK & VertCommunicateOK);
  /******************** END CONFIGURING THE NETWORK **********************/
}

u8 HomeMotion(u8 MotorID, u8 MotionDirection, u32 MotionSpeed, u32 MotionAcce)
{
	u8  motioncommand[8];
	u8  ReachHomeOK = 0x00;
	u8  err = 0xff;
	u8  speed0, speed1, speed2, speed3;
	u8  acce0, acce1, acce2, acce3;
	
	speed0 = (u8) MotionSpeed;
	speed1 = (u8) (MotionSpeed >> 8);
	speed2 = (u8) (MotionSpeed >> 16);
	speed3 = (u8) (MotionSpeed >> 24);
	
	acce0  = (u8)  MotionAcce;
	acce1  = (u8)  (MotionAcce >> 8);
	acce2  = (u8)  (MotionAcce >> 16);
	acce3  = (u8)  (MotionAcce >> 24);
	
  switch(MotorID)
  {
    case 0x00:
		{
			motioncommand[0] = 0x2f;
			motioncommand[1] = 0x60;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x06;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			waitingWriteToSlaveDict(0x01, 0);
				
			motioncommand[0] = 0x2f;
			motioncommand[1] = 0x98;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = MotionDirection;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			waitingWriteToSlaveDict(0x01, 0);
			
//			OSTimeDly(10);
				
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x99;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x01;
			motioncommand[4] = speed0;
			motioncommand[5] = speed1;
			motioncommand[6] = speed2;
			motioncommand[7] = speed3;
			writeSDO(0x01, motioncommand, 8);      
			waitingWriteToSlaveDict(0x01, 0);
					
//			OSTimeDly(10);
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x99;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x02;
			motioncommand[4] = 0xd2;
			motioncommand[5] = 0x4e;
			motioncommand[6] = 0x01;
			motioncommand[7] = 0x00;
			writeSDO(0x01, motioncommand, 8);      
			waitingWriteToSlaveDict(0x01, 0);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x9a;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = acce0;
			motioncommand[5] = acce1;
			motioncommand[6] = acce2;
			motioncommand[7] = acce3;
			writeSDO(0x01, motioncommand, 8);      
			waitingWriteToSlaveDict(0x01, 0);		
			
			
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x0f;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);
			waitingWriteToSlaveDict(0x01, 0);
			
//			OSTimeDly(10);
			
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x1f;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			waitingWriteToSlaveDict(0x01, 0);
			
//			OSTimeDly(10);
			
			HoriFirstPosInit = 0x01;
			OSSemPend(SemHoriReachHome,0xFDE8,&err);
	
			if (err == OS_ERR_NONE)
			{
				ReachHomeOK = 0xff;
				home_reached = 0;
				OSTimeDly(3);     //2015.8.12
				home_reached = 1;
				OSTimeDly(3);     //2015.8.12
				home_reached = 0;
			}
			else
				ReachHomeOK = 0xfe;
			break;
		}
		case 0x01:
		{
			motioncommand[0] = 0x2f;
			motioncommand[1] = 0x60;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x06;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			waitingWriteToSlaveDict(0x02, 0);
				
			motioncommand[0] = 0x2f;
			motioncommand[1] = 0x98;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = MotionDirection;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
				
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x99;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x01;
			motioncommand[4] = speed0;
			motioncommand[5] = speed1;
			motioncommand[6] = speed2;
			motioncommand[7] = speed3;
			writeSDO(0x02, motioncommand, 8);      
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x99;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x02;
			motioncommand[4] = speed0;
			motioncommand[5] = speed1;
			motioncommand[6] = speed2;
			motioncommand[7] = speed3;
			writeSDO(0x02, motioncommand, 8);      
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x9a;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = acce0;
			motioncommand[5] = acce1;
			motioncommand[6] = acce2;
			motioncommand[7] = acce3;
			writeSDO(0x02, motioncommand, 8);      
			waitingWriteToSlaveDict(0x02, 0);
			
						
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x0f;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
			
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x1f;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
			
			VertFirstPosInit = 0x01;
			OSSemPend(SemVertReachHome,0xFDE8,&err);
	
			if (err == OS_ERR_NONE)
			{
				ReachHomeOK = 0xff;
				home_reached = 0;
				OSTimeDly(3);      //2015.8.12
				home_reached = 1;
				OSTimeDly(3);      //2015.8.12
				home_reached = 0;
				
			}
			else
				ReachHomeOK = 0xfd;
			
			break;
		}
		default:
			break;
  }	
	
	return ReachHomeOK;
}

u8 TablePosInit(u8 HomeNumber)//put the hori and vert in home motion mode
{
	u8 HoriPosInitResult, VertPosInitResult;
	
	if(HomeNumber==0x01)
		vertPoleCounterPowerOn = 0x12135a5;//0x12135a5高度为880cm;
	else
		vertPoleCounterPowerOn = 0x13AF095;//0x13AF095高度为990cm;//0x13E35A6;//0x1ac7788//0x13ACC95;
	
	switch(HomeNumber)
	{
		case 0x00:
		case 0x01:
		case 0x03:
			H_CLUTCH = SET_H_CLUTCH;
			H_BRAKE  = CLR_H_BRAKE;
			V_BRAKE = CLR_V_BRAKE;
		  
			handleMotionError(0);
			OSTimeDly(10);
			handleMotionError(1);
			OSTimeDly(20);
		
			HoriPosInitResult = HomeMotion(0x00, 0x13, 0x14ed2, 0x11f40); 
			VertPosInitResult = HomeMotion(0x01, 0x13, 0xbec78, 0x1fa0);		
			break;
		case 0x02:
			H_CLUTCH = SET_H_CLUTCH;
			H_BRAKE  = CLR_H_BRAKE;
		
			handleMotionError(0);
			OSTimeDly(10);
			
			HoriPosInitResult = HomeMotion(0x00, 0x13, 0x14ed2, 0x11f40);
			VertPosInitResult = 0xFF;
			break;
	}
	return (HoriPosInitResult & VertPosInitResult);
}

u8 Post(u8 Post_Type)
{
	u8  TestNetResult, TestPosInitResult;
	
	/*Test the communication to drivers*/
	TestNetResult = CANopenNetworkInit(Post_Type);
	/*Homing of horizontal and vertical*/
	if((TestNetResult&0x01)==0xFE)//水平失败
	{
		TestPosInitResult = 0xFC;
	}
	else if(TestNetResult==0xFD)//水平成功，垂直失败,LC16 LOW
	{
		model_type = 0x02;
		TestPosInitResult = TablePosInit(0x02);
	}
	else if(TestNetResult==0xFF)//都成功，LC16 HIGH
	{
		model_type = 0x01;
		TestPosInitResult = TablePosInit(0x01);
	}

	return (((TestPosInitResult<<2) | 0xf3) & TestNetResult);
}


=======
  /******************************************************************************
  * @file    bsp.c
  * @author  Vic. WangQingqiang
  * @version V2.0.0
  * @date    2018.3.26
  * @brief   This file provides bed controller hardware related functions.
  ******************************************************************************/
#include "can.h"
#include "bsp.h"
#include "table_gpio.h"
#include "math.h"
#include "objacces.h"
#include "nmtMaster.h"
#include "sdo.h"
#include "init.h"
#include "ucos_ii.h"														
extern OS_EVENT  * QueueCAN1ISRtoTSKSC;    //2015.8.4
extern OS_EVENT * SemHoriCommunication;
extern OS_EVENT * SemVertCommunication;
extern OS_EVENT * SemHoriReachHome;
extern OS_EVENT * SemVertReachHome;
extern GLOBALSTATE  		GlobalState;
extern u8 CAN1_TX_ok;
extern volatile u8 EMStop;
extern u8 post_flag;
extern u8 vertWriteCommandFlag;
extern u8 horiWriteCommandFlag;
extern u8 horiMotorStatusFlg;
extern u8 vertMotorStatusFlg;
extern u8 HoriDisableTrigger;
extern u8 VertDisableTrigger;
extern u8 HoriFirstPosInit ;
extern u8 VertFirstPosInit;
extern u32 Hori_Fault_SDO;
extern u32 Vert_Fault_SDO;
extern u8 model_type;

u32 vertPoleCounterPowerOn = 0x13AF095;//0x13E35A6;//0x1ac7788//0x13ACC95;
u8  horiFloatingStatus=0;
u8  HoriCommunicateOK;
u8  VertCommunicateOK;
u8 connectedNode[3];

u8 hori_commu_flag=0;
u8 vert_commu_flag=0;

volatile u8 m_HoriState=0;
volatile u8 m_VertState=0;

static u16 HoriPosTemp=0;
static u16 VertPosTemp=9900;
static u32 HoriPosDelta=0;
static u32 VertPosDelta=0;
static u16 errcode;
static u8  EMStop_before;
u32 GetCurrentPoleCount(void)               //获得当前推杆的相对编码器counter 值，并加上上电初始偏移量
{
	u32 curPoleCount;
	curPoleCount = ReadOD(VERT_POS_REG);
	curPoleCount += vertPoleCounterPowerOn;
	
	return (curPoleCount);
};

double HeightToPoleLength(double Height)   //根据高度计算推杆长度，返回值mm ,公式提供：孙雪超
{
	double y;
	y= sqrt(530489.0921 - 327579.8355 * cos(asin((Height - 315.0)/780.00) + 0.34821));
	return y;
}

double PoleLengthToHeight(double polelength)   //根据推杆长度计算高度，返回值mm ,公式提供：孙雪超
{
	double y;
	y = 780.00 * sin(acos(1.6194-0.00000305*polelength*polelength)-0.348172) + 315.0;
	return y;
}

u32 GetCurrentHorizontalPosition(void)  // return value in 0.01 mm
{
	u32 curHorizontalCount;

	curHorizontalCount = ReadOD(HORI_POS_REG);
	
	if(curHorizontalCount>0xFFF0BDC0)   //如果大于-1000000counts，正常范围是4800~-604800
	{
		curHorizontalCount = (u32)((double)(0xffffffff - curHorizontalCount + 1)/(double)horizontal_incr_position_25MM);
	}
	else
	{
		curHorizontalCount = (u32)((double)curHorizontalCount/(double)horizontal_incr_position_25MM);
	}
	
	return (u32)(curHorizontalCount);	
}

void Judge_Gloab_State(u8 PostResult)
{
	  if(post_flag ==0x01)
		{
				if (PostResult == 0xff)
				{
					GlobalState = POSTPASS;
					errcode &=0xFFFFFFF0;
				}
				else if ((PostResult&0x0F)^0x0F)
				{	
					GlobalState = POSTFAIL;
					errcode &=0xFFFFFFF0;
					if(((~PostResult)>>3)&0x01)
					{
						errcode |= 0x02;
					}
					if(((~PostResult)>>2)&0x01)
					{
						errcode |= 0x01;
					}
					if(((~PostResult)>>1)&0x01)
					{
						errcode |= 0x02;
					}
					if(((~PostResult)>>0)&0x01)
					{
						errcode |= 0x01;
					}
				}
				else
				{
					GlobalState = POST;	
					errcode &=0xFFFFFFF0;
				}
		}
		else
		{
			GlobalState = ABNORMAL;	
		}
					
}
	
void CAN_SendRespFramePart1(ID_44 message)
{
	u8 can_cnt=0;
  CanTxMsg TxMessage;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 8;
  TxMessage.StdId = 0x44;
  TxMessage.Data[0] = (u8)(message.HoriPos&0xFF);
  TxMessage.Data[1] = (u8)((message.HoriPos>>8)&0xFF);
  TxMessage.Data[2] = (u8)(message.HoriSpeed&0xFF);
  TxMessage.Data[3] = (u8)((message.HoriSpeed>>8)&0xFF);
  TxMessage.Data[4] = (u8)(message.VertPos&0xFF);
  TxMessage.Data[5] = (u8)((message.VertPos>>8)&0xFF);
  TxMessage.Data[6] = (u8)(message.VertSpeed&0xFF);
  TxMessage.Data[7] = (u8)((message.VertSpeed>>8)&0xFF);
  do
	{
		CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
		OSTimeDly(1);
		if(can_cnt==200)
			break;
		else
			can_cnt++;
	}
	while (!CAN1_TX_ok);
}

void CAN_SendRespFramePart2(ID_45 message)
{
	u8 can_cnt=0;
  CanTxMsg TxMessage;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 8;
  TxMessage.StdId = 0x45;
	
  TxMessage.Data[0] = message.GlobalState;
  TxMessage.Data[1] = message.HoriState;
  TxMessage.Data[2] = message.VertState;
  TxMessage.Data[3] = message.Float_State;
  TxMessage.Data[4] = message.EMS_Stop_State;
  TxMessage.Data[5] = message.IO_State;
  TxMessage.Data[6] = message.errcode&0xFF;
  TxMessage.Data[7] = (message.errcode>>8)&0xFF;
  do
	{
		CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
		OSTimeDly(1);
		if(can_cnt==200)
			break;
		else
			can_cnt++;
	}
	while (!CAN1_TX_ok);
}

void CAN_SendRespFramePart3(ID_49 message)
{
		u8 can_cnt=0;
  CanTxMsg TxMessage;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 8;
  TxMessage.StdId = 0x49;
	
  TxMessage.Data[0] = message.Hori_err&0xFF;
  TxMessage.Data[1] = (message.Hori_err>>8)&0xFF;
  TxMessage.Data[2] = (message.Hori_err>>16)&0xFF;
  TxMessage.Data[3] = (message.Hori_err>>24)&0xFF;
  TxMessage.Data[4] = message.Vert_err;
  TxMessage.Data[5] = (message.Vert_err>>8)&0xFF;
  TxMessage.Data[6] = (message.Vert_err>>16)&0xFF;
  TxMessage.Data[7] = (message.Vert_err>>24)&0xFF;
  do
	{
		CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
		OSTimeDly(1);
		if(can_cnt==200)
			break;
		else
			can_cnt++;
	}
	while (!CAN1_TX_ok);
}

void HandleRespData(RES_TABLE_STATE msg)
{
	CAN_SendRespFramePart1(msg.id44_res);
	CAN_SendRespFramePart2(msg.id45_res);
}

void CAN_SendTableTotal(u8 type,u32 delta)
{
	u8 can_cnt=0;
	CanTxMsg TxMessage;
  TxMessage.RTR = CAN_RTR_DATA;
  TxMessage.IDE = CAN_ID_STD;
  TxMessage.DLC = 5;
	TxMessage.StdId = 0x48;
	
	if(type<4)
	{
		TxMessage.Data[0] = delta&0xFF;
		TxMessage.Data[1] = (delta>>8)&0xFF;
		TxMessage.Data[2] = (delta>>16)&0xFF;
		TxMessage.Data[3] = (delta>>24)&0xFF;
		TxMessage.Data[4] = type;
		do
		{
			CAN_TransmitStatus(CAN1, CAN_Transmit(CAN1, &TxMessage));
			OSTimeDly(1);
			if(can_cnt==200)
				break;
			else
				can_cnt++;
		}
		while (!CAN1_TX_ok);
	}
}

/***********************************************************/
RES_TABLE_STATE ResponseData(u8 type)
{
	u32 curVertSpeed;
	u32 curVertStandardSpeed;
	u32 curHoriSpeed;
	u32 curHoriStandardSpeed;
	u8 IO_state;
	u16 horiMotorStatus=0;
	u16 vertMotorStatus;
	u8 horiMotionStatus=0;
	u8 vertMotionStatus=0;

	u32 RealVertCounts = 0;
	u8 HoriVert_SDO[8] = {0x40,0x01,0x10,0x00,0x00,0x00,0x00,0x00};
	ID_49 ID49;

	RES_TABLE_STATE ResMsgTemp;
	double motor_pos_offset;
	
	
	{//读取水平驱动器位置,单位0.1mm
		ResMsgTemp.id44_res.HoriPos = (u16)(GetCurrentHorizontalPosition()/10);
		//读取水平驱动器速度,单位0.01mm/s
		curHoriSpeed = ReadOD(HORI_SPEED_REG);
		if (curHoriSpeed > 0x80000000)//若是一个负数
		{
			curHoriStandardSpeed = 0xffffffff - curHoriSpeed + 1;
		}
		else
			curHoriStandardSpeed = curHoriSpeed;
		
		ResMsgTemp.id44_res.HoriSpeed = (u16)((double)curHoriStandardSpeed/(double)horizontal_encoder_speed_25MM);
	}
	
	{//读取当前水平电机是否出错
		horiMotorStatus = ReadOD(HORI_STATUS_REG);		
		if(((horiMotorStatus)>> 3) & 0x01)
		{//出错了
			errcode &=0xFFFFFFEF;
			errcode |=0x00000010;
		}
		else
		{
			errcode &=0xFFFFFFEF;
		}	
	}
	
	{//读取当前水平电机状态，0=停止，1=运动
			//horiMotionStatus在是在发送一次命令之后并且运动到位才有效
		horiMotionStatus = (horiMotorStatus)>>10 & 0x01;
			//horiMotorStatusFlg是在发送给驱动器位置，速度等参数时置1，在驱动器接受完之后置0。
		if(horiMotorStatusFlg==0)      
		{
				//满足4个条件之一表示运动停止。1.运动完成 2.有停止命令发送给驱动器 3.碰到左限位开关 4.碰到右限位开关
				if(horiMotionStatus||!HORI_LS_LEFT||!HORI_LS_RIGHT)
				{
						m_HoriState = 0;
						//HoriDisableTrigger清零表示可以接收下一次停止命令
						HoriDisableTrigger = 0;
						//horiWriteCommandFlag清零表示可以接收下一次命令
						horiWriteCommandFlag =0;
						if(GlobalState==POSTPASS)
						{
							HoriPosDelta += abs(HoriPosTemp-ResMsgTemp.id44_res.HoriPos);
							if(HoriPosDelta>1000)
							{
								CAN_SendTableTotal(HORI_TOTAL,HoriPosDelta);
								HoriPosDelta = 0;
							}
							HoriPosTemp = ResMsgTemp.id44_res.HoriPos;
						}
				}
				else
				{
					m_HoriState = 1;
				}
		}
	}
	
	if(type!=0x02)
	{
		{//读取当前垂直电机是否出错
			vertMotorStatus = ReadOD(VERT_STATUS_REG);	
			if(((vertMotorStatus)>> 3) & 0x01)
			{
				errcode &=0xFFFFFFDF;
				errcode |=0x00000020;
			}
			else
			{
				errcode &=0xFFFFFFDF;
			}
		}
		
		{//读取垂直驱动器位置速度
			//读取垂直驱动器位置，单位0.1mm
			RealVertCounts   =  GetCurrentPoleCount();
			motor_pos_offset = PoleLengthToHeight(((double)RealVertCounts)/(double)30000);

			ResMsgTemp.id44_res.VertPos = (u32)(motor_pos_offset*10);
			//读取垂直驱动器速度
			curVertSpeed = ReadOD(VERT_SPEED_REG);
			if (curVertSpeed > 0x900000)//   31mm/s 一般速度10mm/s
			{
				curVertStandardSpeed = 0xffffffff - curVertSpeed + 1;
			}
			else
				curVertStandardSpeed = curVertSpeed;
			
			ResMsgTemp.id44_res.VertSpeed = (u16)((((double)curVertStandardSpeed)/(double)200000)*(double)235);//new vertical motor
		}
		
		{//读取当前垂直电机状态，0=停止，1=运动
			vertMotionStatus = (vertMotorStatus)>>10 & 0x01;
			if(vertMotorStatusFlg==0)      
			{
					if(vertMotionStatus||!VERT_LS_TOP||!VERT_LS_BOTTOM)
					{
							m_VertState = 0;
							VertDisableTrigger = 0;
							vertWriteCommandFlag =0;
							if(GlobalState==POSTPASS)
							{
								VertPosDelta += abs(VertPosTemp-ResMsgTemp.id44_res.VertPos);
								if(VertPosDelta>1000)
								{
									CAN_SendTableTotal(VERT_TOTAL,VertPosDelta);
									VertPosDelta = 0;
								}
								VertPosTemp = ResMsgTemp.id44_res.VertPos;
							}
					}
					else
					{
						m_VertState = 1;
					}
			}
		}
		if(GlobalState == POSTPASS)
		{
			if(vert_commu_flag!=0)
			{
				errcode |= 0x08;
			}
			else
			{
				errcode &= (~0x08);
			}
		}
		vert_commu_flag =1;
	}
	else
	{
		ResMsgTemp.id44_res.VertPos = 8500;
		ResMsgTemp.id44_res.VertSpeed = 0;
	}
	
	{//读取脚踏开关状态
		if (!FOOT_DOWN)              // foot down active ,low 
		{
 		if(FOOT_UP)                  // foot up inactive ,high
 		  IO_state|=0x01;
 		else
 			IO_state&=0xFE;
	  }
		
		if (!FOOT_UP)         
		{
 		if (FOOT_DOWN)
 		  IO_state|=0x02;
 		else
 			IO_state&=0xFD;
	  }
		
		if(FOOT_XRAY)
			IO_state|=0x04;
		else
			IO_state&=0xFB;
	}
	
	{//读取限位开关的状态
		if(!HORI_LS_LEFT)
			IO_state|=0x10;
		else
			IO_state&=0xEF;
		
		if(!HORI_LS_RIGHT)
			IO_state|=0x20;
		else
			IO_state&=0xDF;
		
		if(!VERT_LS_BOTTOM)
			IO_state|=0x40;
		else
			IO_state&=0xBF;
		
		if(!VERT_LS_TOP)
			IO_state|=0x80;
		else
			IO_state&=0x7F;
	}
	//通过急停判断刹车状态
	{
		if(EMStop>EMStop_before)
		{
			CAN_SendTableTotal(V_BREAK_TOTAL,1);
			EMStop_before = EMStop;
		}
	}
	
	{
		if((errcode&0x33) !=0 )
		{
			hori_commu_flag =1;
			writeSDO(0x01,HoriVert_SDO,8);
			OSTimeDly(100);
			if(hori_commu_flag!=0)
			{
				errcode |= 0x04;
			}
			else
			{
				errcode &= (~0x04);
			}
			ID49.Hori_err =  ReadOD(HORI_ERROR_REG);
			vert_commu_flag =1;
			writeSDO(0x02,HoriVert_SDO,8);
			OSTimeDly(100);
			if(vert_commu_flag!=0)
			{
				errcode |= 0x08;
			}
			else
			{
				errcode &= (~0x08);
			}
			ID49.Vert_err =  ReadOD(VERT_ERROR_REG);
			CAN_SendRespFramePart3(ID49);
		}
		if(GlobalState == POSTPASS)
		{
			if(hori_commu_flag!=0)
			{
				errcode |= 0x04;
			}
			else
			{
				errcode &= (~0x04);
			}
		}	
		hori_commu_flag =1;
	}
	
	{//写入ID 45
		ResMsgTemp.id45_res.GlobalState      		= GlobalState;
		ResMsgTemp.id45_res.Float_State			    = horiFloatingStatus;
		ResMsgTemp.id45_res.EMS_Stop_State			= EMStop;
		ResMsgTemp.id45_res.VertState						= m_VertState;
		ResMsgTemp.id45_res.HoriState						= m_HoriState;
		ResMsgTemp.id45_res.IO_State					  = IO_state;
		ResMsgTemp.id45_res.errcode             = errcode;
	}
	return ResMsgTemp;
}
/***********************************************************/

u32 ReadOD(u8 name)
{
	u8 size;
	u32* data;
	
	switch(name)
	{
		case 0x01 : //read hori state register
		{
			size = 2;
			getODentry(0x6000, 1, (void * *)&data, &size, 0);
			break;
		}
		case 0x02 ://read hori I/O register
		{
			size = 2;
			getODentry(0x6000, 2, (void * *)&data, &size, 0);
			break;
		}
		case 0x03 ://read vert state register
		{
			size = 2;
			getODentry(0x6001, 1, (void * *)&data, &size, 0);
			break;
		}
		case 0x04 ://read vert i/o register
		{
			size = 2;
			getODentry(0x6001, 2, (void * *)&data, &size, 0);
			break;
		}
		case 0x05 ://read hori position counts
		{
			size = 4;
			getODentry(0x6004, 1, (void * *)&data, &size, 0);
			break;
		}
		case 0x06 ://read hori speed counts/s
		{
			size = 4;
			getODentry(0x6004, 2, (void * *)&data, &size, 0);
			break;
		}
		case 0x07 ://read vert position counts
		{
			size = 4;
			getODentry(0x6005, 1, (void * *)&data, &size, 0);
			break;
		}
		case 0x08 ://read vert speed counts/s
		{
			size = 4;
			getODentry(0x6005, 2, (void * *)&data, &size, 0);
			break;
		}
		case 0x09 ://read hori fault reg add by wangqiangqiang (fault code) 2018/6/6
		{
			//size =4;
			//getODentry(0x6002, 1, (void * *)&data, &size, 0);
			data = &Hori_Fault_SDO;
			break;
		}
		case 0x0A://read vert fault reg add by wangqiangqiang (fault code) 2018/6/6
		{
			//size =4;
			//getODentry(0x6003, 1, (void * *)&data, &size, 0);
			data = &Vert_Fault_SDO;
			break;
		}
		default:
			break;
	}
	
	return *data;
}

void writeSDO(u8 nodeId, u8* data, u8 len)
{
	u8 i;
	Message m;
  CanTxMsg canmsg;
	/* message copy for sending */
    m.cob_id.w = 0x600 + nodeId;
    m.rtr = DONNEES; 
    //the length of SDO must be 8
    m.len = len;

	for (i = 0 ; i < len ; i++)
    {
      m.data[i] =  data[i];
    }
    
    CANopen2CAN(&m, &canmsg);
		CAN_Transmit(CAN2, &canmsg);
}

u8 handleMotionDisable (u8 ControllerID, u8 StopMode)
{
	u8  motioncommand[8];
	
	switch (ControllerID)
  {
    case 0x00:
		{
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x5a;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			
			if (StopMode == 0x00)//abrupt to stop
				motioncommand[4] = 0x07;
			else
				motioncommand[4] = 0x05;
			
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x01, 0);

			motioncommand[0] = 0x23;
			motioncommand[1] = 0x84;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x00;
			motioncommand[5] = 0x19;
			motioncommand[6] = 0x00;
			motioncommand[7] = 0x00;
			writeSDO(0x01, motioncommand, 8);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x01, 0);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x85;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x00;
			motioncommand[5] = 0x19;
			motioncommand[6] = 0x00;
			motioncommand[7] = 0x00;
			writeSDO(0x01, motioncommand, 8);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x01, 0);
//			OSTimeDly(10);
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x0b;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x01, 0);
//			OSTimeDly(10);
			break;
		}
		case 0x01:
		{
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x5a;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			
			if (StopMode == 0x00)//abrupt to stop
				motioncommand[4] = 0x07;
			else
				motioncommand[4] = 0x05;
			
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x02, 0);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x85;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x80;
			motioncommand[5] = 0x38;
			motioncommand[6] = 0x01;
			motioncommand[7] = 0x00;
			writeSDO(0x02, motioncommand, 8);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x02, 0);
//			OSTimeDly(10);
				/* set the stop switch and kill the motion */
			//temp = 0x0100;
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x0b;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			//err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x02, 0);
//			OSTimeDly(10);

			break;
		}
		default:
			break;
  }
 return 0;
}

u8 handleMotionError (u8 ControllerID)
{
	u8  motioncommand[8];

  switch (ControllerID)
  {
    case 0x00:
		{
	  motioncommand[0] = 0x2b;
		motioncommand[1] = 0x40;
		motioncommand[2] = 0x60;
		motioncommand[3] = 0x00;
		motioncommand[4] = 0x80;
		motioncommand[5] = 0x00;
		writeSDO(0x01, motioncommand, 6);      
	  //err  = writeNetworkDict(0, 0x01, 0x6040, 0x00, 0x02, &temp);
	  waitingWriteToSlaveDict(0x01, 0);
	  break;
		}
		case 0x01:
		{
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x80;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);		
			//err  = writeNetworkDict(0, 0x02, 0x6040, 0x00, 0x02, &temp);
			waitingWriteToSlaveDict(0x02, 0x00);
			break;
		}
		default:
			break;
  }
 return 0;
}

u8 ShutOffMotor(u8 ControllerID)
{
	u8  motioncommand[8];
	
	motioncommand[0] = 0x2b;
	motioncommand[1] = 0x40;
	motioncommand[2] = 0x60;
	motioncommand[3] = 0x00;
	motioncommand[4] = 0x04;
	motioncommand[5] = 0x00;

  switch (ControllerID)
  {
    case 0x00:
		{	  
			writeSDO(0x01, motioncommand, 6);      
			waitingWriteToSlaveDict(0x01, 0);
			break;
		}
		case 0x01:
		{
			writeSDO(0x02, motioncommand, 6);		
			waitingWriteToSlaveDict(0x02, 0x00);
			break;
		}
		default:
			break;
  }
 return 0;
}

/*****************************************************************************
 * work as motor control function
 * motorID    : horizontal motor(0x01) or vertical motor(0x02) 
 * operMode   : profile position mode(0x01) or velocity mode(0x03)
 * profileType: actived in position mode(interrupt--0x01, noninterrupt--0x10)
 * *m         : motor motion information(position, velocity, acce, dece)
******************************************************************************/

u8 MotionControl(u8 motorID, u8 operMode, u8 profileType, u32 Position, u32 Speed, u32 Acce, u32 Dece)
{

	u8  motioncommand[8];
	u8  pos8, pos16, pos24, pos32;
	u8  speed8, speed16, speed24, speed32;
	u8  acce8, acce16, acce24, acce32;
	u8  dece8, dece16, dece24, dece32;
	
	pos8  = (u8)Position;
	pos16 = (u8)(Position>>8);
	pos24 = (u8)(Position>>16);
	pos32 = (u8)(Position>>24);
	
	speed8  = (u8)Speed;
	speed16 = (u8)(Speed>>8);
	speed24 = (u8)(Speed>>16);
	speed32 = (u8)(Speed>>24);
	
	acce8  = (u8)Acce;
	acce16 = (u8)(Acce>>8);
	acce24 = (u8)(Acce>>16);
	acce32 = (u8)(Acce>>24);
	
	dece8  = (u8)Dece;
	dece16 = (u8)(Dece>>8);
	dece24 = (u8)(Dece>>16);
	dece32 = (u8)(Dece>>24);
	
	
	switch(operMode)
	{
		case 0x01:
		  {
			  /* set mode of operation by SDO*/
				//temp = 0x01;
				motioncommand[0] = 0x2f;
				motioncommand[1] = 0x60;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = 0x01;
				writeSDO(motorID, motioncommand, 5);
				//err  = writeNetworkDict(0, motorID, 0x6060, 0x00, 0x01, &temp);
				waitingWriteToSlaveDict(motorID, 0);
				/* set motion profile type--Trapezoidal*/
				//os_dly_wait(10);
				//temp = 0x00;
				motioncommand[0] = 0x2b;
				motioncommand[1] = 0x86;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = 0x00;
				motioncommand[5] = 0x00;
				writeSDO(motorID, motioncommand, 6);
				//err  = writeNetworkDict(0, motorID, 0x6086, 0x00, 0x02, &temp);
				waitingWriteToSlaveDict(motorID, 0);
//				OSTimeDly(10);
				/* set target position*/
				motioncommand[0] = 0x23;
				motioncommand[1] = 0x7a;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = pos8;
				motioncommand[5] = pos16;
				motioncommand[6] = pos24;
				motioncommand[7] = pos32;
				writeSDO(motorID, motioncommand, 8);
				//err  = writeNetworkDict(0, motorID, 0x607A, 0x00, 0x04, &Position);
				waitingWriteToSlaveDict(motorID, 0);
				//os_dly_wait(10);
				/* set target velocity*/
				motioncommand[0] = 0x23;
				motioncommand[1] = 0x81;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = speed8;
				motioncommand[5] = speed16;
				motioncommand[6] = speed24;
				motioncommand[7] = speed32;
				writeSDO(motorID, motioncommand, 8);
				//err  = writeNetworkDict(0, motorID, 0x6081, 0x00, 0x04, &Speed);
				waitingWriteToSlaveDict(motorID, 0);
//				OSTimeDly(10);
				/* set profile acceleration*/
				//temp = 0x0FA0;
				motioncommand[0] = 0x23;
				motioncommand[1] = 0x83;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = acce8;
				motioncommand[5] = acce16;
				motioncommand[6] = acce24;
				motioncommand[7] = acce32;
				writeSDO(motorID, motioncommand, 8);
				//err  = writeNetworkDict(0, motorID, 0x6083, 0x00, 0x04, &Acce);
				waitingWriteToSlaveDict(motorID, 0);
				//os_dly_wait(10);
				/* set profile deceleration*/
				motioncommand[0] = 0x23;
				motioncommand[1] = 0x84;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = dece8;
				motioncommand[5] = dece16;
				motioncommand[6] = dece24;
				motioncommand[7] = dece32;
				writeSDO(motorID, motioncommand, 8);
				//err  = writeNetworkDict(0, motorID, 0x6084, 0x00, 0x04, &Dece);
				waitingWriteToSlaveDict(motorID, 0);
//				OSTimeDly(10);				
				
				/* enable controller1*/
				//temp = 0x0f;
				//motioncommand[8] = {0x2b, 0x40, 0x60, 0x00, 0x0f, 0x00};
				motioncommand[0] = 0x2b;
				motioncommand[1] = 0x40;
				motioncommand[2] = 0x60;
				motioncommand[3] = 0x00;
				motioncommand[4] = 0x0f;
				motioncommand[5] = 0x00;
				writeSDO(motorID, motioncommand, 6);
				//err  = writeNetworkDict(0, motorID, 0x6040, 0x00, 0x02, &temp);
				waitingWriteToSlaveDict(motorID, 0);
				
				if (profileType == 0x10) //select noninterrupt operational mode
				{
					/* start motor*/
					//temp = 0x005F;
					//motioncommand[8] = {0x2b, 0x40, 0x60, 0x00, 0x5f, 0x00};
					motioncommand[0] = 0x2b;
					motioncommand[1] = 0x40;
					motioncommand[2] = 0x60;
					motioncommand[3] = 0x00;
					motioncommand[4] = 0x1f;
					motioncommand[5] = 0x00;
					writeSDO(motorID, motioncommand, 6);
					//err  = writeNetworkDict(0, motorID, 0x6040, 0x00, 0x02, &temp);
					waitingWriteToSlaveDict(motorID, 0);
//					OSTimeDly(10);
				}
				else if (profileType == 0x01) //select interrupt operational mode
				{
					/* enable controller*/
					//temp = 0x007F;
					//motioncommand[8] = {0x2b, 0x40, 0x60, 0x00, 0x7f, 0x00};
					motioncommand[0] = 0x2b;
					motioncommand[1] = 0x40;
					motioncommand[2] = 0x60;
					motioncommand[3] = 0x00;
					motioncommand[4] = 0x3f;
					motioncommand[5] = 0x00;
					writeSDO(motorID, motioncommand, 6);
					//err  = writeNetworkDict(0, motorID, 0x6040, 0x00, 0x02, &temp);
					waitingWriteToSlaveDict(motorID, 0);
//					OSTimeDly(10);
				}
			}
			break;
		case 0x03:
			break;
		default:
			break;
	}
//}
	return 0;
}


void horizontal_moto_contr(u32 horizontal_acce,u32 horizontal_dece,u32 horizontal_speed,u32 horizontal_position)//
{
	double position_float;
	u32 position_desired;
	
	//the reducer ratio is 7:1, the incremental encoder on motor side is 2000 counts/r
	position_float = horizontal_position * 10;//unit 0.01mm
	
	if(position_float > 0)//move toward right side of new origin position
	{
		position_desired = (u32)((double)position_float * (double)horizontal_incr_position_25MM);
		position_desired = 0xffffffff - position_desired + 1;
	}
	else//move toward left side of new origin position
	{		
		position_desired = (u32)((double)(0.0 - position_float) *(double)horizontal_incr_position_25MM);
	}
	
	horizontal_speed = (u32)((double)horizontal_speed*(double)horizontal_encoder_speed_25MM);
	horizontal_acce  = 0x1900;     //单位为0.01mm/s^2，假设设定加速度为200mm/s^2，那么实际配置到驱动器值为20000*0.3202~=6400，即为0x1900
	horizontal_dece  = 0x1900;     //0x2580

	MotionControl(0x01, 0x01, 0x01, position_desired, horizontal_speed, horizontal_acce, horizontal_dece);//80000counts/sec2 ~=35mm/s2, Acce/Dcce ----units: 10counts/sec2

}

void vertical_moto_contr(u32 vertical_speed,u32 vertical_position)//
{
  u32 position_float; //units: degree
	u32 position_desired;
	u32 speed_desired;


	position_float = (u32)((double)30000 * HeightToPoleLength(vertical_position/10));//new vertical motor, unit: counts   /*2015.7.11*/  		
	speed_desired  = (u32)(((double)vertical_speed/(double)235)*(double)200000);//new vertical motor


	if (position_float > vertPoleCounterPowerOn)// table uping
		position_desired = (u32)(position_float - vertPoleCounterPowerOn);                                 //13ACB89
	else// table downing
		position_desired = (u32)(0xffffffff - vertPoleCounterPowerOn + position_float + 1);	 //0xffffffff- 0x13e35a6 = 0xFEC1CA59
	

 	MotionControl(0x02, 0x01, 0x01, position_desired, speed_desired, 0x9c40, 0x9c40);//new vertical motor0x2A30, 0x2A30    0x9c40
}

void setHoriPDOMapping()
{
	/* Configuring the first PDO receive, defined at index 0x1400 and 0x1600 */
  {
    /* 0x6004 stores the hori position(subindex1) value and hori speed(subindex2) value */
		s_mappedVar tabMappedVar[8] = { {0x6004,1,32}, {0x6004,2,32} }; 
    masterMappingPDO(0x1400, 0x381, tabMappedVar, 2);
  }
  /* Configuring the first PDO transmit, defined at index 0x1800 and 0x1A00 */
  {
    s_mappedVar tabMappedVar[8] = { {0x6064,0,32},{0x2231,0,32}};
    slaveMappingPDO(0x01, 0x1803, 0x381, tabMappedVar, 2, TRANS_EVERY_N_SYNC(1));
	}

	/* Configuring the second PDO receive, defined at index 0x1400 and 0x1600 */
  {
    /* 0x6000 stores the hori status word(subindex1) value and hori operation mode(subindex2) value */
		s_mappedVar tabMappedVar[8] = { {0x6000,1,16}, {0x6000,2,16} }; 
    masterMappingPDO(0x1401, 0x281, tabMappedVar, 2);
  }
  /* Configuring the second PDO transmit, defined at index 0x1800 and 0x1A00 */
  {
    s_mappedVar tabMappedVar[8] = { {0x6041,0,16}, {0x2190,0,16} };
    slaveMappingPDO(0x01, 0x1801, 0x281, tabMappedVar, 2, TRANS_EVERY_N_SYNC(2));
  }   
//add by wangqingqiang (add one more PDO trans)	2018/6/6	
	/* Configuring the second PDO receive, defined at index 0x1400 and 0x1600 */
//   {
//     /* 0x6000 stores the hori status word(subindex1) value and hori operation mode(subindex2) value */
// 		s_mappedVar tabMappedVar[8] = { {0x6002,1,32}, {0x6002,2,16} }; 
//     masterMappingPDO(0x1404, 0x181, tabMappedVar, 2);
//   }
//   /* Configuring the second PDO transmit, defined at index 0x1800 and 0x1A00 */
//   {
//     s_mappedVar tabMappedVar[8] = { {0x1002,0,32}, {0x21B4,0,16} };
//     slaveMappingPDO(0x01, 0x1802, 0x181, tabMappedVar, 2, TRANS_EVENT);
//   }
 	
}


	void setVertPDOMapping()
	{
		/* Configuring the first PDO receive, defined at index 0x1400 and 0x1600 */
		{
			/* 0x6005 stores the vert position(subindex1) value and vert speed(subindex2) value */
			s_mappedVar tabMappedVar[8] = { {0x6005,1,32},{0x6005,2,32}};
			masterMappingPDO(0x1402, 0x382, tabMappedVar, 2);
		}
		/* Configuring the first PDO transmit, defined at index 0x1800 and 0x1A00 */
		{
			s_mappedVar tabMappedVar[8] = { {0x6064,0,32},{0x606C,0,32}};
			slaveMappingPDO(0x02, 0x1803, 0x382, tabMappedVar, 2, TRANS_EVERY_N_SYNC(1));
			//slaveMappingPDO(0x02, 0x1802, 0x382, tabMappedVar, 2, TRANS_EVERY_N_SYNC(1));
		}
		
						
		/* Configuring the second PDO receive, defined at index 0x1400 and 0x1600 */
		{
			/* 0x6001 stores the vert status word(subindex1) value and vert operation mode(subindex2) value */
			s_mappedVar tabMappedVar[8] = { {0x6001,1,16}, {0x6001,2,16} }; 
			masterMappingPDO(0x1403, 0x182, tabMappedVar, 2);
		}
		/* Configuring the second PDO transmit, defined at index 0x1800 and 0x1A00 */
		{
			s_mappedVar tabMappedVar[8] = { {0x6041,0,16}, {0x2190,0,16} };
			slaveMappingPDO(0x02, 0x1801, 0x182, tabMappedVar, 2, TRANS_EVERY_N_SYNC(2));
			//slaveMappingPDO(0x02, 0x1800, 0x182, tabMappedVar, 2, TRANS_EVERY_N_SYNC(2));
		}
//add by wangqingqiang (add one more PDO trans)	2018/6/6	
// 		{
// 			/* 0x6000 stores the hori status word(subindex1) value and hori operation mode(subindex2) value */
// 			s_mappedVar tabMappedVar[8] = { {0x6003,1,32}, {0x6003,2,16} }; 
// 			masterMappingPDO(0x1405, 0x282, tabMappedVar, 2);
// 		}
// 		/* Configuring the second PDO transmit, defined at index 0x1800 and 0x1A00 */
// 		{
// 			s_mappedVar tabMappedVar[8] = { {0x1002,0,32}, {0x21B4,0,16} };
// 			slaveMappingPDO(0x02, 0x1802, 0x282, tabMappedVar, 2, TRANS_EVENT);
// 		}
	}


u8 CANopenNetworkInit(u8 type)
{
  u8 i; 
	u8 err = 0xff;
  
  /* arrays initialisation, etc */
  initCANopenMain();

  /* arrays initialisation, etc */
  initCANopenMaster();  

  /* Defining the node Id */
  setNodeId(0x03);

  /* Put the master in operational mode */
  setState(Operational);

  /* Init the table of connected nodes */
  for (i = 0 ; i < 3 ; i++)
    connectedNode[i] = 0;  
  
  /* Whose node have answered ? */
  {
		/* Sending a request Node guard to node 1 */
		masterReadNodeState(0, 0x01);
		/* Waiting for a second the response */
    OSTimeDly(100);
		connectedNode[1] = stateNode(1);
		
		OSSemPend(SemHoriCommunication,0x2710,&err);
		if (err == OS_ERR_NONE)
		{
			HoriCommunicateOK = 0xff;
			/* Configure the SDO master to communicate with node 1 */
			configure_master_SDO(0x1280, 0x01);
			/* Configure the PDO to communicate with node 1 */
			setHoriPDOMapping();
		}
		else
			HoriCommunicateOK = 0xfe;     // 11111110
		OSTimeDly(10);
  }
 
	if(type != 2)
  {
		/* Sending a message to the node 2, only as example */
		masterReadNodeState(0, 0x02);
		/* Waiting for a second the response */
		OSTimeDly(100);
		connectedNode[2] = stateNode(2);
		err = 0xff;
		OSSemPend(SemVertCommunication,0x2710,&err);
		if (err == OS_ERR_NONE)
		{
			VertCommunicateOK = 0xff;
			/* Configure the SDO master to communicate with node 2 */
			configure_master_SDO(0x1281, 0x02);
			/* Configure the PDO to communicate with node 2 */
			setVertPDOMapping();			
		}
		else
			VertCommunicateOK = 0xfd;         // 11111101
  }
	else
	{
		VertCommunicateOK = 0xff;
	}

	if(HoriCommunicateOK == 0xFF && VertCommunicateOK ==0xff)
	{
		//set CAN network synch time
		slaveSYNCEnable(0x01, 0x80, 0x2710, 0x01);
		
		/* start all nodes into operational mode */
		masterSendNMTstateChange(0, 0x00, NMT_Start_Node);
	}
	
	return (HoriCommunicateOK & VertCommunicateOK);
  /******************** END CONFIGURING THE NETWORK **********************/
}

u8 HomeMotion(u8 MotorID, u8 MotionDirection, u32 MotionSpeed, u32 MotionAcce)
{
	u8  motioncommand[8];
	u8  ReachHomeOK = 0x00;
	u8  err = 0xff;
	u8  speed0, speed1, speed2, speed3;
	u8  acce0, acce1, acce2, acce3;
	
	speed0 = (u8) MotionSpeed;
	speed1 = (u8) (MotionSpeed >> 8);
	speed2 = (u8) (MotionSpeed >> 16);
	speed3 = (u8) (MotionSpeed >> 24);
	
	acce0  = (u8)  MotionAcce;
	acce1  = (u8)  (MotionAcce >> 8);
	acce2  = (u8)  (MotionAcce >> 16);
	acce3  = (u8)  (MotionAcce >> 24);
	
  switch(MotorID)
  {
    case 0x00:
		{
			motioncommand[0] = 0x2f;
			motioncommand[1] = 0x60;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x06;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			waitingWriteToSlaveDict(0x01, 0);
				
			motioncommand[0] = 0x2f;
			motioncommand[1] = 0x98;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = MotionDirection;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			waitingWriteToSlaveDict(0x01, 0);
			
//			OSTimeDly(10);
				
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x99;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x01;
			motioncommand[4] = speed0;
			motioncommand[5] = speed1;
			motioncommand[6] = speed2;
			motioncommand[7] = speed3;
			writeSDO(0x01, motioncommand, 8);      
			waitingWriteToSlaveDict(0x01, 0);
					
//			OSTimeDly(10);
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x99;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x02;
			motioncommand[4] = 0xd2;
			motioncommand[5] = 0x4e;
			motioncommand[6] = 0x01;
			motioncommand[7] = 0x00;
			writeSDO(0x01, motioncommand, 8);      
			waitingWriteToSlaveDict(0x01, 0);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x9a;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = acce0;
			motioncommand[5] = acce1;
			motioncommand[6] = acce2;
			motioncommand[7] = acce3;
			writeSDO(0x01, motioncommand, 8);      
			waitingWriteToSlaveDict(0x01, 0);		
			
			
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x0f;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);
			waitingWriteToSlaveDict(0x01, 0);
			
//			OSTimeDly(10);
			
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x1f;
			motioncommand[5] = 0x00;
			writeSDO(0x01, motioncommand, 6);      
			waitingWriteToSlaveDict(0x01, 0);
			
//			OSTimeDly(10);
			
			HoriFirstPosInit = 0x01;
			OSSemPend(SemHoriReachHome,0xFDE8,&err);
	
			if (err == OS_ERR_NONE)
			{
				ReachHomeOK = 0xff;
				home_reached = 0;
				OSTimeDly(3);     //2015.8.12
				home_reached = 1;
				OSTimeDly(3);     //2015.8.12
				home_reached = 0;
			}
			else
				ReachHomeOK = 0xfe;
			break;
		}
		case 0x01:
		{
			motioncommand[0] = 0x2f;
			motioncommand[1] = 0x60;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x06;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			waitingWriteToSlaveDict(0x02, 0);
				
			motioncommand[0] = 0x2f;
			motioncommand[1] = 0x98;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = MotionDirection;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
				
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x99;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x01;
			motioncommand[4] = speed0;
			motioncommand[5] = speed1;
			motioncommand[6] = speed2;
			motioncommand[7] = speed3;
			writeSDO(0x02, motioncommand, 8);      
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x99;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x02;
			motioncommand[4] = speed0;
			motioncommand[5] = speed1;
			motioncommand[6] = speed2;
			motioncommand[7] = speed3;
			writeSDO(0x02, motioncommand, 8);      
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
			
			motioncommand[0] = 0x23;
			motioncommand[1] = 0x9a;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = acce0;
			motioncommand[5] = acce1;
			motioncommand[6] = acce2;
			motioncommand[7] = acce3;
			writeSDO(0x02, motioncommand, 8);      
			waitingWriteToSlaveDict(0x02, 0);
			
						
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x0f;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
			
			motioncommand[0] = 0x2b;
			motioncommand[1] = 0x40;
			motioncommand[2] = 0x60;
			motioncommand[3] = 0x00;
			motioncommand[4] = 0x1f;
			motioncommand[5] = 0x00;
			writeSDO(0x02, motioncommand, 6);      
			waitingWriteToSlaveDict(0x02, 0);
			
//			OSTimeDly(10);
			
			VertFirstPosInit = 0x01;
			OSSemPend(SemVertReachHome,0xFDE8,&err);
	
			if (err == OS_ERR_NONE)
			{
				ReachHomeOK = 0xff;
				home_reached = 0;
				OSTimeDly(3);      //2015.8.12
				home_reached = 1;
				OSTimeDly(3);      //2015.8.12
				home_reached = 0;
				
			}
			else
				ReachHomeOK = 0xfd;
			
			break;
		}
		default:
			break;
  }	
	
	return ReachHomeOK;
}

u8 TablePosInit(u8 HomeNumber)//put the hori and vert in home motion mode
{
	u8 HoriPosInitResult, VertPosInitResult;
	
	if(HomeNumber==0x01)
		vertPoleCounterPowerOn = 0x12135a5;//0x12135a5高度为880cm;
	else
		vertPoleCounterPowerOn = 0x13AF095;//0x13AF095高度为990cm;//0x13E35A6;//0x1ac7788//0x13ACC95;
	
	switch(HomeNumber)
	{
		case 0x00:
		case 0x01:
		case 0x03:
			H_CLUTCH = SET_H_CLUTCH;
			H_BRAKE  = CLR_H_BRAKE;
			V_BRAKE = CLR_V_BRAKE;
		  
			handleMotionError(0);
			OSTimeDly(10);
			handleMotionError(1);
			OSTimeDly(20);
		
			HoriPosInitResult = HomeMotion(0x00, 0x13, 0x14ed2, 0x11f40); 
			VertPosInitResult = HomeMotion(0x01, 0x13, 0xbec78, 0x1fa0);		
			break;
		case 0x02:
			H_CLUTCH = SET_H_CLUTCH;
			H_BRAKE  = CLR_H_BRAKE;
		
			handleMotionError(0);
			OSTimeDly(10);
			
			HoriPosInitResult = HomeMotion(0x00, 0x13, 0x14ed2, 0x11f40);
			VertPosInitResult = 0xFF;
			break;
	}
	return (HoriPosInitResult & VertPosInitResult);
}

u8 Post(u8 Post_Type)
{
	u8  TestNetResult, TestPosInitResult;
	
	/*Test the communication to drivers*/
	TestNetResult = CANopenNetworkInit(Post_Type);
	/*Homing of horizontal and vertical*/
	if((TestNetResult&0x01)==0xFE)//水平失败
	{
		TestPosInitResult = 0xFC;
	}
	else if(TestNetResult==0xFD)//水平成功，垂直失败,LC16 LOW
	{
		model_type = 0x02;
		TestPosInitResult = TablePosInit(0x02);
	}
	else if(TestNetResult==0xFF)//都成功，LC16 HIGH
	{
		model_type = 0x01;
		TestPosInitResult = TablePosInit(0x01);
	}

	return (((TestPosInitResult<<2) | 0xf3) & TestNetResult);
}


>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
