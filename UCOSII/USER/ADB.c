#include "ADB.h"

extern u8 is_New;
extern OS_EVENT *ADB_Q;
static void send_adb(u8 * buf,u8 size);

static int abd_get_nack_err_code(int code)
{
	int res=adb_err_nack_unknow;

	switch(code)
	{
		case 1:
			res = abd_err_nack_crc;
			break;
		case 2:
			res = abd_err_nack_bad_request_id;
			break;
		case 3:
			res = abd_err_nack_bad_parameter;
			break;
		case 4:
			res = abd_err_nack_msg_timeout;
			break;
		case 5:
			res = abd_err_nack_bad_arg_length;
			break;
		case 6:	
			res = abd_err_nack_real_time_clk;
			break;
		case 7:
			res = abd_err_nack_system_is_full;
			break;
		case 8:
			res = abd_err_nack_operation_fail;
			break;
		case 9:
			res = abd_err_nack_illegal_enc;
			break;
		case 10:
			res = abd_err_nack_msg_len_short;
			break;
		case 11:
			res = abd_err_nack_secu_traf_dis;
			break;
	}
	return res;
}

static void send_adb(u8 * buf,u8 size)
{
	int i=0;
	for(i=0;i<size;i++)
	{
			while((USART2->SR&0X40)==0);//循环发送,直到发送完毕   
			USART2->DR = (u8) buf[i];
	}
}

int ADB_authenticate_begin(u8 year,u8 month,u8 day,u8 hour,u8 minute,u8 seconds)
{
	u8 err;
	u8 buf[14+2]={0};
	u8 *p;
	u8 adb_res[50] = {0};

	buf[0] = 0x2B;
	buf[1] = 14;
	buf[2] = 0x01;
	buf[3] = 0xEE;
	buf[4] = 0x00;
	buf[5] = 0x00;
	buf[6] = year;
	buf[7] = month;
	buf[8] = day;
	buf[9] = hour;
	buf[10] = minute;
	buf[11] = seconds;
	CRC16_IBM(buf,12,&buf[14]);
	buf[12] = buf[14];
	buf[13] = buf[15];
	
	is_New = 1;
	send_adb(buf,14);
	
	p = (u8 *)OSQPend(ADB_Q,5000,&err);
	if(err!=OS_ERR_NONE)
	{
		return err;
	}
	memcpy(adb_res,p,p[1]);
	if(adb_res[1]<9)
		return adb_err_length;
	if(adb_res[2]!=0x01)
		return adb_err_id;
	if(adb_res[3]!=0xFD)
	{
		return abd_get_nack_err_code(adb_res[6]);
	}
	if(adb_res[4]!=0x00)
		return adb_err_enc_flag;
	if(adb_res[5]!=0x00)
		return adb_err_index;
	if(adb_res[6]!=0xee)
		return adb_err_data;
	CRC16_IBM(adb_res,7,&buf[14]);
	if(adb_res[7]!=buf[14]||adb_res[8]!=buf[15])
		return adb_err_crc;
	
	return adb_auth_begin_success;
}
