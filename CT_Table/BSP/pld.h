#ifndef __PLD_H
#define __PLD_H
#include "stm32f10x.h"

#define pldbit0 GPIO_Pin_4
#define pldbit1 GPIO_Pin_5
#define pldbit2 GPIO_Pin_6
#define pldbit3 GPIO_Pin_7
						  	    													  
void PLD_Init(void);  
u8 Read_Byte(u8);
double Read_Encoder_Position(u8 Encoder_Object);
u8 Read_PLD_IO(void);
u8 PLD_Self_test(void);
#endif
