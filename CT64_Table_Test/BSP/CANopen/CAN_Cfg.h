<<<<<<< HEAD
/*----------------------------------------------------------------------------
 *      R T L  -  C A N   D r i v e r
 *----------------------------------------------------------------------------
 *      Name:    CAN_Cfg.h
 *      Purpose: Header for STR91x CAN Configuration Settings
 *      Rev.:    V3.40
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2008 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#ifndef _CAN_CFG_H
#define _CAN_CFG_H

#include "applicfg.h"

/* 
// *** <<< Use Configuration Wizard in Context Menu >>> *** 
*/

/*
// <o>Number of transmit objects for controllers <1-1024>
// <i> Determines the size of software message buffer for transmitting.
// <i> Default: 20
*/
#define CAN_No_SendObjects     50

/*
// <o>Number of receive objects for controllers <1-1024>
// <i> Determines the size of software message buffer for receiving.
// <i> Default: 20
*/
#define CAN_No_ReceiveObjects  50

/*
// *** <<< End of Configuration section             >>> *** 
*/

/* Maximum index of used CAN controller 1 (only one CAN controller exists)
   Needed for memory allocation for CAN messages.                            */

#define CAN_CTRL_MAX_NUM      2


typedef enum
{
   STANDARD_FORMAT      = 0,
   EXTENDED_FORMAT
} CAN_FORMAT;

/** Used for the Can message structure */
/*
union SHORT_CAN {
  struct { u8 b0,b1; } b;
  u32 w;
};
*/

typedef struct {
  u32 w; // 32 bits
} SHORT_CAN;


/** Can message structure */
typedef struct {
  SHORT_CAN cob_id;	// l'ID du mesg
  u8 rtr;			// remote transmission request. 0 if not rtr, 
                                // 1 for a rtr message
  u8 len;			// message length (0 to 8)
  u8 data[8];  	// data 
} Message;

/* Error values that functions can return                                    */
typedef enum
{
   CAN_OK                                          = 0,
   /* No error                              */
   CAN_NOT_IMPLEMENTED_ERROR,
   /* Function has not been implemented     */
   CAN_MEM_POOL_INIT_ERROR,
   /* Memory pool initialization error      */
   CAN_BAUDRATE_ERROR,
   /* Baudrate was not set                  */
   CAN_TX_BUSY_ERROR,
   /* Transmitting hardware busy            */
   CAN_OBJECTS_FULL_ERROR,
   /* No more rx or tx objects available    */
   CAN_ALLOC_MEM_ERROR,
   /* Unable to allocate memory from pool   */
   CAN_DEALLOC_MEM_ERROR,
   /* Unable to deallocate memory           */
   CAN_TIMEOUT_ERROR,
   /* Timeout expired                       */
   CAN_UNEXIST_CTRL_ERROR,
   /* Controller does not exist             */
   CAN_UNEXIST_CH_ERROR
   /* Channel does not exist                */
}  CAN_ERROR;

#endif /* _CAN_CFG_H */


/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/

=======
/*----------------------------------------------------------------------------
 *      R T L  -  C A N   D r i v e r
 *----------------------------------------------------------------------------
 *      Name:    CAN_Cfg.h
 *      Purpose: Header for STR91x CAN Configuration Settings
 *      Rev.:    V3.40
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2008 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#ifndef _CAN_CFG_H
#define _CAN_CFG_H

#include "applicfg.h"

/* 
// *** <<< Use Configuration Wizard in Context Menu >>> *** 
*/

/*
// <o>Number of transmit objects for controllers <1-1024>
// <i> Determines the size of software message buffer for transmitting.
// <i> Default: 20
*/
#define CAN_No_SendObjects     50

/*
// <o>Number of receive objects for controllers <1-1024>
// <i> Determines the size of software message buffer for receiving.
// <i> Default: 20
*/
#define CAN_No_ReceiveObjects  50

/*
// *** <<< End of Configuration section             >>> *** 
*/

/* Maximum index of used CAN controller 1 (only one CAN controller exists)
   Needed for memory allocation for CAN messages.                            */

#define CAN_CTRL_MAX_NUM      2


typedef enum
{
   STANDARD_FORMAT      = 0,
   EXTENDED_FORMAT
} CAN_FORMAT;

/** Used for the Can message structure */
/*
union SHORT_CAN {
  struct { u8 b0,b1; } b;
  u32 w;
};
*/

typedef struct {
  u32 w; // 32 bits
} SHORT_CAN;


/** Can message structure */
typedef struct {
  SHORT_CAN cob_id;	// l'ID du mesg
  u8 rtr;			// remote transmission request. 0 if not rtr, 
                                // 1 for a rtr message
  u8 len;			// message length (0 to 8)
  u8 data[8];  	// data 
} Message;

/* Error values that functions can return                                    */
typedef enum
{
   CAN_OK                                          = 0,
   /* No error                              */
   CAN_NOT_IMPLEMENTED_ERROR,
   /* Function has not been implemented     */
   CAN_MEM_POOL_INIT_ERROR,
   /* Memory pool initialization error      */
   CAN_BAUDRATE_ERROR,
   /* Baudrate was not set                  */
   CAN_TX_BUSY_ERROR,
   /* Transmitting hardware busy            */
   CAN_OBJECTS_FULL_ERROR,
   /* No more rx or tx objects available    */
   CAN_ALLOC_MEM_ERROR,
   /* Unable to allocate memory from pool   */
   CAN_DEALLOC_MEM_ERROR,
   /* Unable to deallocate memory           */
   CAN_TIMEOUT_ERROR,
   /* Timeout expired                       */
   CAN_UNEXIST_CTRL_ERROR,
   /* Controller does not exist             */
   CAN_UNEXIST_CH_ERROR
   /* Channel does not exist                */
}  CAN_ERROR;

#endif /* _CAN_CFG_H */


/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/

>>>>>>> e3f6a3410b8ad0ffd2f42831f118e55ba176fe1c
