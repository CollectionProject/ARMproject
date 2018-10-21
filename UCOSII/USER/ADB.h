#include "stm32f10x.h"
#include "sys.h"	 
#include "stdlib.h"
#include "usart.h"	 
#include "delay.h"
#include "includes.h"					//ucos สนำร
#include "crc16.h"
typedef enum
{
	adb_start=0,
	adb_length,
	adb_id,
	adb_ack_nack,
	adb_enc_flag,
	adb_index,
	adb_data,
	adb_crc,
	adb_idel
}adb_fsm;

typedef enum
{
	adb_failed=200,
	adb_auth_begin_success,
	adb_err_length,
	adb_err_id,
	adb_err_nack_unknow,
	abd_err_nack_crc,
	abd_err_nack_bad_request_id,
	abd_err_nack_bad_parameter,
	abd_err_nack_msg_timeout,
	abd_err_nack_bad_arg_length,
	abd_err_nack_real_time_clk,
	abd_err_nack_system_is_full,
	abd_err_nack_operation_fail,
	abd_err_nack_illegal_enc,
	abd_err_nack_msg_len_short,
	abd_err_nack_secu_traf_dis,
	adb_err_enc_flag,
	adb_err_index,
	adb_err_data,
	adb_err_crc,
}adb_state;

int ADB_authenticate_begin(u8 year,u8 month,u8 day,u8 hour,u8 minute,u8 seconds);

