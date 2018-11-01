
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
 *             Laurent ROMIEUX                           *
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
           File : init.c
 *-------------------------------------------------------*
 *                                                       *
 *                                                       *      
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
//#include <canOpenDriver.h>
#include <can.h>
#include <def.h>
#include <objdictdef.h>
#include <objacces.h>
#include <init.h>
#include <lifegrd.h>
#include "sdo.h"
#include "ucos_ii.h"
//#include "RTL.h"
//#include "RTX_CAN.h"
//#include "main.h"

/************** variables declaration*********************************/
// buffer used by SDO
extern s_transfer transfers[MAX_CAN_BUS_ID][SDO_MAX_NODE_ID];	
e_nodeState nodeState;

extern proceed_info proceed_infos[];
/*************************************************************************/


e_nodeState getState(void)
{
  return nodeState;
}

void setState(e_nodeState newState)
{
  nodeState = newState;
}

u8 getNodeId(void)
{
  return bDeviceNodeId;
}

u8 setNodeId(u8 nodeId)
{
//  u32 ret;
  u8 i;
  u32 pbData32;
  // bDeviceNodeId is defined in the object dictionary.
  bDeviceNodeId = nodeId;
  // ** Initialize the server(s) SDO parameters
  // Remember that only one SDO server is allowed, defined at index 0x1200
  pbData32 = 0x600 + nodeId;
  setODentry(0x1200, 1, &pbData32, 4, 0); // Subindex 1

  pbData32 = 0x580 + nodeId;
  setODentry(0x1200, 2, &pbData32, 4, 0); // Subindex 2

  // Subindex 3 : node Id client. As we do not know the value, we put the node Id Server
  setODentry(0x1200, 3, &nodeId, 1, 0);

  // ** Initialize the client(s) SDO parameters  
  // Nothing to initialize (no default values required by the DS 401)
  // ** Initialize the receive PDO communication parameters. Only for 0x1400 to 0x1403
  {
    u32 cobID[] = {0x200, 0x300, 0x400, 0x500};
    for (i = 0 ; (i < dict_cstes.max_count_of_PDO_receive)&&(i < 4) ; i++) {
      pbData32 = cobID[i] + nodeId;
 	  setODentry(0x1400 + i, 1, &pbData32, 4, 0); // Subindex 1
    }
  }
  // ** Initialize the transmit PDO communication parameters. Only for 0x1800 to 0x1803
  {
    u32 cobID[] = {0x180, 0x280, 0x380, 0x480};
    for (i = 0 ; (i < dict_cstes.max_count_of_PDO_transmit)&&(i < 4) ; i++) {
      pbData32 = cobID[i] + nodeId;
	  setODentry(0x1800 + i, 1, &pbData32, 4, 0); // Subindex 1
    }
  }
  return 0;
}

u8 initCANopenMain(void)
{
  u8 i, k; 
  u8 j;

  /* transfer structure initialization */
  for (i = 0 ; i < MAX_CAN_BUS_ID ; i++) {
    for (j = 0 ; j < SDO_MAX_NODE_ID ; j++) {
      transfers[i][j].nodeId = 0;
      transfers[i][j].state = 0;
      transfers[i][j].index = 0;
      transfers[i][j].subindex = 0;
      transfers[i][j].count = 0;
      transfers[i][j].offset = 0;
      for (k = 0 ; k < SDO_MAX_DOMAIN_LEN ; k++)
				transfers[i][j].data[k] = 0;
    }
  }// end (i = 0 ; i < MAX_CAN_BUS_ID ; i++)

	
  /* writeNetworkDictWaited array initialization */
  for (i = 0 ; i < MAX_CAN_BUS_ID ; i++) {
    for (j = 0 ; j < SDO_MAX_NODE_ID ; j++)
      writeNetworkDictWaited[i][j].state = SDO_SUCCESS;
  }
	
	
  /* readNetworkDictWaited array initialization */
  for ( i=0; i<MAX_CAN_BUS_ID; i++){
    for ( j=0; j<SDO_MAX_NODE_ID; j++){
      readNetworkDictWaited[i][j].state = SDO_SUCCESS;
      for ( k=0; k<SDO_MAX_DOMAIN_LEN; k++)
				readNetworkDictWaited[i][j].data[k] = 0;
    }
  }
	

  /* NMTable */
  for (i = 0 ; i < MAX_CAN_BUS_ID ; i++)
    for (j = 0 ; j < NMT_MAX_NODE_ID ; j++)
      NMTable[i][j] = Unknown_state;
  
  return 0;
}

