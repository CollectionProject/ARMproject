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

 
 
/************************************************
 ALIENTEKս��STM32������ʵ��53
 UCOSIIʵ��3-��Ϣ���С��ź������������ʱ��  ʵ�� 
 ����֧�֣�www.openedv.com
 �Ա����̣�http://eboard.taobao.com 
 ��ע΢�Ź���ƽ̨΢�źţ�"����ԭ��"����ѻ�ȡSTM32���ϡ�
 ������������ӿƼ����޹�˾  
 ���ߣ�����ԭ�� @ALIENTEK
************************************************/


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

//������Ϣ��ʾ����
//�����������ȼ�
#define QMSGSHOW_TASK_PRIO    			5 
//���������ջ��С
#define QMSGSHOW_STK_SIZE  		 		128
//�����ջ	
OS_STK QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE];
//������
void qmsgshow_task(void *pdata);


//������
//�����������ȼ�
#define MAIN_TASK_PRIO       			4 
//���������ջ��С
#define MAIN_STK_SIZE  					128
//�����ջ	
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//������
void main_task(void *pdata);

//�ź���������
//�����������ȼ�
#define FLAGS_TASK_PRIO       			3 
//���������ջ��С
#define FLAGS_STK_SIZE  		 		128
//�����ջ	
OS_STK FLAGS_TASK_STK[FLAGS_STK_SIZE];
//������
void flags_task(void *pdata);


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
OS_EVENT * q_msg;			//��Ϣ����
OS_TMR   * tmr1;			//�����ʱ��1
OS_TMR   * tmr2;			//�����ʱ��2
OS_TMR   * tmr3;			//�����ʱ��3
OS_FLAG_GRP * flags_key;	//�����ź�����
void * MsgGrp[256];			//��Ϣ���д洢��ַ,���֧��256����Ϣ

//�����ʱ��1�Ļص�����	
//ÿ100msִ��һ��,������ʾCPUʹ���ʺ��ڴ�ʹ����		   
void tmr1_callback(OS_TMR *ptmr,void *p_arg) 
{
 	static u16 cpuusage=0;
	static u8 tcnt=0;	    
	if(tcnt==5)
	{
 		//��ʾCPUʹ����  
		cpuusage=0;
		tcnt=0; 
	}
	cpuusage+=OSCPUUsage;
	tcnt++;				    
 	//��ʾ�ڴ�ʹ����	 	  		 					    
	//��ʾ���е�ǰ�Ĵ�С		   
 }

//�����ʱ��2�Ļص�����				  	   
void tmr2_callback(OS_TMR *ptmr,void *p_arg) 
{	
	static u8 sta=0;
	sta++;
	if(sta>6)sta=0;	 											   
}
//�����ʱ��3�Ļص�����				  	   
void tmr3_callback(OS_TMR *ptmr,void *p_arg) 
{	
	u8* p;	 
	u8 err; 
	static u8 msg_cnt=0;	//msg���	  
	p=mymalloc(SRAMIN,13);	//����13���ֽڵ��ڴ�
	if(p)
	{
	 	sprintf((char*)p,"ALIENTEK %03d",msg_cnt);
		msg_cnt++;
		err=OSQPost(q_msg,p);	//���Ͷ���
		if(err!=OS_ERR_NONE) 	//����ʧ��
		{
			myfree(SRAMIN,p);	//�ͷ��ڴ�
			OSTmrStop(tmr3,OS_TMR_OPT_NONE,0,&err);	//�ر������ʱ��3
 		}
	}
} 
//����������   
void ucos_load_main_ui(void)
{
	delay_ms(300);
}	  


 int main(void)
 {	 		    
	delay_init();	    	 //��ʱ������ʼ��	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//�����ж����ȼ�����Ϊ��2��2λ��ռ���ȼ���2λ��Ӧ���ȼ�
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200

	LED_Init(LED1);
	KEY_Init(KEY1,BUTTON_MODE_GPIO);
	KEY_Init(KEY2,BUTTON_MODE_GPIO);
	KEY_Init(KEY3,BUTTON_MODE_GPIO);
	KEY_Init(KEY4,BUTTON_MODE_GPIO);	 
	BEEP_Init();
	
	ucos_load_main_ui(); 		//����������
	OSInit();  	 				//��ʼ��UCOSII
  	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//������ʼ����
	OSStart();	  
}

