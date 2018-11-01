#include "pld.h"
#include "spi.h"
#include "ucos_ii.h"
#include "stdlib.h"
#include "table_gpio.h"
#include "math.h"

/*pld¹Ü½ÅÉèÖÃ
  pldbit0---pd4
  pldbit1---pd5
  pldbit2---pd6
  pldbit3---pd7

  */
void PLD_Init(void)
{	 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(	RCC_APB2Periph_GPIOD, ENABLE );	
		 

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
		
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7 | GPIO_Pin_6;   //  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
		
	PLD_RST = 0;
}  
	
u8 Read_Byte(u8 Cmd)
{
	u8 spi_rx=0;
	
  spi_rx = SPIx_ReadWriteByte(Cmd);
	spi_rx = SPIx_ReadWriteByte(Cmd);
	OSTimeDly(1);

	return spi_rx;
}



u32 Gray2Decimal_32(u32 x)
{
	u32 y;
	y = x;
	
	while(x>>=1)
		y^=x;
	
	return y;
}

u32 Read_Encoder(u8 Object)
{
	u8 b0,b1,b2=0;
	u32 y;
	b0=Read_Byte(Object);
	b1=Read_Byte(Object + 1);
	b2=Read_Byte(Object + 2);
	y=(u32)b0+(u32)(b1<<8)+(u32)(b2<<16);
	
	if (Object == 0x30)
	{
		y = Gray2Decimal_32(y);
	}
	else
		y = Gray2Decimal_32(y);
		
	return y;	
}

double Read_Encoder_Position(u8 Encoder_Object)
{
	u32 ReadResult = 0x00;
	double PosResult= 0.00;
	
	switch(Encoder_Object)
	{
		case 0x30: //horizontal absolute encoder
		{
			ReadResult = Read_Encoder(0x30);
			PosResult  = (double)(((double)ReadResult*6.127)/(double)10);
			break;
		}
		case 0x20: //vertical absolute encoder
		{
			ReadResult = Read_Encoder(0x20);
			PosResult  = sin((((double)ReadResult/(double)4096)/(double)5)*2*3.1415926+ 0.2077)*800.00 + 190.00 +140.00;
			break;
		}
		case 0x40: //horizontal incremental encoder
		{
			ReadResult = Read_Encoder(0x40);
			break;
		}
		default:
			break;
	}
	
	return PosResult;
}
	
u8 Read_PLD_IO(void)
{
	u8 y;
	y=Read_Byte(3);
	return y;
}
	
u8 PLD_Self_test(void)
{
	u8 y,r;
	y=Read_Byte(4);
	if (y==0x5A)
		r=0xFF;
	else
		r=0x00;
		return r;
}
	
