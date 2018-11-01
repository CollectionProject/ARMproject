
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
           File : canOpenMaster.c
 *-------------------------------------------------------*
 * Functions used by a master node only                  *      
 *                                                       *
 *********************************************************/


/* Comment when the code is debugged */
//#define DEBUG_CAN
//#define DEBUG_WAR_CONSOLE_ON
//#define DEBUG_ERR_CONSOLE_ON
/* end Comment when the code is debugged */


#include <stddef.h> /* for NULL */
#include <string.h>
#include <stdio.h>
#include <applicfg.h>
#include <can.h>
#include <def.h>
#include <objdictdef.h>
#include <nmtMaster.h>
#include "ucos_ii.h"





/********************extern variables declaration****************************/
extern u8 bDeviceNodeId;
extern proceed_info proceed_infos[];  // Array of message processing information



/************************variables declaration********************************/
// Network Management Table, stores slaves'state
extern e_nodeState NMTable[MAX_CAN_BUS_ID][NMT_MAX_NODE_ID]; 
/* creation of an array which stores the node_id of the nodes
from which the master is waiting for a response.
Takes the values TRUE or FALSE
*/ 
u8 NodeStateWaited[MAX_CAN_BUS_ID][NMT_MAX_NODE_ID];




/****************************************************************************/
u8 initCANopenMaster(void)
{
  u8 i, j;
	
  /* NodeStateWaited array initialization */
  for (i = 0 ; i < MAX_CAN_BUS_ID ; i++) {
    for (j = 0 ; j < NMT_MAX_NODE_ID ; j++)
      NodeStateWaited[i][j] = FALSE;
  } // end (i = 0 ; i < MAX_CAN_BUS_ID ; i++)
	
  return 0;
}



/******************************************************************************/
u8 masterSendNMTstateChange(u8 bus_id, u8 Node_ID, u8 cs)
{
  CanTxMsg msg;

//  MSG_WAR(0x3501, "Send_NMT cs : ", cs);
//  MSG_WAR(0x3502, "    to node : ", Node_ID);
  /* message configuration */
  msg. StdId   = 0x0000; /*(NMT) << 7*/
  msg. RTR  = CAN_RTR_DATA;
  msg. DLC  = 2;
  msg. Data[0] = cs;
  msg. Data[1] = Node_ID;
	msg. IDE     = CAN_ID_STD;
  
//  return f_can_send(bus_id,&m);
  CAN_Transmit(CAN2, &msg);
  return 0;
}


/****************************************************************************/
u8 masterSendNMTnodeguard(u8 bus_id, u8 nodeId)
{
  CanTxMsg msg_send;

  msg_send.StdId =  nodeId | (NODE_GUARD << 7);
  msg_send.RTR   =	CAN_RTR_REMOTE;
  msg_send.DLC   =  0x01;
	msg_send.IDE   =  CAN_ID_STD;
	
	CAN_Transmit(CAN2, &msg_send);

  return 0;
}

/******************************************************************************/
u8 masterReadNodeState(u8 bus_id, u8 nodeId)
{
  // FIXME: should warn for bad toggle bit.

  /* NMTable configuration to indicate that the master is waiting
   * for a Node_Guard frame from the slave whose node_id is ID */
  NMTable[bus_id][nodeId] = Unknown_state; // A state that does not exist
  NodeStateWaited[bus_id][nodeId] = TRUE;

  if (nodeId == 0) { // NMT broadcast
    u8 i = 0;
    for (i = 0 ; i < NMT_MAX_NODE_ID ; i++) {
      NMTable[bus_id][i] = Unknown_state;
      NodeStateWaited[bus_id][i] = TRUE;
    }
  }
  masterSendNMTnodeguard(bus_id,nodeId);
	
  return 0;
}









