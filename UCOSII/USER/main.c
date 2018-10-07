#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 		 	 
#include "bsp_lcd.h"  
#include "key.h"     
#include "malloc.h"
#include "beep.h"   
#include "touch.h"    
#include "includes.h"  
#include "ff.h"
#include "diskio.h"
#include "sdcard.h"
#include "ugui.h"

/* ------------------file------------------------ */
FATFS fs;            // Work area (file system object) for logical drive
FIL fsrc, fdst;      // file objects
BYTE buffer[51];     // file copy buffer
FRESULT res;         // FatFs function common result code
UINT br, bw;         // File R/W count

UG_GUI gui ; // Global GUI structure

/////////////////////////UCOSII��������///////////////////////////////////
//START ����
//�����������ȼ�
#define START_TASK_PRIO      			10 //��ʼ��������ȼ�����Ϊ���
//���������ջ��С
#define START_STK_SIZE  				64
//�����ջ	
OS_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *pdata);	
 			   
//LED����
//�����������ȼ�
#define LED_TASK_PRIO       			7 
//���������ջ��С
#define LED_STK_SIZE  		    		64
//�����ջ
OS_STK LED_TASK_STK[LED_STK_SIZE];
//������
void led_task(void *pdata);

//����������
//�����������ȼ�
#define TOUCH_TASK_PRIO       		 	6
//���������ջ��С
#define TOUCH_STK_SIZE  				128
//�����ջ	
OS_STK TOUCH_TASK_STK[TOUCH_STK_SIZE];
//������
void touch_task(void *pdata);


//������
//�����������ȼ�
#define MAIN_TASK_PRIO       			20 
//���������ջ��С
#define MAIN_STK_SIZE  					1024
//�����ջ	
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//������
void main_task(void *pdata);


//����ɨ������
//�����������ȼ�
#define KEY_TASK_PRIO       			2 
//���������ջ��С
#define KEY_STK_SIZE  					128
//�����ջ	
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//������
void key_task(void *pdata);
//////////////////////////////////////////////////////////////////////////////
    
OS_EVENT * msg_key;			//���������¼���	  
void * MsgGrp[256];			//��Ϣ���д洢��ַ,���֧��256����Ϣ

//����������   
void ucos_load_main_ui(void)
{
	LCD_DisplayCurrentFont();
	delay_ms(300);
}	  



void UserPixelSetFunction ( UG_S16 x , UG_S16 y ,UG_COLOR c )
{
	LCD_SetPoint(y,x,RGB_888toRGB_565(c));
}

int main(void)
{	 		    
//	char *str;
// 	FILINFO finfo;
// 	DIR dirs;
// 	char path[100]={""};  
// 	SD_CardInfo cardinfo;
	
	delay_init();	    	 //��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
	SZ_STM32_LCDInit();
	//BEEP_Init();

	UG_Init(&gui , UserPixelSetFunction , 320 , 240) ;
	UG_FontSetHSpace(0);
	UG_FontSetVSpace(0);
	LED_Init(LED1);
	LED_Init(LED2);
	LED_Init(LED3);
	LED_Init(LED4);
	
// 	printf("\n file system starting! \n");
// 	SD_Init();
// 	SD_GetCardInfo(&cardinfo);
// 	disk_initialize(0);
// 	f_mount(0, &fs);
	//LCD_Clear(rgb_24_2_565(C_BLUE));
	//UG_FillScreen(C_RED);
	
	OSInit();  	 				//��ʼ��UCOSII
		OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//������ʼ����
	OSStart();	  
}