u8 initResetMode(void)
{
  // Some stuff to do when the node enter in reset mode
  u32 COB_ID_Sync = 0x00000080; 
  u32 Sync_Cycle_Period = 0; 
  u32 Sync_window_length=0;
  u16 ProducerHeartbitTime = 0;
  setODentry(0x1005, 0, &COB_ID_Sync, 4, 0);	
  setODentry(0x1006, 0, &Sync_Cycle_Period, 4, 0);
  setODentry(0x1007, 0, &Sync_window_length, 4, 0);
  setODentry(0x1017, 0, &ProducerHeartbitTime, 2, 0);	  
  // Init the nodeguard toggle
  toggle = 0;
  return 0;
}

u8 initPreOperationalMode(void)
{
  // Some stuff to do when the node enter in pre-operational mode
  // Init the nodeguard toggle
  toggle = 0;
  return 0;
}


/* Node mode result after NodeGuard query */
e_nodeState stateNode(u8 node) 
{
  e_nodeState state = getNodeState(0, node);
  
  return state;
}

//------------------------------------------------------------------------------
/* The master is writing in its dictionnary to configure the SDO parameters 
to communicate with server_node
*/
void configure_master_SDO(u32 index, u8 serverNode)
{
  u32 data32;
  u8 data8;
  u8 sizeData = 4 ; // in bytes

  /* The master side is client, and the slave side is server */

  /* At subindex 1, the cobId of the Can message from the client
  It is always defined inside the server dictionnary as 0x6000 + server_node.
  So, we have no choice here ! */
  data32 = 0x600 + serverNode;
  setODentry(index, 1, &data32, sizeData, 0);

  /* At subindex 2, the cobId of the Can message from the server to the client.
  It is always defined inside the server dictionnary as 0x580 + client_node.
  So, we have no choice here ! */
  data32 = 0x580 + serverNode;
  setODentry(index, 2, &data32, sizeData, 0);

  /* At subindex 3, the node of the server */
  /* Do not use a data other than UNS8, like UNS32, to write a data type UNS8.*/
  /* It will not work ! */
  /* For this reason, it is safer to use sizeof(data) instead of "1" for the size*/
  data8 = serverNode;
  sizeData = sizeof(data8);
  setODentry(index, 3, &data8, sizeData, 0);
}

//------------------------------------------------------------------------------
/*
 */
u8 waitingWriteToSlaveDict(u8 slaveNode, u8 error)
{
  u8 err;
	u16 cnt=0;
  //CAN_msg canmsg;
  //u8 fc;
  //Message msg_rece;
  /*  MSG_WAR(0x3F21, "Sending SDO to write in dictionnary of node : ", slaveNode); */
  if (error) {
  /*  MSG_ERR(0x1F22, "Unable to send the SDO to node ", slaveNode); */
    return 0xFF;
  }
	
  do
  {
    err = getWriteResultNetworkDict(0, slaveNode);
		OSTimeDly(5);
		if(cnt >= 1000)
		{
			return 0xff;
		}
		else
			cnt++;
  }
  while (err == SDO_IN_PROGRESS);
  /*MSG_WAR(0x3F22, "OK response of Node", slaveNode); */
  /*err = getWriteResultNetworkDict(0, slaveNode);	   */
  if (err == SDO_ABORTED) 
  {
    return 0xff;
  }
    return 0;
}

//------------------------------------------------------------------------------
/*
 */
