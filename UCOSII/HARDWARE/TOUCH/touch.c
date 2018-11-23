#include "touch.h" 
#include "bsp_lcd.h"
#include "delay.h"
#include "stdlib.h"
#include "math.h"
#include "24cxx.h" 
uint32_t TSC_Value_X;
uint32_t TSC_Value_Y;
__IO uint8_t touch_done = 0;

void TS_GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /* Enable TouchScreen_CS clock */
    RCC_APB2PeriphClockCmd(TS_CS_GPIO_CLK , ENABLE);

    /* TouchScreen_CS pins configuration */
    GPIO_InitStructure.GPIO_Pin = TS_CS_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(TS_CS_GPIO_PORT, &GPIO_InitStructure);


    /* Enable TouchScreen_INT clock */
    RCC_APB2PeriphClockCmd(TS_INT_GPIO_CLK , ENABLE);

    /* TouchScreen_INT pins configuration */
    GPIO_InitStructure.GPIO_Pin = TS_INT_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(TS_INT_GPIO_PORT, &GPIO_InitStructure);  
}

  
void TS_SPI_Config(void) 
{ 
    GPIO_InitTypeDef  GPIO_InitStructure; 
    SPI_InitTypeDef   SPI_InitStructure; 

    /* Enable TouchScreen SPI clock */
    RCC_APB2PeriphClockCmd(RCC_TS_SPI,ENABLE);   
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI3,ENABLE);

    /* Configure TouchScreen SPI pins: SCK and MOSI  */
    GPIO_InitStructure.GPIO_Pin = TS_SPI_SCK | TS_SPI_MOSI; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;   //复用推挽输出
    GPIO_Init(TS_SPI_PORT,&GPIO_InitStructure);  

    /* Configure TouchScreen SPI pins: MISO */
    GPIO_InitStructure.GPIO_Pin = TS_SPI_MISO; 
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;     //浮空输入或带上拉输入
    GPIO_Init(TS_SPI_PORT,&GPIO_InitStructure); 

    /* Enable the TS_SPI Pins Software Remapping */
    /* 使能STM32的SPI3的管脚重映射 */
    GPIO_PinRemapConfig(GPIO_Remap_SPI3, ENABLE);
        
    /* Configure TouchScreen SPI */ 
    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex; 
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master; 
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b; 
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low; 
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge; 
    SPI_InitStructure.SPI_NSS  = SPI_NSS_Soft;
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16; 
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB; 
    SPI_InitStructure.SPI_CRCPolynomial = 7; 
    SPI_Init(TS_SPI,&SPI_InitStructure); 

    /* enable TouchScreen SPI */ 
    SPI_Cmd(TS_SPI, ENABLE);  
}

/*=====================================================================*/
unsigned char TS_SPI_ReadWriteByte(unsigned char data) 
{ 
    unsigned char Data = 0; 

    //Wait until the transmit buffer is empty 
    while(SPI_I2S_GetFlagStatus(TS_SPI,SPI_I2S_FLAG_TXE)==RESET); 
    // Send the byte  
    SPI_I2S_SendData(TS_SPI, data); 

    //Wait until a data is received 
    while(SPI_I2S_GetFlagStatus(TS_SPI,SPI_I2S_FLAG_RXNE)==RESET); 
    // Get the received data 
    Data = SPI_I2S_ReceiveData(TS_SPI); 

    // Return the shifted data 
    return Data; 
}  


/*=====================================================================*/
uint16_t _AD2Y(uint16_t adx) //240
{
  uint16_t sx=0;
  int r = adx - 200;
  r *= 240;
  sx=r / (4000 - 280);
  if (sx<=0 || sx>240)
    return 0;
	else
		sx=240-sx;
  return sx;
}

