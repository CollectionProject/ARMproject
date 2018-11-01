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
           File : nmtMaster.h
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/

#ifndef __nmtMaster_h__
#define __nmtMaster_h__


/** Initialize the array NMTable (at UNKNOWN_STATE, a state that does not exist)
 * where are stored the slaves state
 * and the array NodeStateWaited which store the node_id of the slaves 
 * from which the master is waiting for the state
 * return 0
 */
u8 initCANopenMaster (void);

/** Transmit a NMT message on the bus number bus_id
 * to the slave whose node_id is ID
 * bus_id is hardware dependant
 * cs represents the order of state changement:
 * cs =  NMT_Start_Node            // Put the node in operational mode             
 * cs =	 NMT_Stop_Node		   // Put the node in stopped mode
 * cs =	 NMT_Enter_PreOperational  // Put the node in pre_operational mode  
 * cs =  NMT_Reset_Node		   // Put the node in initialization mode 
 * cs =  NMT_Reset_Comunication	   // Put the node in initialization mode 
 * The mode is changed according to the slave state machine mode :
 *        initialisation  ---> pre-operational (Automatic transition)
 *        pre-operational <--> operational
 *        pre-operational <--> stopped
 *        pre-operational, operational, stopped -> initialisation
 *
 * return f_can_send(bus_id,&m)               
 */
u8 masterSendNMTstateChange (u8 bus_id, u8 Node_ID, u8 cs);

/** Transmit a Node_Guard message on the bus number bus_id
 * to the slave whose node_id is nodeId
 * bus_id is hardware dependant
 * return f_can_send(bus_id,&m)
 */
u8 masterSendNMTnodeguard (u8 bus_id, u8 nodeId);


/** Prepare a Node_Guard message transmission on the bus number bus_id
 * to the slave whose node_id is nodeId
 * Put nodeId = 0 to send an NMT broadcast.
 * This message will ask for the slave, whose node_id is nodeId, its state
 * bus_id is hardware dependant
 * return 0
 */
u8 masterReadNodeState (u8 bus_id, u8 nodeId);


#endif // __nmtMaster_h__