u8 waitingReadToSlaveDict(u8 slaveNode, void * data, u8 * size, u8 error)
{
  u8 err;
  //MSG_WAR(0x3F2A, "Sending SDO to read in dictionnary of node : ", slaveNode);
  if (error) {
    //MSG_ERR(0x1F2B, "Unable to send the SDO to node ", slaveNode);
    return 0xFF;
  }
  /* Waiting until the slave have responded */
  while (getReadResultNetworkDict (0, slaveNode, data, size) == SDO_IN_PROGRESS);
  //MSG_WAR(0x32C, "OK response of Node", slaveNode);
  err = getReadResultNetworkDict (0, slaveNode, data, size);
  if (err == SDO_ABORTED) {
    //s_SDOabort SDOabort;
    //SDOabort = getSDOerror(0, slaveNode);
		return 0xff;
    //SDOabort.index : The index where the read or write failure occurs
    //SDOabort.subIndex : The subIndex where the read or write failure occurs
    //SDOabort.abortCode : The SDO abort codes. (See DS 301 v.4.02 p.48 table 20
    //MSG_WAR(0x3F2D, "Aborted SDO. Abort code : ", SDOabort.abortCode);
    //exit(-1);
  }
 return 0;
}

//------------------------------------------------------------------------------
/* The master is writing in the slave dictionnary to configure the SDO parameters
Remember that the slave is the server, and the master is the client.
 */
u8 configure_client_SDO(u8 slaveNode, u8 clientNode)
{
  u8 data;
  u8 NbDataToWrite = 1 ; // in bytes
  u8 err = 0;
  /*MSG_WAR(0x3F20, "Configuring SDO by writing in dictionnary Node ", slaveNode);*/
  /* It is only to put at subindex 3 the serverNode. It is optionnal.
     In the slave dictionary, only one SDO server is defined, at index 
     0x1200 */
  data = clientNode;
  err = writeNetworkDict(0, slaveNode, 0x1200, 3, NbDataToWrite, &data); 
  waitingWriteToSlaveDict(slaveNode, err);
 
  return 0;
}		
  
//------------------------------------------------------------------------------
/*
 */

void masterMappingPDO(u32 indexPDO, u32 cobId, s_mappedVar *tabMappedVar, u8 nbVar)
{
  u32 *pbData;
  u32 data32; 
  u8 i;
  u8 size = 0;

  /*if ((indexPDO >= 0x1400) && (indexPDO <= 0x15FF))
    MSG_WAR(0x3F30, "Configuring MASTER for PDO receive, COBID : ", cobId);

  if ((indexPDO >= 0x1800) && (indexPDO <= 0x19FF))
    MSG_WAR(0x3F31, "Configuring MASTER for PDO transmit, COBID : ", cobId);
  */

  /* At indexPDO, subindex 1, defining the cobId of the PDO */
  setODentry(indexPDO, 1, &cobId, 4, 0);

  /* The mapping ... */
  /* ----------------*/
  /* At subindex 0, the number of variables in the PDO */
  setODentry(indexPDO + 0x200, 0, &nbVar, 1, 0);
  getODentry(indexPDO + 0x200, 0, (void * *)&pbData, &size, 0);
  //printf("data writen = 0x%X, taille : %d\n", *pbData, size);

  /* At each subindex 1 .. nbVar, The index,subindex and size of the variable to map in 
     the PDO. The first variable after the COBID is defined at subindex 1, ... 
     The data to write is the concatenation on 32 bits of (msb ... lsb) : 
     index(16b),subIndex(8b),sizeVariable(8b)
*/
  for (i = 0 ; i < nbVar ; i++) 
  {
    data32 = ((tabMappedVar + i)->index << 16) | (((tabMappedVar + i)->subIndex & 0xFF) << 8) |	((tabMappedVar + i)->size & 0xFF);

    // Write dictionary
    setODentry(indexPDO + 0x200, i + 1, &data32, 4, 0);
    
  }
}

//------------------------------------------------------------------------------
/*
 */

