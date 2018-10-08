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
           File : sync.h
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/

#ifndef __SYNC_h__
#define __SYNC_h__

#include "CAN_Cfg.h"


/** Time management for sending the SYNC
 */
typedef struct
{
  u32 ourShouldTime;
  /** the time which has passed since the last sending of a sync
   *   message
   */
  u32 ourTime;
  /** the time when the computesync-function (computeSYNC) has been called.
   *  This is neccessary to know, so we can decide when to send our sync.
   */
  /** The current time when the time test function has been called.
   */
  u32 ourLastTime;
} s_sync_values ;

extern s_sync_values SyncValues ;

/** transmit a SYNC message on the bus number bus_id
 * bus_id is hardware dependant
 * return f_can_send(bus_id,&m)
 */
u8 sendSYNC (u8 bus_id, u32 cob_id);

/** Transmit a SYNC message after time * periodeSync, trigered by the timer.
 * periodeSync is defined in the dictionary, index 0x1006.
 * return 0 or 1. The meaning of the return value must be confirmed.
 * This function test if the timer have reached the time. If yes, send a SYNC
 * If no, returns without sending a SYNC. So, this function must be used in a loop.
 * For a better accuracy, put this function in void timerInterrupt( int iValue )
 * Returns 
 * 0 : Time expired -> SYNC is sent
 * 1 : Time non expired or not authorized to send the SYNC (value = 0) in objdic at index 1006)
 * 0xFF : error.
 */
u8 computeSYNC (void);

/** This function is called when the node is receiving a SYNC 
 * message (cob-id = 0x80).
 * What does the function :
 * check if the node is in OERATIONAL mode. (other mode : return 0 but does nothing).
 * Get the SYNC cobId by reading the dictionary index 1005. (Return -1 if it does not correspond 
 * to the cobId received).
 * Scan the dictionary from index 0x1800 to the last PDO defined (dict_cstes.max_count_of_PDO_transmit)
 *   for each PDO whose transmission type is on synchro (transmission type < 241) and if the msg must
 *   be send at this SYNC. read the COBID. Verify that the nodeId inside the 
 *   nodeId correspond to bDeviceNodeId. (Assume that the cobId of a PDO Transmit is made 
 *   with the node id of the node who transmit), get the mapping, launch PDOmGR to send the PDO
 * *m is a pointer to the message received
 * bus_id is hardware dependant
 * return 0 if OK, 0xFF if error
 */
u8 proceedSYNC (u8 bus_id, Message * m);

#endif
