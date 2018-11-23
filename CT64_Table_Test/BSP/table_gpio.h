<<<<<<< HEAD
#ifndef _GPIO_TABLE_H_
#define _GPIO_TABLE_H_

#include "stm32f4xx.h"


//-----------------------------------------------------------------------------------------------------
//ADDRESS=0x4200 0000 + (0x0001 100C*0x20) + (bitx*4)	;
//
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
//
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr))

#define BIT_ADDR(addr, bitnum)   MEM_ADDR( BITBAND(addr, bitnum)  )

#define GPIOA_ODR    (GPIOA_BASE+20) //0x4001080C
#define GPIOB_ODR    (GPIOB_BASE+20)//0x40010C0C
#define GPIOC_ODR    (GPIOC_BASE+20) //0x4001100C
#define GPIOD_ODR    (GPIOD_BASE+20) //0x4001140C
#define GPIOE_ODR    (GPIOE_BASE+20) //0x4001180C

#define GPIOA_IDR    (GPIOA_BASE+16)  //0x4001080C
#define GPIOB_IDR    (GPIOB_BASE+16)  //0x40010C0C
#define GPIOC_IDR    (GPIOC_BASE+16)  //0x4001100C
#define GPIOD_IDR    (GPIOD_BASE+16)  //0x4001140C
#define GPIOE_IDR    (GPIOE_BASE+16)  //0x4001180C






	#define home_reached       	BIT_ADDR(GPIOC_ODR, 7) //PC7

//	#define PLD_RST       	BIT_ADDR(GPIOB_ODR, 11) //PB11
	#define HORI_LS_LEFT    BIT_ADDR(GPIOD_IDR, 10) //PD10
	#define HORI_LS_RIGHT		BIT_ADDR(GPIOD_IDR, 11) //PD11
	#define VERT_LS_TOP			BIT_ADDR(GPIOD_IDR, 12) //PD12
	#define VERT_LS_BOTTOM	BIT_ADDR(GPIOD_IDR, 13) //PD13
	
	#define H_CLUTCH  			BIT_ADDR(GPIOA_ODR, 6)  //PA6
	#define H_BRAKE			  	BIT_ADDR(GPIOA_ODR, 7)  //PA7
	
	#define FOOT_UP       	BIT_ADDR(GPIOD_IDR, 9)  //PD9
	#define FOOT_DOWN     	BIT_ADDR(GPIOA_IDR, 8)  //PA8
	#define FOOT_XRAY     	BIT_ADDR(GPIOD_IDR, 14) //PD14
	
	#define EMS           	BIT_ADDR(GPIOD_IDR, 8)  //PD8


	#define V_BRAKE 		  BIT_ADDR(GPIOA_ODR, 5)  //PA5

	
	#define HORI_LED 		  	BIT_ADDR(GPIOA_ODR, 11) //PA11
	#define VERT_LED 		  	BIT_ADDR(GPIOA_ODR, 12) //PA12







	#define CLR_H_CLUTCH    0x01                  //   
	#define SET_H_CLUTCH    0x00                  //	


//  #define CLR_H_BRAKE     0x00                  //	nanjing²âÊÔ»úÆ÷	
//  #define SET_H_BRAKE     0x01                  //		
 #define CLR_H_BRAKE     0x01                  //	p1	
 #define SET_H_BRAKE     0x00                  //		

#define CLR_V_BRAKE     0x01                  //		
#define SET_V_BRAKE     0x00                  //		


void Table_GPIO_Init(void);


#endif
=======
#ifndef _GPIO_TABLE_H_
#define _GPIO_TABLE_H_

#include "stm32f4xx.h"


//-----------------------------------------------------------------------------------------------------
//ADDRESS=0x4200 0000 + (0x0001 100C*0x20) + (bitx*4)	;
//
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
//
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr))

#define BIT_ADDR(addr, bitnum)   MEM_ADDR( BITBAND(addr, bitnum)  )

#define GPIOA_ODR    (GPIOA_BASE+20) //0x4001080C
#define GPIOB_ODR    (GPIOB_BASE+20)//0x40010C0C
#define GPIOC_ODR    (GPIOC_BASE+20) //0x4001100C
#define GPIOD_ODR    (GPIOD_BASE+20) //0x4001140C
#define GPIOE_ODR    (GPIOE_BASE+20) //0x4001180C

#define GPIOA_IDR    (GPIOA_BASE+16)  //0x4001080C
#define GPIOB_IDR    (GPIOB_BASE+16)  //0x40010C0C
#define GPIOC_IDR    (GPIOC_BASE+16)  //0x4001100C
#define GPIOD_IDR    (GPIOD_BASE+16)  //0x4001140C
#define GPIOE_IDR    (GPIOE_BASE+16)  //0x4001180C






	#define home_reached       	BIT_ADDR(GPIOC_ODR, 7) //PC7

//	#define PLD_RST       	BIT_ADDR(GPIOB_ODR, 11) //PB11
	#define HORI_LS_LEFT    BIT_ADDR(GPIOD_IDR, 10) //PD10
	#define HORI_LS_RIGHT		BIT_ADDR(GPIOD_IDR, 11) //PD11
	#define VERT_LS_TOP			BIT_ADDR(GPIOD_IDR, 12) //PD12
	#define VERT_LS_BOTTOM	BIT_ADDR(GPIOD_IDR, 13) //PD13
	
	#define H_CLUTCH  			BIT_ADDR(GPIOA_ODR, 6)  //PA6
	#define H_BRAKE			  	BIT_ADDR(GPIOA_ODR, 7)  //PA7
	
	#define FOOT_UP       	BIT_ADDR(GPIOD_IDR, 9)  //PD9
	#define FOOT_DOWN     	BIT_ADDR(GPIOA_IDR, 8)  //PA8
	#define FOOT_XRAY     	BIT_ADDR(GPIOD_IDR, 14) //PD14
	
	#define EMS           	BIT_ADDR(GPIOD_IDR, 8)  //PD8


	#define V_BRAKE 		  BIT_ADDR(GPIOA_ODR, 5)  //PA5

	
	#define HORI_LED 		  	BIT_ADDR(GPIOA_ODR, 11) //PA11
	#define VERT_LED 		  	BIT_ADDR(GPIOA_ODR, 12) //PA12







	#define CLR_H_CLUTCH    0x01                  //   
	#define SET_H_CLUTCH    0x00                  //	


//  #define CLR_H_BRAKE     0x00                  //	nanjing²âÊÔ»úÆ÷	
//  #define SET_H_BRAKE     0x01                  //		
 #define CLR_H_BRAKE     0x01                  //	p1	
 #define SET_H_BRAKE     0x00                  //		

#define CLR_V_BRAKE     0x01                  //		
#define SET_V_BRAKE     0x00                  //		


void Table_GPIO_Init(void);


#endif
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
