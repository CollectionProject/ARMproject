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
           File : lifegrd.c
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/

/* Comment when the code is debugged */
//#define DEBUG_CAN
//#define DEBUG_WAR_CONSOLE_ON
//#define DEBUG_ERR_CONSOLE_ON
/* end Comment when the code is debugged */

#include <stdio.h>

#include <lifegrd.h>
#include <objacces.h>
#include <objdictdef.h>
#include <timer.h>
#include "can.h"
#include "ucos_ii.h"
#include "stm32f4xx_can.h"

extern u8 bDeviceNodeId;
extern e_nodeState nodeState;

s_heartbeat_entry    heartBeatTable[NB_OF_HEARTBEAT_PRODUCERS];
s_ourHeartBeatValues ourHeartBeatValues;
e_nodeState NMTable[MAX_CAN_BUS_ID][NMT_MAX_NODE_ID]; 

/* Used only for the heartbeat to send.
* Values are 0 or 1
*/
u8 toggle = 0;

/** This variable indicates wheter the device is in the Notfall-state (this
*  means that one of its heartbeatproducers hasn't send a heartbeat message. 
*/
u8 bErrorOccured;


e_nodeState getNodeState (u8 bus_id, u8 nodeId)
{
   e_nodeState networkNodeState = NMTable[bus_id][nodeId]; // Do not read the toggle bit of the node Guarding
  return networkNodeState;
}


/* Retourne le node-id */
u8 proceedNMTerror(u8 bus_id, Message* m )
{
  //  u16 time, should_time;
  CanTxMsg canmsg;
  u16 should_time;
  u8 index; // Index to scan the table of heartbeat consumers
  u8 *   pbConsumerHeartbeatCount;
  // Pointer to HBConsumerTimeArray (Array of expected heartbeat cycle time for each node.
  // HBConsumerTimeArray is defined in objdict.c *  \param CheckAccess if other than 0, do not read if the data is Write Only
  u32 * pbConsumerHeartbeatEntry;// Index 1016 on 32 bits
  u8 *  pdwSize;
  u8    dwSize, count, ConsummerHeartBeat_nodeId ;
  u8 		nodeId = (u8) GET_NODE_ID((*m));

  pdwSize = &dwSize;
  
  
  if((m->rtr == 1) ) /* Notice that only the master can have sent this node guarding request */
  { 
    // Receiving a NMT NodeGuarding (request of the state by the master)
    //  only answer to the NMT NodeGuarding request, the master is not checked (not implemented)
   
    if (nodeId == bDeviceNodeId )
    {
      Message msg;
      msg.cob_id.w = bDeviceNodeId + 0x700;
      msg.len = (u8)0x01;
      msg.rtr = 0;
      msg.data[0] = nodeState; 
      if (toggle)
      {
        msg.data[0] |= 0x80 ;
        toggle = 0 ;
      }
      else
        toggle = 1 ; 
      // send the nodeguard response.
      // MSG_WAR(0x3130, "Sending NMT Nodeguard to master, state: ", nodeState);
      // f_can_send( 0, &msg );
      CANopen2CAN(&msg, &canmsg);
			CAN_Transmit(CAN2, &canmsg);
      //CAN_send(2, &canmsg, 0xffff);
    }  
    return 1 ;
  }
  
  if (m->rtr == 0) // Not a request CAN
  {
    /*MSG_WAR(0x3110, "Received NMT nodeId : ", nodeId);   */
    /* the slave's state receievd is stored in the NMTable */
    // The state is stored on 7 bit
    NMTable[bus_id][nodeId] = (e_nodeState) ((*m).data[0] & 0x7F) ;
    
    /* Boot-Up frame reception */
    if ( NMTable[bus_id][nodeId] == Initialisation)
      {
        // The device send the boot-up message (Initialisation)
        // to indicate the master that it is entered in pre_operational mode
        // Because the  device enter automaticaly in pre_operational mode,
        // the pre_operational mode is stored 
        // NMTable[bus_id][nodeId] = Pre_operational;
        // MSG_WAR(0x3100, "The NMT is a bootup from node : ", nodeId);
        return 1;
      }
      
    if( NMTable[bus_id][nodeId] != Unknown_state ) {
      if( getODentry( (u16)0x1016, (u8)0x0, (void * *) &pbConsumerHeartbeatCount, pdwSize, 0 ) == OD_SUCCESSFUL )
      {
        if (*pbConsumerHeartbeatCount > (u8)NB_OF_HEARTBEAT_PRODUCERS)
          count = (u8)NB_OF_HEARTBEAT_PRODUCERS ;
        else
          count = *pbConsumerHeartbeatCount ;

        for( index = (u8)0x00; index < count; index++ )
        {
          if( getODentry( (u16)0x1016, (u8)index + 1, (void * *)&pbConsumerHeartbeatEntry, pdwSize, 0 ) == OD_SUCCESSFUL )
          {
            should_time = (u16) ( (*pbConsumerHeartbeatEntry) & (u32)0x0000FFFF ) ;
            ConsummerHeartBeat_nodeId = (u8)( ((*pbConsumerHeartbeatEntry) & (u32)0x00FF0000) >> (u8)16 );
            if ( ( should_time ) && ( nodeId == ConsummerHeartBeat_nodeId ) )
            {
  			      //getTime16( &(heartBeatTable[index].time) );
              //setTime16( &heartBeatTable[index].time, (u8)0x0000 );
              if( bErrorOccured == TRUE )
              {
                lifeguardCallback( (u8)CONNECTION_OK ); // a little strange
                return 0 ;
              }
            }
          }
        } // end for
      }  
    } //End if( NMTable[bus_id][nodeId] != Unknown_state) 
  } // End if (m->rtr == 0)
              
  return 1;
}


