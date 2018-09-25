#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 		 	 
#include "lcd.h"  
#include "key.h"     
#include "malloc.h"
#include "beep.h"   
#include "touch.h"    
#include "includes.h"  
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"

/* ------------------file------------------------ */
FATFS fs;            // Work area (file system object) for logical drive
FIL fsrc, fdst;      // file objects
BYTE buffer[51];     // file copy buffer
FRESULT res;         // FatFs function common result code
UINT br, bw;         // File R/W count

/////////////////////////UCOSII任务设置///////////////////////////////////
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			10 //开始任务的优先级设置为最低
//设置任务堆栈大小
#define START_STK_SIZE  				64
//任务堆栈	
OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata);	
 			   
//LED任务
//设置任务优先级
#define LED_TASK_PRIO       			7 
//设置任务堆栈大小
#define LED_STK_SIZE  		    		64
//任务堆栈
OS_STK LED_TASK_STK[LED_STK_SIZE];
//任务函数
void led_task(void *pdata);

//触摸屏任务
//设置任务优先级
#define TOUCH_TASK_PRIO       		 	6
//设置任务堆栈大小
#define TOUCH_STK_SIZE  				128
//任务堆栈	
OS_STK TOUCH_TASK_STK[TOUCH_STK_SIZE];
//任务函数
void touch_task(void *pdata);


//主任务
//设置任务优先级
#define MAIN_TASK_PRIO       			4 
//设置任务堆栈大小
#define MAIN_STK_SIZE  					128
//任务堆栈	
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//任务函数
void main_task(void *pdata);


//按键扫描任务
//设置任务优先级
#define KEY_TASK_PRIO       			2 
//设置任务堆栈大小
#define KEY_STK_SIZE  					128
//任务堆栈	
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//任务函数
void key_task(void *pdata);
//////////////////////////////////////////////////////////////////////////////
    
OS_EVENT * msg_key;			//按键邮箱事件块	  
void * MsgGrp[256];			//消息队列存储地址,最大支持256个消息

//加载主界面   
void ucos_load_main_ui(void)
{
	LCD_DisplayCurrentFont();
	delay_ms(300);
}	  

int main(void)
{	 		    
	char *str;
			FILINFO finfo;
		DIR dirs;
		char path[100]={""};  
		SD_CardInfo cardinfo;
	delay_init();	    	 //延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200
	SZ_STM32_LCDInit();
	//BEEP_Init();

	LED_Init(LED1);
	LED_Init(LED2);
	LED_Init(LED3);
	LED_Init(LED4);

	printf("\n file system starting! \n");
	SD_Init();
	SD_GetCardInfo(&cardinfo);
	disk_initialize(0);
	f_mount(0, &fs);
  LCD_Clear(LCD_COLOR_WHITE);
	LCD_DisplayStringLine(LCD_LINE_0,"SD Card IN");
	sprintf(str,"file size is: %d MB", cardinfo.CardCapacity/ 1024 / 1024);
	LCD_DisplayStringLine(LCD_LINE_1,str);
	
	if (f_opendir(&dirs, path) == FR_OK) 
	{
		while (f_readdir(&dirs, &finfo) == FR_OK)  
		{
			if (finfo.fattrib & AM_ARC) 
			{
				if(!finfo.fname[0])	
					break;         
				  sprintf(str,"file size is: %s", finfo.fname);
					LCD_DisplayStringLine(LCD_LINE_2,str);
				  sprintf(str,"file size is: %d bytes", finfo.fsize);
				  LCD_DisplayStringLine(LCD_LINE_3,str);
			}
			else
			{
				printf("\n\r Path name is: %s", finfo.fname); 
				break;
			}
		} 
	}
	else
	{
			printf("\n\r err: f_opendir\n\r"); 
	}
	
	//ucos_load_main_ui(); 		//加载主界面
	OSInit();  	 				//初始化UCOSII
		OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//创建起始任务
	OSStart();	  
}

//开始任务
void start_task(void *pdata)
{
  OS_CPU_SR cpu_sr=0;    	    
	pdata = pdata; 	
	
 	OS_ENTER_CRITICAL();				//进入临界区(无法被中断打断)    
 	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);						   
 	OSTaskCreate(touch_task,(void *)0,(OS_STK*)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1],TOUCH_TASK_PRIO);	 				   		   
 	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);	 							   
 	OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);	 				   
 	OSTaskSuspend(START_TASK_PRIO);		//挂起起始任务.
	OS_EXIT_CRITICAL();					//退出临界区(可以被中断打断)
	
}
//LED任务
void led_task(void *pdata)
{
	u8 t=0;
	while(1)
	{
		t++;
		delay_ms(10);
		if(t==8)LED1_STATE=1;	//LED0灭
		if(t==100)		//LED0亮
		{
			t=0;
			LED1_STATE=0;
		}
	}									 
}
//触摸屏任务
void touch_task(void *pdata)
{	  	
		FILINFO finfo;
		DIR dirs;
		char path[100]={""};  
		SD_CardInfo cardinfo;

		printf("\n file system starting! \n");
		SD_Init();
		SD_GetCardInfo(&cardinfo);
		disk_initialize(0);
		f_mount(0, &fs);

		if (f_opendir(&dirs, path) == FR_OK) 
		{
			while (f_readdir(&dirs, &finfo) == FR_OK)  
			{
				if (finfo.fattrib & AM_ARC) 
				{
					if(!finfo.fname[0])	
						break;         
						printf("\n\r file name is: %s\n",finfo.fname);
						printf("\n\r file size is: %d ", finfo.fsize); 
				}
				else
				{
					printf("\n\r Path name is: %s", finfo.fname); 
					break;
				}
			} 
		}
		else
		{
				printf("\n\r err: f_opendir\n\r"); 
		}
 
		while(1)
		{
			LED2_STATE = 0;
			delay_ms(1000);	 
			LED2_STATE = 1;
			delay_ms(1000);
			//printf("This is Touch task\r\n");
		}
}     

//主任务
void main_task(void *pdata)
{							 			 
 	while(1)
	{
		LED3_STATE = 0;
		delay_ms(1000);	 
		LED3_STATE = 1;
		delay_ms(1000);
		//printf("This is Main task\r\n");
	}
}		   
   		    
//按键扫描任务
void key_task(void *pdata)
{	
	while(1)
	{
		LED4_STATE = 0;
		delay_ms(1000);	 
		LED4_STATE = 1;
		delay_ms(1000);
		//printf("This is Key task\r\n");
	}
}
