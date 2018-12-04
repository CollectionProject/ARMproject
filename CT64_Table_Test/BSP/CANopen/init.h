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
           File : init.h
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/

#ifndef __INIT_h__
#define __INIT_h__




/// buffer used by SDO
extern s_transfer transfers[MAX_CAN_BUS_ID][SDO_MAX_NODE_ID];	
/// those following arrays store the node_id from which a 
/// SDO frame response is waited for
extern WriteDic writeNetworkDictWaited[MAX_CAN_BUS_ID][SDO_MAX_NODE_ID];
extern ReadDic  readNetworkDictWaited[MAX_CAN_BUS_ID][SDO_MAX_NODE_ID];
extern u8 bDeviceNodeId;

/* The variable to map in a PDO is defined at index and subIndex. Its length is size bytes */
typedef struct mappedVar 
{
  u32 index;
  u8  subIndex;
  u8  size; // in byte
} s_mappedVar; 

typedef struct heartbeatConsumer
{
  u8 nodeProducer;
  u16 time_ms;
} s_heartbeatConsumer;

/************************* prototypes ******************************/

/** Returns the state of the node 
*/
e_nodeState getState (void);

/** Change the state of the node 
*/
void setState (e_nodeState newState);

/** Returns the nodId 
*/
u8 getNodeId (void);

/** Define the node ID. Initialize the object dictionary
*/
u8 setNodeId (u8 nodeId);

void configure_master_SDO (u32 index, u8 serverNode);
u8 configure_client_SDO (u8 slaveNode, u8 clientNode);
void slaveSYNCEnable(u8 slaveNode, u32 cobId, u32 SYNCPeriod, u8 syncType);
void masterMappingPDO (u32 indexPDO, u32 cobId, s_mappedVar *tabMappedVar, u8 nbVar);
void slaveMappingPDO (u8 slaveNode, u32 indexPDO, u32 cobId, s_mappedVar *tabMappedVar, u8 nbVar, u8 transType);
void masterHeartbeatConsumer (s_heartbeatConsumer *tabHeartbeatConsumer, u8 nbHeartbeats);
void masterHeartbeatProducer (u16 time);
void slaveHeartbeatConsumer (u8 slaveNode, s_heartbeatConsumer *tabHeartbeatConsumer, u8 nbHeartbeats);
void slaveHeartbeatProducer (u8 slaveNode, u16 time);
void masterPDOTransmissionMode (u32 indexPDO,  u8 transType);
void slavePDOTransmissionMode (u8 slaveNode, u32 indexPDO,  u8 transType);
void masterSYNCPeriod (u32 SYNCPeriod);
u8 waitingWriteToSlaveDict ( u8 slaveNode, u8 error);
u8 waitingReadToSlaveDict (u8 slaveNode, void * data, u8 * size, u8 error);

/** Initialize the different arrays used by the application
 * return 0
 */
u8 initCANopenMain (void);

/** Some stuff to do when the node enter in reset mode
 *
 */

u8 initResetMode (void);

e_nodeState stateNode (u8 node);


/** Some stuff to do when the node enter in pre-operational mode
 *
 */

u8 initPreOperationalMode (void);


#endif


