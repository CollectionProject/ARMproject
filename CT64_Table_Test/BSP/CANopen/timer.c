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
 *                                                       *
 *  Contact : bossard.ca@voila.fr                        *
 *            francis.dupin@inrets.fr                    *
 *                                                       *
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
           File : timer.c
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/

/* Comment when the code is debugged */
//#define DEBUG_CAN
//#define DEBUG_WAR_CONSOLE_ON
//#define DEBUG_ERR_CONSOLE_ON
/* end Comment when the code is debugged */



#include <applicfg.h>
#include <timer.h>
#include <timerhw.h>
#include <objdictdef.h>
#include <lifegrd.h>
#include <sync.h>



void timerInterrupt( u8 unused)
{
  u8 i;
  for( i=0; i < NB_OF_HEARTBEAT_PRODUCERS; i++ ){
    // this increaes the time var. +1 means +1ms
 /*   if( heartBeatTable[i].should_time > 0 ) */ {
      heartBeatTable[i].time += TIMER_RESOLUTION_MS;
    }
  }
  // increase time of "our" heartbeat...
 // if( ourHeartBeatValues.ourShouldTime > 0 )
    ourHeartBeatValues.ourTime += TIMER_RESOLUTION_MS;

  if (SyncValues.ourShouldTime > 0 )
    SyncValues.ourTime += TIMER_RESOLUTION_US ;
	
}



u16 getTime16( u16* time )
{
  u16 wReturnValue;
  u16 wTestTime;

  wReturnValue = *time;
  wTestTime = *time;
  if( wTestTime != wReturnValue ){
    // maybe an overflow occured. call this function again (recursive)
    wReturnValue = getTime16( time );
  }
  return wReturnValue;
}

u32 getTime32( u32* time )
{
  u32 wReturnValue;
  u32 wTestTime;

  wReturnValue = *time;
  wTestTime = *time;
  if( wTestTime != wReturnValue ){
    // maybe an overflow occured. call this function again (recursive)
    wReturnValue = getTime32 ( time );
  }
  return wReturnValue;
}


void setTime16( u16* time, u16 value )
{
  *time = value;
  if( *time != value ){ 
    // it seems that the irq-serviceroutine was called during setting the
    // value, so try again
    setTime16( time, value );
  }
}

void setTime32( u32* time, u32 value )
{
  *time = value;
  if( *time != value ){ 
    // it seems that the irq-serviceroutine was called during setting the
    // value, so try again
    setTime32( time, value );
  }
}

