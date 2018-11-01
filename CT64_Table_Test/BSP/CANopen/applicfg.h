/*********************************************************                   
 *                                                       *
 *             Master/slave CANopen Library              *
 *                                                       *
 *  LIVIC : Laboratoire Interractions Véhicule           * 
 *          Infrastructure Conducteur                    *
 *                       ----                            *
 *  INRETS/LIVIC : http://www.inrets.fr                  *
 *      Institut National de Recherche sur               *
 *      les Transports                                   *
 *      et leur Sécurité                                 *
 *  LCPC Laboratoire Central des Ponts et Chaussées      *
 *  Laboratoire Interractions Véhicule Infrastructure    *
 *  Conducteur                                           *
 *                                                       *
 *  Authors  : Camille BOSSARD                           *
 *             Francis DUPIN                             *
 *             Laurent Romieux                           *
 *             Zakaria BELAMRI                           *
 *  Contact : bossard.ca@voila.fr                        *
 *            francis.dupin@inrets.fr                    *
 *            ZAKARIA_BELAMRI@HOTMAIL.COM                *
 *  Date    : 2003                                       *
 * This work is based on                                 *
 * -     CanOpenMatic by  Edouard TISSERANT              *
 *       http://sourceforge.net/projects/canfestival/    * 
 * -     slavelib by    Raphael Zulliger                 *
 *       http://sourceforge.net/projects/canopen/        *
 *********************************************************
 *                                                       *
 *********************************************************
 * This program is free software; you can redistribute   *
 * it and/or modify it under the terms of the GNU General*
 * Public License as published by the Free Software      *
 * Foundation; either version 2 of the License, or (at   *
 * your option) any later version.                       *
 *                                                       *
 * This program is distributed in the hope that it will  *
 * be useful, but WITHOUT ANY WARRANTY; without even the *
 * implied warranty of MERCHANTABILITY or FITNESS FOR A  *
 * PARTICULAR PURPOSE.  See the GNU General Public       *
 * License for more details.                             *
 *                                                       *
 * You should have received a copy of the GNU General    *
 * Public License along with this program; if not, write *
 * to 	The Free Software Foundation, Inc.               *
 *	675 Mass Ave                                     *
 *	Cambridge                                        *
 *	MA 02139                                         *
 * 	USA.                                             *
 *********************************************************
           File : applicfg.h
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/

#ifndef __APPLICFG__
#define __APPLICFG__

#include "stm32f4xx.h"

/// Define the architecture : little_endian or big_endian
// -----------------------------------------------------
// Test :
// u32 v = 0x1234ABCD;
// char *data = &v;
//
// Result for a little_endian architecture :
// data[0] = 0xCD;
// data[1] = 0xAB;
// data[2] = 0x34;
// data[3] = 0x12;
//
// Result for a big_endian architecture :
// data[0] = 0x12;
// data[1] = 0x34;
// data[2] = 0xAB;
// data[3] = 0xCD;

// little_endian (Intel CPU)
//#define CANOPEN_LITTLE_ENDIAN

// big_endian (Motorola, CS12)
#define CANOPEN_BIG_ENDIAN

// CANOPEN_LITTLE_ENDIAN (INTEL)
//#define CANOPEN_LITTLE_ENDIAN


// If defined, code is generated to print messages to the console
// (short or large message, depending on DEBUG_CAN)
// Should be defined
//#define DEBUG_ERR_CONSOLE_ON
//#define DEBUG_WAR_CONSOLE_ON

// If defined, a code is generated for large debuging messages on the console.
// Should not be defined. (waste time and memory)
//#define DEBUG_CAN

//for sending messages PDO_error 

#define  PDO_ERROR


// Use or not the PLL
//#define USE_PLL

#ifdef USE_PLL
#  define BUS_CLOCK 24 // If the quartz on the board is 16 MHz. If different, change this value
#else 
#  define BUS_CLOCK 8  // If the quartz on the board is 16 MHz. If different, change this value
#endif



/// Configuration of the serials port SCI0 and SCI1
// Tested : 
//   SERIAL_SCI0_BAUD_RATE 9600      BUS_CLOCK 8   Send OK      Receive not tested
//   SERIAL_SCI0_BAUD_RATE 19200     BUS_CLOCK 8   Send OK      Receive not tested
//   SERIAL_SCI0_BAUD_RATE 38400     BUS_CLOCK 8   Send OK      Receive not tested
//   SERIAL_SCI0_BAUD_RATE 57600     BUS_CLOCK 8   Send Failed  Receive not tested
//   SERIAL_SCI0_BAUD_RATE 115200    BUS_CLOCK 8   Send Failed  Receive not tested

//   SERIAL_SCI0_BAUD_RATE 9600      BUS_CLOCK 24  Send OK      Receive not tested
//   SERIAL_SCI0_BAUD_RATE 19200     BUS_CLOCK 24  Send OK      Receive not tested
//   SERIAL_SCI0_BAUD_RATE 38400     BUS_CLOCK 24  Send OK but init problems     Receive not tested
//   SERIAL_SCI0_BAUD_RATE 57600     BUS_CLOCK 24  Send Failed  Receive not tested
//   SERIAL_SCI0_BAUD_RATE 115200    BUS_CLOCK 24  Send Failed  Receive not tested

#define SERIAL_SCI0_BAUD_RATE 19200
#define SERIAL_SCI1_BAUD_RATE 9600






// Several hardware definitions functions
// --------------------------------------


/// Initialisation of the serial port 0
extern void initSCI_0 (void);

/// Initialisation of the serial port 1
extern void initSCI_1 (void);

/// Convert an integer to a string in hexadecimal format
/// If you do not wants to use a lastCar, put lastCar = '\0' (end of string)
/// ex : value = 0XABCDEF and lastCar = '\n'
/// buf[0] = '0'
/// buf[1] = 'X'
/// buf[2] = 'A'
/// ....
/// buf[7] = 'F'
/// buf[8] = '\n'
/// buf[9] = '\0'
extern char *
hex_convert (char *buf, unsigned long value, char lastCar);

/// Print the string to the serial port sci 
/// (sci takes the values SCI0 or SCI1)
extern void printSCI_str (USART_TypeDef* USARTx, const char * str); 

/// Print the number in hexadecimal  to the serial port sci 
/// (sci takes the values SCI0 or SCI1)
extern void printSCI_nbr (USART_TypeDef* USARTx, unsigned long nbr, char lastCar);


// Integers
#define INTEGER8
#define INTEGER16
#define INTEGER24
#define INTEGER32
#define INTEGER40
#define INTEGER48
#define INTEGER56
#define INTEGER64
 
// Unsigned integers
#define u8   unsigned char
#define u16  unsigned short
#define u32  unsigned long
#define UNS24
#define UNS40
#define UNS48
#define UNS56
#define UNS64 unsigned __int64

// Reals
#define REAL32	float
#define REAL64 double

///The nodes states 
//-----------------
// values are choosen so, that they can be sent directly
// for heartbeat messages...
// Must be coded on 7 bits only
/* Should not be modified */
enum enum_nodeState {
  Initialisation  = 0x00, 
  Disconnected    = 0x01,
  Connecting      = 0x02,
  Preparing       = 0x02,
  Stopped         = 0x04,
  Operational     = 0x05,
  Pre_operational = 0x7F,
  Unknown_state   = 0x0F
};

