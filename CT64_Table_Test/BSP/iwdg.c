<<<<<<< HEAD
/****************************************************
**File name: 	   ems_handle.c
**Description：  PE8 interrupt handle for EMS 
**Editor:        HuYu
**Company:       Fullshare Tech
**Date:          2012-08-16
**Change items： 
****************************************************/
#include "iwdg.h"


void TableIWDG_Init(u8 prer, u16 rlr)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	IWDG_SetPrescaler(prer);
	IWDG_SetReload(rlr);
	IWDG_ReloadCounter();
}

void TableIWDG_Enable(void)
{
	IWDG_Enable();
}
void TableIWDG_Reload(void)
{
	IWDG_ReloadCounter();
}

/******************************************************************** 
 函 数 名 : void BSP_Table_IWDGInit(uint32_t TimeoutFreMs)
 功    能 ：
 说    明 看门狗初始化
        
 入口参数：
          uint32_t TimeoutFreMs：  延时定时时间         
          
 返 回 值：无  
 设    计：王军             日    期：2015-12-21
 修    改：                 日    期： 
***********************************************************************/
void BSP_Table_IWDGInit(uint32_t TimeoutFreMs)/* 2 -  3276*/
{
    uint16_t ReloadValue;
	
	  RCC_LSICmd(ENABLE);                             
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY)==RESET);  

    /* IWDG timeout equal to 280 ms (the timeout may varies due to LSI frequency
    dispersion) */
    /* Enable write access to IWDG_PR and IWDG_RLR registers */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    /* IWDG counter clock: 40KHz(LSI) / 32 = 1.25 KHz */
    IWDG_SetPrescaler(IWDG_Prescaler_32);

    /* Set counter reload value to 349 *//* range from 0 to 4095 */
    //IWDG_SetReload(349);
    if(TimeoutFreMs < 2)
    {
        TimeoutFreMs = 2;
    }

    if(TimeoutFreMs > 3276)
    {
        TimeoutFreMs = 3276;
    }

    ReloadValue = ((TimeoutFreMs * 1250)/1000) - 1;
    IWDG_SetReload(ReloadValue);
  
    /* Reload IWDG counter */
    IWDG_ReloadCounter();

    /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
    IWDG_Enable();

}




/************************end***********************/


=======
/****************************************************
**File name: 	   ems_handle.c
**Description：  PE8 interrupt handle for EMS 
**Editor:        HuYu
**Company:       Fullshare Tech
**Date:          2012-08-16
**Change items： 
****************************************************/
#include "iwdg.h"


void TableIWDG_Init(u8 prer, u16 rlr)
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);
	IWDG_SetPrescaler(prer);
	IWDG_SetReload(rlr);
	IWDG_ReloadCounter();
}

void TableIWDG_Enable(void)
{
	IWDG_Enable();
}
void TableIWDG_Reload(void)
{
	IWDG_ReloadCounter();
}

/******************************************************************** 
 函 数 名 : void BSP_Table_IWDGInit(uint32_t TimeoutFreMs)
 功    能 ：
 说    明 看门狗初始化
        
 入口参数：
          uint32_t TimeoutFreMs：  延时定时时间         
          
 返 回 值：无  
 设    计：王军             日    期：2015-12-21
 修    改：                 日    期： 
***********************************************************************/
void BSP_Table_IWDGInit(uint32_t TimeoutFreMs)/* 2 -  3276*/
{
    uint16_t ReloadValue;
	
	  RCC_LSICmd(ENABLE);                             
    while(RCC_GetFlagStatus(RCC_FLAG_LSIRDY)==RESET);  

    /* IWDG timeout equal to 280 ms (the timeout may varies due to LSI frequency
    dispersion) */
    /* Enable write access to IWDG_PR and IWDG_RLR registers */
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    /* IWDG counter clock: 40KHz(LSI) / 32 = 1.25 KHz */
    IWDG_SetPrescaler(IWDG_Prescaler_32);

    /* Set counter reload value to 349 *//* range from 0 to 4095 */
    //IWDG_SetReload(349);
    if(TimeoutFreMs < 2)
    {
        TimeoutFreMs = 2;
    }

    if(TimeoutFreMs > 3276)
    {
        TimeoutFreMs = 3276;
    }

    ReloadValue = ((TimeoutFreMs * 1250)/1000) - 1;
    IWDG_SetReload(ReloadValue);
  
    /* Reload IWDG counter */
    IWDG_ReloadCounter();

    /* Enable IWDG (the LSI oscillator will be enabled by hardware) */
    IWDG_Enable();

}




/************************end***********************/


>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
