<<<<<<< HEAD
#ifndef _IWDG_H
#define _IWDG_H

#include "stm32f4xx.h"

void TableIWDG_Init(u8 prer, u16 rlr);
void TableIWDG_Enable(void);
void TableIWDG_Reload(void);

void BSP_Table_IWDGInit(uint32_t TimeoutFreMs);

#endif
=======
#ifndef _IWDG_H
#define _IWDG_H

#include "stm32f4xx.h"

void TableIWDG_Init(u8 prer, u16 rlr);
void TableIWDG_Enable(void);
void TableIWDG_Reload(void);

void BSP_Table_IWDGInit(uint32_t TimeoutFreMs);

#endif
>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