//��ʼ����
void start_task(void *pdata)
{
    OS_CPU_SR cpu_sr=0;
	u8 err;	    	    
	pdata = pdata; 	
	msg_key=OSMboxCreate((void*)0);		//������Ϣ����
	q_msg=OSQCreate(&MsgGrp[0],256);	//������Ϣ����
 	flags_key=OSFlagCreate(0,&err); 	//�����ź�����		  
	  
	OSStatInit();						//��ʼ��ͳ������.�������ʱ1��������	
 	OS_ENTER_CRITICAL();				//�����ٽ���(�޷����жϴ��)    
 	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);						   
 	OSTaskCreate(touch_task,(void *)0,(OS_STK*)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1],TOUCH_TASK_PRIO);	 				   
 	OSTaskCreate(qmsgshow_task,(void *)0,(OS_STK*)&QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE-1],QMSGSHOW_TASK_PRIO);	 				   
 	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);	 				   
 	OSTaskCreate(flags_task,(void *)0,(OS_STK*)&FLAGS_TASK_STK[FLAGS_STK_SIZE-1],FLAGS_TASK_PRIO);	 				   
 	OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);	 				   
 	OSTaskSuspend(START_TASK_PRIO);		//������ʼ����.
	OS_EXIT_CRITICAL();					//�˳��ٽ���(���Ա��жϴ��)
}
//LED����
void led_task(void *pdata)
{
	u8 t=0;
	while(1)
	{
		t++;
		delay_ms(10);
		if(t==8)LED1_STATE=1;	//LED0��
		if(t==100)		//LED0��
		{
			t=0;
			LED1_STATE=0;
		}
	}									 
}
//����������
void touch_task(void *pdata)
{	  	
	while(1)
	{
		delay_ms(5);	 
	}
}     
//������Ϣ��ʾ����
void qmsgshow_task(void *pdata)
{
	u8 *p;
	u8 err;
	while(1)
	{
		p=OSQPend(q_msg,0,&err);//������Ϣ����
 		myfree(SRAMIN,p);	  
		delay_ms(500);	 
	}									 
}
//������
void main_task(void *pdata)
{							 

	u8 err;	
 

  
 	tmr1=OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr1_callback,0,"tmr1",&err);		//100msִ��һ��
	tmr2=OSTmrCreate(10,20,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr2_callback,0,"tmr2",&err);		//200msִ��һ��
	tmr3=OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr3_callback,0,"tmr3",&err);		//100msִ��һ��
	OSTmrStart(tmr1,&err);//���������ʱ��1				 
	OSTmrStart(tmr2,&err);//���������ʱ��2				 
 	while(1)
	{
		delay_ms(10);
	}
}		   
//�ź�������������
void flags_task(void *pdata)
{	
	
	u8 err;	    						 
	while(1)
	{
		OSFlagPend(flags_key,0X001F,OS_FLAG_WAIT_SET_ANY,0,&err);//�ȴ��ź���

		BEEP_On();
		delay_ms(50);
		BEEP_Off();
		OSFlagPost(flags_key,0X001F,OS_FLAG_CLR,&err);//ȫ���ź�������
 	}
}
   		    
//����ɨ������
void key_task(void *pdata)
{	
	u8 key;		    						 
	while(1)
	{
		key=KEY_Scan();   
		if(key)OSMboxPost(msg_key,(void*)key);//������Ϣ
 		delay_ms(10);
	}
}
