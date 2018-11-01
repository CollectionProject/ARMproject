/****************************************************
**File name: 	   ems_handle.c
**Description：  PE8 interrupt handle for EMS 
**Editor:        HuYu
**Company:       Fullshare Tech
**Date:          2012-08-16
**Change items： 
****************************************************/
#include "table_gpio.h"
#include "bsp.h"


/********************Timer**************************/
extern volatile u8 TimeTick;
extern volatile GLOBALSTATE GlobalState;
volatile u8 EMStop = 0;

void EXTI9_5_IRQHandler (void) 
{
	if (EXTI_GetITStatus(EXTI_Line8)!=RESET)
	{
		EXTI_ClearITPendingBit(EXTI_Line8);
		
		if (EMS==1)                //EMS PRESSED
    {		
			EMStop = 0x01;
			H_BRAKE = SET_H_BRAKE;
			
			V_BRAKE = SET_V_BRAKE;
		}
    else
		{
			EMStop  = 0x00;          //EMS RELEASED
			H_BRAKE = CLR_H_BRAKE;

			V_BRAKE = CLR_V_BRAKE;
		} 
	}	
} 

void Exit_Configuration(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
  //外部中断使用的RCC时钟
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);//使能GPIO时钟  

  //外部中断使用的GPIO配置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_Init(GPIOD, &GPIO_InitStructure);  
	
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);//使能SYSCFG时钟
  /* Connect EXTI Line8 to PD.8 */
  SYSCFG_EXTILineConfig(EXTI_PortSourceGPIOD, EXTI_PinSource8);

  /* Configure EXTI Line12 to generate an interrupt on falling edge */
  //配置外部中断  
  EXTI_InitStructure.EXTI_Line = EXTI_Line8;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  //外部中断使用的NVIC配置
  /* Configure one bit for preemption priority */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  
  /* Enable the EXTI9_5 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

/************************end***********************/
