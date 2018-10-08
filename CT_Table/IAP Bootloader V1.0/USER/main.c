#include "sys.h"
#include "delay.h"    
#include "stmflash.h" 
#include "iap.h"   
#include "can.h"

extern u32 addr_offset;
extern u8 block_start;
extern u8 block_end;
extern u8 total_end;
extern u32 CAN_RX_CNT;	
extern u8 CAN_RX_BUF[CAN_REC_LEN];
extern u16 total_bytes;
typedef enum
{
	IDLE=0,
	READY_START,
	READY_END,
	OPERATION,
	DONE,
	BACK_UP
}State;

int main(void)
{ 
	u16 t=0;
	u8 state = IDLE;
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置系统中断优先级分组2
	delay_init(168);  //初始化延时函数
	CAN_Config();
	
	delay_ms(500);
	
	start_download();
	
	while(1)
	{
		switch(state)
		{
				case IDLE:
					if(t>500)
					{
						t = 0;
						state = DONE;
					}
					else
					{
						t++;
					}
					if(block_start)
					{
						start_block_trans();
						block_start=0;
						state = READY_START;
					}
					break;
				case READY_START:
					if(block_end)
					{
						end_block_trans(addr_offset,total_bytes);
						block_end=0;
						state = READY_END;
					}
					break;
				case READY_END:
					if(block_start)
					{
						start_block_trans();
						block_start=0;
						state = READY_START;
					}
					else if(total_end)
					{
						state = OPERATION;
					}
					break;
				case OPERATION:
					if(((*(vu32*)(0X20001001))&0xFFFFFFFF)==0x4C424154)
					{
						if(((*(vu32*)(0X20001000+4+8))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
						{	 
							iap_write_appbin(FLASH_APP1_ADDR,CAN_RX_BUF+8,CAN_RX_CNT-8);//更新FLASH代码   
							end_download();
							state = DONE;
						}
						else
						{
							state = IDLE;
						}
					}
					else 
					{
						state = IDLE;
					}
					break;
				case DONE:
					//尝试进入APP
					if(((*(vu32*)(FLASH_APP1_ADDR+4))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
					{	 
						//__set_FAULTMASK(1);      // 关闭所有中端
						iap_load_app(FLASH_APP1_ADDR);//执行FLASH APP代码
					}else 
					{
						//进入不成功
						state = BACK_UP;
					}
					break;
				case BACK_UP:
					if(((*(vu32*)(FLASH_ORIGIN_APP_ADDR+4))&0xFF000000)==0x08000000)//判断是否为0X08XXXXXX.
					{	
						//__set_FAULTMASK(1);      // 关闭所有中端
						iap_load_app(FLASH_ORIGIN_APP_ADDR);//执行原始APP代码
					}
					else
					{
						state = IDLE;
					}
					break;
		}
		delay_ms(10);
	}  
}

