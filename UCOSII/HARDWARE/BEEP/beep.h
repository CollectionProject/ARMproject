#ifndef __BEEP_H
#define __BEEP_H	 
#include "sys.h"

/** �������ܽ���Դ����     ����͵�ƽ���������� **/
#define BEEP_PIN_NUM                     3 
#define BEEP_PIN                         GPIO_Pin_3    
#define BEEP_GPIO_PORT                   GPIOA    
#define BEEP_GPIO_CLK                    RCC_APB2Periph_GPIOA
#define BEEPOBB                          Periph_BB((uint32_t)&BEEP_GPIO_PORT->ODR, BEEP_PIN_NUM)
  

void BEEP_Init(void);	//��ʼ��
void BEEP_On(void);
void BEEP_Off(void);							
#endif

