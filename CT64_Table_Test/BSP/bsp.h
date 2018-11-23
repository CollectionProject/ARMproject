<<<<<<< HEAD
#ifndef __BSP_H
#define __BSP_H

#include "CAN_Cfg.h"
#include "stm32f4xx.h"

//---------------------------firmware version-----------------------
#define VERSION_MAJOR 0x02
#define VERSION_MINOR 0x05
#define VERSION_Y 0x18
#define VERSION_W READ_MODEL_TYPE
//-----------------------------------------------------------------

#define SERIAL_NO_ADDR   0x08050800
#define MODEL_TYPE_ADDR  0x080D0000
#define USER_ADDR_BASE   0x080A0000


#define READ_MODEL_TYPE  (*(volatile unsigned char*)MODEL_TYPE_ADDR)

#define HORI_STATUS_REG   0x01
#define HORI_IO_REG       0x02
#define VERT_STATUS_REG   0x03
#define VERT_IO_REG		    0x04
#define HORI_POS_REG		  0x05
#define HORI_SPEED_REG    0x06
#define VERT_POS_REG	    0x07
#define VERT_SPEED_REG    0x08
#define HORI_ERROR_REG    0x09
#define VERT_ERROR_REG    0x0a


#define horizontal_incr_position_25MM   3.2000   
#define horizontal_encoder_speed_25MM   32.0000
#define horizontal_encoder_acce_25MM    0.32000   
#define horizontal_encoder_dece_25MM    0.32000	

typedef enum 
{
   POST=1,
   POSTPASS ,
   POSTFAIL ,
   IDLE ,
   ARMED ,
   ABNORMAL 
} GLOBALSTATE;

typedef enum 
{
   HORI_TOTAL=0,
	 VERT_TOTAL,
	 H_FLOAT_TOTAL,
	 V_BREAK_TOTAL
} TableTotal;

//Response ID44 information
typedef struct {
	u16 HoriPos;
	u16 HoriSpeed;
	u16 VertPos;
	u16 VertSpeed;
}ID_44;

//Response ID45 information
typedef struct {
  u8 	GlobalState;
	u8  HoriState;
	u8	VertState;
	u8  Float_State;
	u8 	EMS_Stop_State;
	u8  IO_State;
	u16 errcode;
}ID_45;

//Response ID45 information
typedef struct {
  u32 Hori_err;
	u32 Vert_err;
}ID_49;

typedef struct {
  ID_44 id44_res;
	ID_45 id45_res;
}RES_TABLE_STATE;

typedef struct
{
  u32  h_pos; 
  u32  h_speed;  
	u32  h_acce;
	u32  h_dece;
} HORI_MSG;

typedef struct
{
  u32  v_pos; 
  u32  v_speed;  
} VERT_MSG;

typedef struct
{
  u8  motor_id; //1: hori motor, 2: vert motor 3: every motor  
  u8  stop_mode;//0: abrupt stop, 1: low to stop	
} DISABLE_MSG;

u32 ReadOD(u8 name);
u8 MotionControl(u8 motorID, u8 operMode, u8 profileType, u32 Position, u32 Speed, u32 Acce, u32 Dece);
u8 handleMotionError (u8 ControllerID);
u8 ShutOffMotor(u8 ControllerID);
u8 handleMotionDisable (u8 ControllerID, u8 StopMode);
void horizontal_moto_contr(u32 horizontal_acce,u32 horizontal_dece,u32 horizontal_speed,u32 horizontal_position);
void vertical_moto_contr(u32 vertical_speed,u32 vertical_position);
u8 CANopenNetworkInit(u8 type);
u8 TablePosInit(u8 HomeNumber);
u8 Post(u8 Post_Type);
double HeightToPoleLength(double Height);
void Exit_Configuration(void);
RES_TABLE_STATE ResponseData(u8 type);
void HandleRespData(RES_TABLE_STATE msg);
void Judge_Gloab_State(u8 PostResult);
void writeSDO(u8 nodeId, u8* data, u8 len);
void CAN_SendTableTotal(u8 type,u32 delta);
#endif

