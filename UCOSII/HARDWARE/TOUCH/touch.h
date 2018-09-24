#ifndef __TOUCH_H__
#define __TOUCH_H__
#include "sys.h"
#include "ott2001a.h"	    
#include "gt9147.h"	    
#include "ft5206.h"	    

#define SIZE_3_2INCH_TFT_LCD
//#define SIZE_2_8INCH_TFT_LCD


// A/D 通道选择命令字和工作寄存器
#define	CHX 	0x90 //0x90 	//通道Y+的选择控制字	//0x94
#define	CHY 	0xD0 //0xd0	    //通道X+的选择控制字	//0xD4


/* SPI3 Remap pins definition for test */
#define TS_SPI_PORT                 GPIOC
#define TS_SPI_SCK                  GPIO_Pin_10
#define TS_SPI_MISO                 GPIO_Pin_11
#define TS_SPI_MOSI                 GPIO_Pin_12
#define RCC_TS_SPI                  RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO
#define TS_SPI                      SPI3

/* TouchScreen_CS */
#define TS_CS_PIN_NUM               9 /* bitband别名区使用宏定义  */
#define TS_CS_PIN                   GPIO_Pin_9
#define TS_CS_GPIO_PORT             GPIOC
#define TS_CS_GPIO_CLK              RCC_APB2Periph_GPIOC  
#define TS_CS_OBB                   Periph_BB((uint32_t)&TS_CS_GPIO_PORT->ODR, TS_CS_PIN_NUM)
/* Select TouchScreen SPI:   ChipSelect pin low  */
#define TS_CS_LOW()                 TS_CS_OBB = 0
/* Deselect TouchScreen SPI: ChipSelect pin high */
#define TS_CS_HIGH()                TS_CS_OBB = 1


/* TouchScreen_INT */
#define TS_INT_PIN_NUM              5 
#define TS_INT_PIN                  GPIO_Pin_5
#define TS_INT_GPIO_PORT            GPIOC
#define TS_INT_GPIO_CLK             RCC_APB2Periph_GPIOC
#define TS_INT_EXTI_LINE            EXTI_Line5
#define TS_INT_EXTI_PORT_SOURCE     GPIO_PortSourceGPIOC
#define TS_INT_EXTI_PIN_SOURCE      GPIO_PinSource5
#define TS_INT_EXTI_IRQn            EXTI9_5_IRQn  
#define TS_INT_IBB                  Periph_BB((uint32_t)&TS_INT_GPIO_PORT->IDR, TS_INT_PIN_NUM) //等价于Periph_BB((uint32_t)&GPIOC->IDR, 4)
#define TS_INT_VALID                (!TS_INT_IBB)

extern uint32_t TSC_Value_X;
extern uint32_t TSC_Value_Y;
extern __IO uint8_t touch_done;


void SZ_TS_Init(void);
void SZ_TS_Read(void);
 
#endif

