typedef enum enum_nodeState e_nodeState;

//------------------------sending error message with PDO-------------------------
/*to send error message with PDO, before all the noeud must runing in operational
 state. 
 this PDO is send in event when an error has occured.
 this PDO contain the number of error "4 bytes" and the value of error "4 bytes":
*/

//the state alloude to send PDO is OPERTIONAL STATE
extern e_nodeState         nodeState;

// this function is used to send PDO with the value and number of error
extern u8 sendPDOevent( u8 bus_id, void * variable );



// the number of the error, to send in a PDO
// These variables are defined in the object dictionnary
extern u32 canopenErrNB ;

// the value, to send in a PDO
extern u32 canopenErrVAL;
//-------------------------------------------------------------------------------

// if not null, allow the printing of message to the console
// Could be managed by PDO
extern u8 printMsgErrToConsole;
extern u8 printMsgWarToConsole;
/// Definition of error and warning macros
// --------------------------------------


#ifdef DEBUG_CAN
#  define MSG(string, args) 
#else
#  define MSG(string, args)
#endif

/// Definition of MSG_ERR
// ---------------------



#ifdef DEBUG_ERR_CONSOLE_ON
#  ifdef DEBUG_CAN
#    define MSG_ERR(num, str, val)            \
      {                                       \
        if (printMsgErrToConsole) {           \
        /*  initSCI_0(); */                       \
          printSCI_nbr(USART1, num, ' ');       \
          /* large printing on console  */    \
          printSCI_str(USART1, str);            \
          printSCI_nbr(USART1, val, '\n');      \
        }                                     \
        /* the code here to send by PDO */    \
        if(nodeState == Operational ){ \
         canopenErrNB = num;                 \
         canopenErrVAL= val;                 \
         sendPDOevent ( 0, &canopenErrNB);            \
	}                                     \
      }
#  else
#    define MSG_ERR(num, str, val)            \
      {                                       \
        if (printMsgErrToConsole) {           \
        /*  initSCI_0(); */                       \
          printSCI_nbr(USART1, num, ' ');       \
          /* short printing on console */     \
          printSCI_nbr(USART1, val, '\n');      \
        }                                     \
        /* the code here to send by PDO */    \
        if(nodeState == Operational){ \
         canopenErrNB = num;                 \
         canopenErrVAL = val;                \
         sendPDOevent ( 0, &canopenErrNB);            \
	}                                     \
      }
#  endif
#else 
# ifdef PDO_ERROR
#  define MSG_ERR(num, str, val)              \
     {                                        \
        /* the code here to send by PDO */    \
        if(nodeState == Operational) {\
         canopenErrNB = num;                 \
         canopenErrVAL = val;                \
         sendPDOevent ( 0, &canopenErrNB);            \
	}                                     \
    }

#else
#  define MSG_ERR(num, str, val)             
#endif
#endif


/// Definition of MSG_WAR
// ---------------------
#ifdef DEBUG_WAR_CONSOLE_ON
#  ifdef DEBUG_CAN
#    define MSG_WAR(num, str, val)          \
      if (printMsgWarToConsole) {           \
       /* initSCI_0(); */                       \
        printSCI_nbr(USART1, num, ' ');       \
        /* large printing on console */     \
        printSCI_str(USART1, str);            \
        printSCI_nbr(USART1, val, '\n');      \
      }
#  else
#    define MSG_WAR(num, str, val)          \
      if (printMsgWarToConsole) {           \
       /* initSCI_0(); */                       \
        /* Short printing on console */     \
        printSCI_nbr(USART1, num, ' ');       \
        printSCI_nbr(USART1, val, '\n');      \
      }  
#  endif
#else
#  define MSG_WAR(num, str, val) 
#endif



#endif