=======
#ifndef __BSP_H
#define __BSP_H

#include "CAN_Cfg.h"
#include "stm32f4xx.h"

//---------------------------firmware version-----------------------
#define VERSION_MAJOR 0x02
#define VERSION_MINOR 0x05
#define VERSION_Y 0x18
#define VERSION_W READ_MODEL_TYPE
//-----------------------------------------------------------------

#define SERIAL_NO_ADDR   0x08050800
#define MODEL_TYPE_ADDR  0x080D0000
#define USER_ADDR_BASE   0x080A0000


#define READ_MODEL_TYPE  (*(volatile unsigned char*)MODEL_TYPE_ADDR)

#define HORI_STATUS_REG   0x01
#define HORI_IO_REG       0x02
#define VERT_STATUS_REG   0x03
#define VERT_IO_REG		    0x04
#define HORI_POS_REG		  0x05
#define HORI_SPEED_REG    0x06
#define VERT_POS_REG	    0x07
#define VERT_SPEED_REG    0x08
#define HORI_ERROR_REG    0x09
#define VERT_ERROR_REG    0x0a


#define horizontal_incr_position_25MM   3.2000   
#define horizontal_encoder_speed_25MM   32.0000
#define horizontal_encoder_acce_25MM    0.32000   
#define horizontal_encoder_dece_25MM    0.32000	

typedef enum 
{
   POST=1,
   POSTPASS ,
   POSTFAIL ,
   IDLE ,
   ARMED ,
   ABNORMAL 
} GLOBALSTATE;

typedef enum 
{
   HORI_TOTAL=0,
	 VERT_TOTAL,
	 H_FLOAT_TOTAL,
	 V_BREAK_TOTAL
} TableTotal;

//Response ID44 information
typedef struct {
	u16 HoriPos;
	u16 HoriSpeed;
	u16 VertPos;
	u16 VertSpeed;
}ID_44;

//Response ID45 information
typedef struct {
  u8 	GlobalState;
	u8  HoriState;
	u8	VertState;
	u8  Float_State;
	u8 	EMS_Stop_State;
	u8  IO_State;
	u16 errcode;
}ID_45;

//Response ID45 information
typedef struct {
  u32 Hori_err;
	u32 Vert_err;
}ID_49;

typedef struct {
  ID_44 id44_res;
	ID_45 id45_res;
}RES_TABLE_STATE;

typedef struct
{
  u32  h_pos; 
  u32  h_speed;  
	u32  h_acce;
	u32  h_dece;
} HORI_MSG;

typedef struct
{
  u32  v_pos; 
  u32  v_speed;  
} VERT_MSG;

typedef struct
{
  u8  motor_id; //1: hori motor, 2: vert motor 3: every motor  
  u8  stop_mode;//0: abrupt stop, 1: low to stop	
} DISABLE_MSG;

u32 ReadOD(u8 name);
u8 MotionControl(u8 motorID, u8 operMode, u8 profileType, u32 Position, u32 Speed, u32 Acce, u32 Dece);
u8 handleMotionError (u8 ControllerID);
u8 ShutOffMotor(u8 ControllerID);
u8 handleMotionDisable (u8 ControllerID, u8 StopMode);
void horizontal_moto_contr(u32 horizontal_acce,u32 horizontal_dece,u32 horizontal_speed,u32 horizontal_position);
void vertical_moto_contr(u32 vertical_speed,u32 vertical_position);
u8 CANopenNetworkInit(u8 type);
u8 TablePosInit(u8 HomeNumber);
u8 Post(u8 Post_Type);
double HeightToPoleLength(double Height);
void Exit_Configuration(void);
RES_TABLE_STATE ResponseData(u8 type);
void HandleRespData(RES_TABLE_STATE msg);
void Judge_Gloab_State(u8 PostResult);
void writeSDO(u8 nodeId, u8* data, u8 len);
void CAN_SendTableTotal(u8 type,u32 delta);
#endif

>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
