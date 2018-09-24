#ifndef __SYS_H
#define __SYS_H	
#include "stm32f10x.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/8/18
//版本：V1.7
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
////////////////////////////////////////////////////////////////////////////////// 	 

//0,不支持ucos
//1,支持ucos
#define SYSTEM_SUPPORT_OS		1	//定义系统文件夹是否支持UCOS
																	    
	 
/* 别名区宏定义 bitband macro ------------------------------------------------*/
/* 使用bitband可以提高设计bit操作的程序的效率，外设GPIO管脚的控制效果尤为明显 */
/* 外设别名区 对32MB外设别名区的访问映射为对1MB 外设bit-band区的访问(实际大小根据芯片有关) */
#define Periph_BASE         0x40000000  // 外设基地址 Peripheral 
#define Periph_BB_BASE      0x42000000  // 外设别名区基地址 Peripheral bitband

/* 注意：传入常量参数时，在编译时编译器会计算出别名区地址，这样能达到提升效率的目的(推荐)
         如果传入变量参数，只能在运行时由STM32自己计算别名区地址，效率会大打折扣(不推荐) */
#define Periph_BB(PeriphAddr, BitNumber)    \
          *(__IO uint32_t *) (Periph_BB_BASE | ((PeriphAddr - Periph_BASE) << 5) | ((BitNumber) << 2))
	 
#define Periph_ResetBit_BB(PeriphAddr, BitNumber)    \
          (*(__IO uint32_t *) (Periph_BB_BASE | ((PeriphAddr - Periph_BASE) << 5) | ((BitNumber) << 2)) = 0)
   
#define Periph_SetBit_BB(PeriphAddr, BitNumber)       \
          (*(__IO uint32_t *) (Periph_BB_BASE | ((PeriphAddr - Periph_BASE) << 5) | ((BitNumber) << 2)) = 1)

#define Periph_GetBit_BB(PeriphAddr, BitNumber)       \
          (*(__IO uint32_t *) (Periph_BB_BASE | ((PeriphAddr - Periph_BASE) << 5) | ((BitNumber) << 2)))

/* 外设GPIO输入输出别名区定义，n为bit位置范围为0到15    */
/* 我们对GPIOA.15操作，首先要初始化GPIO，然后使用方法： */
/* 对GPIOA.15输出低电平：   PAOutBit(15) = 0;           */
/* 对GPIOA.15输出低电平：   PAOutBit(15) = 1;           */
/* 读取GPIOA.15输入的电平： data = PAInBit(15);         */
#define PAOutBit(n)     Periph_BB((uint32_t)&GPIOA->IDR,n)  //输出 
#define PASetBit(n)     (PAOutBit(n) = 1)                   //输出 高
#define PAResetBit(n)   (PAOutBit(n) = 0)                   //输出 低
#define PAInBit(n)      Periph_BB((uint32_t)&GPIOA->IDR,n)  //输入 

#define PBOutBit(n)     Periph_BB((uint32_t)&GPIOB->ODR,n)  //输出 
#define PBSetBit(n)     (PBOutBit(n) = 1)                   //输出 高
#define PBResetBit(n)   (PBOutBit(n) = 0)                   //输出 低
#define PBInBit(n)      Periph_BB((uint32_t)&GPIOB->IDR,n)  //输入 

#define PCOutBit(n)     Periph_BB((uint32_t)&GPIOC->ODR,n)  //输出 
#define PCSetBit(n)     (PCOutBit(n) = 1)                   //输出 高
#define PCResetBit(n)   (PCOutBit(n) = 0)                   //输出 低
#define PCInBit(n)      Periph_BB((uint32_t)&GPIOC->IDR,n)  //输入 

#define PDOutBit(n)     Periph_BB((uint32_t)&GPIOD->ODR,n)  //输出 
#define PDSetBit(n)     (PDOutBit(n) = 1)                   //输出 高
#define PDResetBit(n)   (PDOutBit(n) = 0)                   //输出 低
#define PDInBit(n)      Periph_BB((uint32_t)&GPIOD->IDR,n)  //输入 

#define PEOutBit(n)     Periph_BB((uint32_t)&GPIOE->ODR,n)  //输出 
#define PESetBit(n)     (PEOutBit(n) = 1)                   //输出 高
#define PEResetBit(n)   (PEOutBit(n) = 0)                   //输出 低
#define PEInBit(n)      Periph_BB((uint32_t)&GPIOE->IDR,n)  //输入

#define PFOutBit(n)     Periph_BB((uint32_t)&GPIOF->ODR,n)  //输出 
#define PFSetBit(n)     (PFOutBit(n) = 1)                   //输出 高
#define PFResetBit(n)   (PFOutBit(n) = 0)                   //输出 低
#define PFInBit(n)      Periph_BB((uint32_t)&GPIOF->IDR,n)  //输入

#define PGOutBit(n)     Periph_BB((uint32_t)&GPIOG->ODR,n)  //输出 
#define PGSetBit(n)     (PGOutBit(n) = 1)                   //输出 高
#define PGResetBit(n)   (PGOutBit(n) = 0)                   //输出 低
#define PGInBit(n)      Periph_BB((uint32_t)&GPIOG->IDR,n)  //输入

//以下为汇编函数
void WFI_SET(void);		//执行WFI指令
void INTX_DISABLE(void);//关闭所有中断
void INTX_ENABLE(void);	//开启所有中断
void MSR_MSP(u32 addr);	//设置堆栈地址

#endif