void slaveMappingPDO(u8 slaveNode, u32 indexPDO, u32 cobId, s_mappedVar *tabMappedVar, u8 nbVar, u8 transType)
{
  u32 data32; 
  u8 i;
  u8 err;
  u8 nbBytes = 1;
	
	
	OSTimeDly(5);

  /*if ((indexPDO >= 0x1400) && (indexPDO <= 0x15FF))
    MSG_WAR(0x3F32, "Configuring slave for PDO receive, COBID : ", cobId);

  if ((indexPDO >= 0x1800) && (indexPDO <= 0x19FF))
    MSG_WAR(0x3F33, "Configuring slave for PDO transmit, COBID : ", cobId);
  */

  /* At indexPDO, subindex 1, make the RPDO disable */
  data32 = 0x80000000+ cobId;
  err = writeNetworkDict(0, slaveNode, indexPDO, 1, 4, &data32);
	OSTimeDly(5);	
  if(0xff ==waitingWriteToSlaveDict(slaveNode, err))
	{
		return;
	}
	
// 	/* At indexPDO, subindex 1, defining the cobId of the PDO */
//   err = writeNetworkDict(0, slaveNode, indexPDO, 1, 4, &cobId); 
//   waitingWriteToSlaveDict(slaveNode, err);

  /* Following, set the communication mode, the node send the PDO spontaneously when TPDO*/
  /*  */
  err = writeNetworkDict(0, slaveNode, indexPDO, 2, 1, &transType);
	OSTimeDly(5);	
  waitingWriteToSlaveDict(slaveNode, err);

	//if(cobId == 0x381 ||cobId == 0x281)
	{
	/* The mapping ... */
  /* ----------------*/
  /* At subindex 0, clear PDO mapping */
  data32 = 0x00;
  err = writeNetworkDict(0, slaveNode, indexPDO + 0x200, 0, nbBytes, &data32);
	OSTimeDly(5);		
  waitingWriteToSlaveDict(slaveNode, err);

  /* At each subindex 1 .. nbVar, The index,subindex and size of the variable to map in 
     the PDO. The first variable after the COBID is defined at subindex 1, ... 
     The data to write is the concatenation on 32 bits of (msb ... lsb) : 
     index(16b),subIndex(8b),sizeVariable(8b)
*/
  for (i = 0 ; i < nbVar ; i++) 
  {
    data32 = ((tabMappedVar + i)->index << 16) | (((tabMappedVar + i)->subIndex & 0xFF) << 8) |	((tabMappedVar + i)->size & 0xFF);

    // Write dictionary
    err = writeNetworkDict(0, slaveNode, indexPDO + 0x200, i + 1, 4, &data32); 
		OSTimeDly(5);
    waitingWriteToSlaveDict(slaveNode, err);    
  }

	/* At subindex 0, the number of variables in the PDO */
  err = writeNetworkDict(0, slaveNode, indexPDO + 0x200, 0, nbBytes, &nbVar); 
	OSTimeDly(5);
  waitingWriteToSlaveDict(slaveNode, err);
	}

  /* At indexPDO, subindex 1, make the RPDO enable */
  data32 = cobId;
  err = writeNetworkDict(0, slaveNode, indexPDO, 1, 4, &data32);
	OSTimeDly(5);
  waitingWriteToSlaveDict(slaveNode, err);

}

//------------------------------------------------------------------------------
/*
 */
void masterHeartbeatConsumer(s_heartbeatConsumer 
			     *tabHeartbeatConsumer, u8 nbHeartbeats)
{
  u32 data;
  u8 i;
  u8 nbHB = nbHeartbeats;

  //MSG_WAR(0x3F40, "Configuring heartbeats consumers for master", 0);
  /* At index 1016, subindex 0 : the nb of consumers (ie nb of nodes of which are expecting heartbeats) */
  setODentry(0x1016, 0, & nbHB, 1, 0);
  
  /* At Index 1016, subindex 1, ... : 32 bit values : msb ... lsb :
     00 - node_consumer (8b) - time_ms (16b)
     Put 0 to ignore the entry.
  */
  for (i = 0 ; i < nbHeartbeats ; i++) {
    data = (((tabHeartbeatConsumer + i)->nodeProducer & 0xFF)<< 16) | ((tabHeartbeatConsumer + i)->time_ms & 0xFFFF);
    setODentry(0x1016, i + 1, & data, 4, 0);
  }
}