//��ʼ����
void start_task(void *pdata)
{
  OS_CPU_SR cpu_sr=0;    	    
	pdata = pdata; 	
	
 	OS_ENTER_CRITICAL();				//�����ٽ���(�޷����жϴ��)    
 	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);						   
 	OSTaskCreate(touch_task,(void *)0,(OS_STK*)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1],TOUCH_TASK_PRIO);	 				   		   
 	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);	 							   
 	OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);	 				   
 	OSTaskSuspend(START_TASK_PRIO);		//������ʼ����.
	OS_EXIT_CRITICAL();					//�˳��ٽ���(���Ա��жϴ��)
	
}
//LED����
void led_task(void *pdata)
{
	while(1)
	{
		LED1_STATE = 0;
		delay_ms(200);	 
		LED1_STATE = 1;
		delay_ms(200);
		UG_Update();
	}									 
}
//����������
void touch_task(void *pdata)
{	  	
		while(1)
		{
			LED2_STATE = 0;
			delay_ms(1000);	 
			LED2_STATE = 1;
			delay_ms(1000);
		}
}     
   	
void window_1_callback(UG_MESSAGE* msg)
{
	if(msg->type==MSG_TYPE_OBJECT)
	{
		if(msg->id==OBJ_TYPE_BUTTON)
		{
			switch(msg->sub_id)
			{
				case BTN_ID_0:
				{
					break;
				}
			}
		}
	}
}
#define POSX 100
#define POSY 50
//������
void main_task(void *pdata)
{							 		

	
	
	
		UG_WINDOW window_1;
		UG_BUTTON button_1;
		UG_BUTTON button_2;
		UG_BUTTON button_3;
		UG_TEXTBOX textbox_1;
		UG_OBJECT obj_buff_wnd_1[10];
	
	  UG_WindowCreate(&window_1,obj_buff_wnd_1,10,window_1_callback);
		/* Modify the window t i t l e */
		UG_WindowSetTitleText ( &window_1 , "uGUI Demo Window" ) ;
		UG_WindowSetTitleTextFont ( &window_1 , &FONT_12X20 ) ;
		/* Create some buttons */
		UG_ButtonCreate ( &window_1 , &button_1 , BTN_ID_0 , 10 , 10 , 110 , 60 ) ;
		UG_ButtonCreate ( &window_1 , &button_2 , BTN_ID_1 , 10 , 80 , 110 , 130 ) ;
		UG_ButtonCreate ( &window_1 , &button_3 , BTN_ID_2 , 10 , 150 , 110 , 200 ) ;
		/* Label the buttons */
		UG_ButtonSetFont ( &window_1 , BTN_ID_0 , &FONT_12X20 ) ;
		UG_ButtonSetText ( &window_1 , BTN_ID_0 , "BtnA" ) ;
		UG_ButtonSetFont ( &window_1 , BTN_ID_1 , &FONT_12X20 ) ;
		UG_ButtonSetText ( &window_1 , BTN_ID_1 , "BtnB" ) ;
		UG_ButtonSetFont ( &window_1 , BTN_ID_2 , &FONT_12X20 ) ;
		UG_ButtonSetText ( &window_1 , BTN_ID_2 , "BtnC" ) ;
		/* Create a Textbox */
		UG_TextboxCreate ( &window_1 , &textbox_1 , TXB_ID_0 , 120 , 10 , 310 , 200 ) ;
		UG_TextboxSetFont ( &window_1 , TXB_ID_0 , &FONT_8X14 ) ;
		UG_TextboxSetText ( &window_1 , TXB_ID_0 ,"Hi Everyone:\nMy name is \nWangQingQiang\nThis is the test\n of UGUI\nOK,Bye" ) ;
		UG_TextboxSetForeColor ( &window_1 , TXB_ID_0 , C_BLACK ) ;
		UG_TextboxSetAlignment ( &window_1 , TXB_ID_0 , ALIGN_TOP_LEFT ) ;
		UG_TextboxSetHSpace(&window_1,TXB_ID_0,0);
	
	UG_WindowShow(&window_1);
	
	
	
 	while(1)
	{
		LED3_STATE = 0;
		delay_ms(1000);	 
		LED3_STATE = 1;
		delay_ms(1000);
	}
}		   

//����ɨ������
void key_task(void *pdata)
{	
	while(1)
	{
		LED4_STATE = 0;
		delay_ms(1000);	 
		LED4_STATE = 1;
		delay_ms(1000);
	}
}
