#ifndef _IWDG_H
#define _IWDG_H

#include "stm32f4xx.h"

void TableIWDG_Init(u8 prer, u16 rlr);
void TableIWDG_Enable(void);
void TableIWDG_Reload(void);

void BSP_Table_IWDGInit(uint32_t TimeoutFreMs);

#endif