/*=====================================================================*/
uint16_t _AD2X(uint16_t ady) //320
{
  uint16_t sy=0;
  int r = ady - 265;
  r *= 320;
  sy=r/(3960 - 360);
  if (sy<=0 || sy>320)
    return 0;
	else
		sy = 320-sy;
  return sy;
}
/*=====================================================================*/
uint16_t TPReadX(void)//TS_ReadX(void)
{ 
   uint16_t x=0;
   TS_CS_LOW();
   TS_SPI_ReadWriteByte(CHX);
   x=TS_SPI_ReadWriteByte(0x00);
   x<<=8;
   x+=TS_SPI_ReadWriteByte(0x00);
   TS_CS_HIGH(); 
   x = x>>3;    //右对齐
   return (x);
}

/*=====================================================================*/
uint16_t TPReadY(void)
{
  uint16_t y=0;
  TS_CS_LOW();

  TS_SPI_ReadWriteByte(CHY);
  y=TS_SPI_ReadWriteByte(0x00);
  y<<=8;
  y+=TS_SPI_ReadWriteByte(0x00);
  TS_CS_HIGH();
  y = y>>3;     //右对齐
  return (y);
}

/*=====================================================================*/
#define  times  4
static void ADS7843_Rd_Addata(uint16_t *X_Addata,uint16_t *Y_Addata)
{

	uint16_t		i,j,k,x_addata[times],y_addata[times];
    
	for(i=0;i<times;i++)					//采样4次.
	{
		x_addata[i] = TPReadX();
		y_addata[i] = TPReadY();
	}

	for(i=0;i<times;i++)
	{
    	for(j=times;j<times-1;j++)
    	{
           if(x_addata[j] > x_addata[i])
            {
                k = x_addata[j];
                x_addata[j] = x_addata[i];
                x_addata[i] = k;
            }
         }
    }

	for(i=0;i<times;i++)
	{
    	for(j=times;j<times-1;j++)
    	{
           if(y_addata[j] > y_addata[i])
            {
                k = y_addata[j];
                y_addata[j] = y_addata[i];
                y_addata[i] = k;
            }
         }
    }
	
	
	*X_Addata=(x_addata[1] + x_addata[2]) >> 1;
	*Y_Addata=(y_addata[1] + y_addata[2]) >> 1;

}


/*=====================================================================*/
/*=====================================================================*/
uint16_t distence(uint16_t data1,uint16_t data2)
{
    if((data2 > data1 + 2) || (data1 > data2 + 2))
    {
        return 0;
    }

    return 1;    
}


/*=====================================================================*/
void SZ_TS_Read(void)
{
    uint16_t xdata, ydata;
    uint32_t xScreen, yScreen;

    static uint16_t sDataX,sDataY;

    ADS7843_Rd_Addata(&xdata, &ydata);
    xScreen = _AD2X(xdata);
    yScreen = _AD2Y(ydata);

    //xScreen = 320 - ((ydata*320)>>12);
    //yScreen = (xdata*240)>>12;


    //printf("\n\r (0x%x, 0x%x), (%d, %d)", xdata, ydata, xScreen, yScreen);
    if((xScreen>1)&&(yScreen>1)&&(xScreen<320-1)&&(yScreen<240-1))
    {
        //printf("\n\r%d,%d", xScreen, yScreen);
        //printf("\n\r%d,%d", xScreen, yScreen);
        if((TS_INT_VALID) && distence(sDataX,xScreen) && distence(sDataY,yScreen))
        {
						delay_ms(15);
						if((TS_INT_VALID) && distence(sDataX,xScreen) && distence(sDataY,yScreen))
						{
								//LCD_BigPoint(yScreen, xScreen);
								TSC_Value_X = xScreen;
								TSC_Value_Y = yScreen;
								touch_done = 1;
						}
        }

        sDataX = xScreen;
        sDataY = yScreen;
        
		}
}

/*=====================================================================*/

void SZ_TS_Init(void)
{
    TS_GPIO_Config();
    TS_CS_HIGH();   
    TS_SPI_Config();
}
