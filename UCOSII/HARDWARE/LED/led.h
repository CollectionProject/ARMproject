#ifndef __LED_H
#define __LED_H	 
#include "sys.h"

/** ����Ϊö�����ͣ�������ָʾ��ʱ��չ **/
/** ָʾ�ƶ��� **/
typedef enum 
{
  LED1 = 0,
  LED2 = 1,
  LED3 = 2,
  LED4 = 3
} Led_TypeDef;

#ifndef ON
	#define ON  1
	#define OFF 0
#endif

/** ָʾ�ƹܽ���Դ��������  **/

#define LEDn                             4

/** LEDָʾ�ƹܽ���Դ����  ����͵�ƽ����ָʾ�� **/
#define LED1_PIN_NUM                     2 /* bitband������ʹ�ú궨��  */
#define LED1_PIN                         GPIO_Pin_2
#define LED1_GPIO_PORT                   GPIOD
#define LED1_GPIO_CLK                    RCC_APB2Periph_GPIOD  
#define LED1_STATE                          Periph_BB((uint32_t)&LED1_GPIO_PORT->ODR, LED1_PIN_NUM)//�ȼ���Periph_BB((uint32_t)&GPIOD->ODR, 2)

#define LED2_PIN_NUM                     3 
#define LED2_PIN                         GPIO_Pin_3
#define LED2_GPIO_PORT                   GPIOD
#define LED2_GPIO_CLK                    RCC_APB2Periph_GPIOD  
#define LED2_STATE                          Periph_BB((uint32_t)&LED2_GPIO_PORT->ODR, LED2_PIN_NUM)

#define LED3_PIN_NUM                     4 
#define LED3_PIN                         GPIO_Pin_4  
#define LED3_GPIO_PORT                   GPIOD
#define LED3_GPIO_CLK                    RCC_APB2Periph_GPIOD  
#define LED3_STATE                          Periph_BB((uint32_t)&LED3_GPIO_PORT->ODR, LED3_PIN_NUM)

#define LED4_PIN_NUM                     7
#define LED4_PIN                         GPIO_Pin_7  
#define LED4_GPIO_PORT                   GPIOD
#define LED4_GPIO_CLK                    RCC_APB2Periph_GPIOD  
#define LED4_STATE                          Periph_BB((uint32_t)&LED4_GPIO_PORT->ODR, LED4_PIN_NUM)


void LED_Init(Led_TypeDef Led);//��ʼ��

		 				    
#endif