void heartbeatMGR(void)
{
  u8 index; // Index to scan the table of heartbeat consumers
  Message msg;
  CanTxMsg canmsg;
  u16 time, should_time;
  u8 *   pbConsumerHeartbeatCount;
  // Pointer to HBConsumerTimeArray (Array of expected heartbeat cycle time for each node.
  // HBConsumerTimeArray is defined in objdict.c
  u32 *  pbConsumerHeartbeatEntry;// Index 1016 on 32 bits
  // Pointer to HBProducerTime (cycle of the heartbeat time producer, defined in objdict.c
  u16 *  pbProducerHeartbeatEntry; // index 1017 on 16 bits
  u8 *   pdwSize;
  u8     dwSize, nodeId, count ;
//  CAN_TypeDef* CAN1;

  pdwSize = &dwSize;
  // Concern heartbeats to receive
  // *****************************
  // now we have to check if one or more heartbeat messages haven't been
  // received within the configured time. if this is the case, we have to call 
  // lifeguardCallback( ) with the argument LOST_HEARTBEAT_MESSAGE
  if( getODentry( (u16)0x1016, (u8)0x0, (void * *)
       &pbConsumerHeartbeatCount, pdwSize, 0 ) == OD_SUCCESSFUL )
  {
    if (*pbConsumerHeartbeatCount > (u8)NB_OF_HEARTBEAT_PRODUCERS)
      count = (u8)NB_OF_HEARTBEAT_PRODUCERS ;
    else
      count = *pbConsumerHeartbeatCount ;
       
    for( index = (u8)0x00; index < count; index++ )
    {
      if( getODentry( (u16)0x1016, (u8)index + 1,
      (void * *)&pbConsumerHeartbeatEntry, pdwSize, 0 ) == OD_SUCCESSFUL )
      {
        should_time = (u16) ( (*pbConsumerHeartbeatEntry) & (u32)0x0000FFFF ) ;
        if ( should_time )
        {
          //time = getTime16( &(heartBeatTable[index].time) );
          if ( should_time < time )
          {
            nodeId = (u8)( ((*pbConsumerHeartbeatEntry) & (u32)0x00FF0000) >> (u8)16 );
            //setTime16( &heartBeatTable[index].time, (u16)0x0000 );
            lifeguardCallback( (u8)HEARTBEAT_CONSUMER_LOST );
            heartbeatError( nodeId );
          }
        }
      }
    }       
  }

  if( getODentry( (u16)0x1017, (u8)0,
                  (void * *)&pbProducerHeartbeatEntry, pdwSize, 0 ) == OD_SUCCESSFUL )
  {
    should_time = *pbProducerHeartbeatEntry ;
    if ( should_time )
    {
      //time = getTime16( &ourHeartBeatValues.ourTime ); // actual time
      if( ( time >= should_time ) )
      {
        // Time expired, the heartbeat must be sent immediately
        // generate the correct node-id: this is done by the offset 1792
        // (decimal) and additionaly
        // the node-id of this device.
//20120317 msg.cob_id.w = bDeviceNodeId + 0x700;
//20120317        msg.len = (u8)0x01;
//20120317        msg.rtr = 0;
//20120317        msg.data[0] = nodeState; // No toggle for heartbeat !
        // send the heartbeat
//        f_can_send( 0, &msg );
        CAN2CANopen(&canmsg, &msg);
        CAN_Transmit(CAN2, &canmsg);
		    //CAN_send(2, &canmsg, 0xffff );
        // reset the timers...
        //setTime16( &(ourHeartBeatValues.ourTime), (u16)0x0000 );
        ourHeartBeatValues.ourLastTime = (u16)0x0000;
//        MSG_WAR(0x3104, "HeartBeat sent at Time : ", time);
      }
      else
      {
        //ourHeartBeatValues.ourLastTime = getTime16( &(ourHeartBeatValues.ourTime) );
				;
      }
    } // end if( ourHeartBeatValues.ourShouldTime != (u16)0x0000 )
  } 
}


void heartbeatInit(void)
{

  bErrorOccured = FALSE;
}


void lifeguardCallback( u8 bReason )
{ 	
  if( bReason == HEARTBEAT_CONSUMER_LOST ){
    bErrorOccured = TRUE;
  }
  else if( bReason == CONNECTION_OK ){
    bErrorOccured = FALSE;
  }
}

void heartbeatError (u8 nodeId)
{
}



