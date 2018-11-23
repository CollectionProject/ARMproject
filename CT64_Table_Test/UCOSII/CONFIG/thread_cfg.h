<<<<<<< HEAD
#ifndef  _APP_CFG_H_
#define  _APP_CFG_H_

/* task priority */
#define STARTUP_TASK_PRIO                   2   
#define TABLEINIT_TSK_PRIO                  3   
#define TABLEWATCHDOG_TASK_PRIO             4
#define CAN_TRANSMIT_MUTEX_PRIO             5
#define CAN2_TRANSMIT_MUTEX_PRIO            6
#define MONITORCANOPEN_TSK_PRIO             7  
#define DISABLE_TASK_PRIO                   8
#define MONITORSC_TSK_PRIO                  9   
#define MONITORUART_TSK_PRIO                10   
#define POST_TASK_PRIO                      11
#define HORITABLECONTROL_TASK_PRIO          12
#define VERTTABLECONTROL_TASK_PRIO          13
#define HORIMOTION_TASK_PRIO                14
#define VERTMOTION_TASK_PRIO                15


/* task stack size */
#define STARTUP_TASK_STK_SIZE                  128
#define TASK_MONITORSC_STK_SIZE                512  
#define TASK_MONITORCANOPEN_STK_SIZE           256
#define TABLEINIT_TASK_STK_SIZE  							 256  
#define TABLE_WATCHDOG_TASK_STK_SIZE           256
#define POST_TASK_STK_SIZE                     256
#define HORIMOTION_TASK_STK_SIZE               256
#define VERTMOTION_TASK_STK_SIZE               256
#define DISABLE_TASK_STK_SIZE			             256
#define HORITABLECONTROL_TASK_STK_SIZE         256
#define VERTTABLECONTROL_TASK_STK_SIZE         256

/* interrupt priority */

#endif

=======
#ifndef  _APP_CFG_H_
#define  _APP_CFG_H_

/* task priority */
#define STARTUP_TASK_PRIO                   2   
#define TABLEINIT_TSK_PRIO                  3   
#define TABLEWATCHDOG_TASK_PRIO             4
#define CAN_TRANSMIT_MUTEX_PRIO             5
#define CAN2_TRANSMIT_MUTEX_PRIO            6
#define MONITORCANOPEN_TSK_PRIO             7  
#define DISABLE_TASK_PRIO                   8
#define MONITORSC_TSK_PRIO                  9   
#define MONITORUART_TSK_PRIO                10   
#define POST_TASK_PRIO                      11
#define HORITABLECONTROL_TASK_PRIO          12
#define VERTTABLECONTROL_TASK_PRIO          13
#define HORIMOTION_TASK_PRIO                14
#define VERTMOTION_TASK_PRIO                15


/* task stack size */
#define STARTUP_TASK_STK_SIZE                  128
#define TASK_MONITORSC_STK_SIZE                512  
#define TASK_MONITORCANOPEN_STK_SIZE           256
#define TABLEINIT_TASK_STK_SIZE  							 256  
#define TABLE_WATCHDOG_TASK_STK_SIZE           256
#define POST_TASK_STK_SIZE                     256
#define HORIMOTION_TASK_STK_SIZE               256
#define VERTMOTION_TASK_STK_SIZE               256
#define DISABLE_TASK_STK_SIZE			             256
#define HORITABLECONTROL_TASK_STK_SIZE         256
#define VERTTABLECONTROL_TASK_STK_SIZE         256

/* interrupt priority */

#endif

>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
