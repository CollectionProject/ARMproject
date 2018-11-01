#ifndef __SPI_H
#define __SPI_H
#include "stm32f4xx.h"


						  	    													  
void SPI2_Init(void);			 //初始化SPI口
void SPI1_Set_NSS(void);
void SPI1_Reset_NSS(void);
  
u8 SPIx_ReadWriteByte(u8 TxData);//SPI总线读写一个字节
		 
#endif