//------------------------------------------------------------------------------
/*
 */

void masterHeartbeatProducer(u16 time)
{
  u16 hbTime = time;
  //MSG_WAR(0x3F45, "Configuring heartbeat producer for master", 0);
  /* At index 1017, subindex 0, defining the time to send the heartbeat. Put 0 to never send heartbeat */
  setODentry(0x1017, 0, &hbTime, 2, 0);
}

//------------------------------------------------------------------------------
/*
 */
void slaveHeartbeatConsumer(u8 slaveNode, s_heartbeatConsumer 
			    *tabHeartbeatConsumer, u8 nbHeartbeats)
{
  u32 data;
  u8 err;
  u8 i;
  
  //MSG_WAR(0x3F46, "Configuring heartbeats consumers for node  : ", slaveNode);
  
  /* At Index 1016, subindex 1, ... : 32 bit values : msb ... lsb :
     00 - node_consumer (8b) - time_ms (16b)
     Put 0 to ignore the entry.
  */
  for (i = 0 ; i < nbHeartbeats ; i++) {
    data = (((tabHeartbeatConsumer + i)->nodeProducer & 0xFF)<< 16) | 
      ((tabHeartbeatConsumer + i)->time_ms & 0xFFFF);
    err = writeNetworkDict(0, slaveNode, 0x1016, i + 1, 4, &data); 
    waitingWriteToSlaveDict(slaveNode, err);
  }
}

//------------------------------------------------------------------------------
/*
 */

void slaveHeartbeatProducer(u8 slaveNode, u16 time)
{
  u8 err;
  //MSG_WAR(0x3F47, "Configuring heartbeat producer for node  : ", slaveNode);
  /* At index 1017, subindex 0, defining the time to send the heartbeat. Put 0 to never send heartbeat */

  err = writeNetworkDict(0, slaveNode, 0x1017, 0, 2, &time); 
  waitingWriteToSlaveDict(slaveNode, err);
}

//------------------------------------------------------------------------------
/*
 */

void masterPDOTransmissionMode(u32 indexPDO,  u8 transType)
{
  //MSG_WAR(0x3F48, "Configuring transmission from master, indexPDO : ", indexPDO);
 
  /* At subindex 2, the transmission type */
  setODentry(indexPDO, 2, &transType, 1, 0);
}


//------------------------------------------------------------------------------
/*
 */

void slavePDOTransmissionMode(u8 slaveNode, u32 indexPDO,  u8 transType)
{
  u8 err;
  /*MSG_WAR(0x3F41, "Configuring transmission mode for node : ", slaveNode); */
  /*MSG_WAR(0x3F42, "                              indexPDO : ", indexPDO);	 */

  err = writeNetworkDict(0, slaveNode, indexPDO, 2, 1, &transType); 
  waitingWriteToSlaveDict(slaveNode, err);

}

//------------------------------------------------------------------------------
/*
 */

void masterSYNCPeriod(u32 SYNCPeriod)
{
 //MSG_WAR(0x3F49, "Configuring master to send SYNC every ... micro-seconds :", SYNCPeriod);

 /* At index 0x1006, subindex 0 : the period in ms */
 setODentry(0x1006, 0, &SYNCPeriod , 4, 0);
}

void slaveSYNCEnable(u8 slaveNode, u32 cobId, u32 SYNCPeriod, u8 syncType)
{
  u8  err;
  u32 status;

  err = writeNetworkDict(0, slaveNode, 0x1006 , 0, 4, &SYNCPeriod); 
  waitingWriteToSlaveDict(slaveNode, err);
	OSTimeDly(10);

  if (syncType == 0)
    status = cobId;
  else
    status = 0x40000000 + cobId;

  err = writeNetworkDict(0, slaveNode, 0x1005 , 0, 4, &status); 
  waitingWriteToSlaveDict(slaveNode, err);
}




