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
           File : sdo.h
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/


#ifndef __sdo_h__
#define __sdo_h__



/******************** CONSTANTES ****************/
/// Abort Domain Transfer
// ----------------------
/* Should not be modified */
#define ADT_server 0x80
#define ADT_client 0x80




/** When a structure needn't yet to be used
 * the structure fields are put to 0
 */
void resetSDOline (u8 bus_id, u8 num);
void resetSDOline1 (u8 bus_id);

/** Search for an unused line in the transfers array
 * to store a new SDO, frame IDD or IDU, which is arriving
 * bus_id is hardware dependant
 * return 0xFF if all the lines are on use. Else, return 0
 */
u8 getSDOfreeLine (u8 bus_id, u8 *line);

/** Search for the line, in the transfers array, which contains the
 * beginning of the reception of a fragmented SDO
 * when a SDO frame, DDS or UDS, is arriving
 * bus_id is hardware dependant
 * nodeId correspond to the message node-id 
 * return 0xFF if error.  Else, return 0
 */
u8 getSDOlineOnUse (u8 bus_id, u8 nodeId, u8 *line);

/** Transmit a SDO frame on the bus bus_id
 * sdo is a structure which contains the sdo to transmit
 * bus_id is hardware dependant
 * return f_can_send(bus_id,&m) or 0xFF if error
 */
u8 sendSDO (u8 bus_id, s_SDO sdo);

/** Transmit a SDO error to the client. The reasons may be :
 * Read/Write to a undefined object
 * Read/Write to a undefined subindex
 * Read/write a not valid length object
 * Write a read only object
 */
u8 sendSDOabort(u8 bus_id, u16 index, u8 subIndex, u32 abortCode);

/** Function called by fonction Read/writeNetworkDict
 * to prepare a SDO frame transmission
 * to the slave whose node_id is ID
 * bus_id is hardware dependant
 * Line gives the position of the SDO in the storing array
 * return 0xFF if error, 0 if no message to send, or
 * return the result of the function sendSDO
 */
u8 SDOmGR (u8 bus_id, u8 line);

/** Treat a SDO frame reception
 * bus_id is hardware dependant
 * call the function sendSDO
 * return 0xFF if error
 *        0x80 if transfert aborted by the server
 *        0x0  ok
 */
u8 proceedSDO (u8 bus_id, Message *m);

/** Used by the application to send a SDO request frame to write the data *data
 * at the index and subindex indicated
 * in the dictionary of the slave whose node_id is nodeId
 * Count : nb of bytes to write in the dictionnary
 * bus_id is hardware dependant
 * return 0xFF if error, else return 0
 */
u8 writeNetworkDict (u8 bus_id, u8 nodeId, u16 index, 
		       u8 subindex, u8 count, void *data); 

/** Used by the application to send a SDO request frame to read
 * in the dictionary of a server node whose node_id is ID
 * at the index and subindex indicated
 * bus_id is hardware dependant
 * return 0xFF if error, else return 0
 */
u8 readNetworkDict (u8 bus_id, u8 nodeId, u16 index, 
   u8 subindex);

/** Use this function after a readNetworkDict to get the result.
  Returns : SDO_SUCCESS = 1    // data is available
            SDO_ABORTED = 0x80 // Aborted transfer (The server have sent an abort SDO)
            SDO_IN_PROGRESS = 0x0 // Receiving the data

  example :
  u32 data;
  u8 size;
  readNetworkDict(0, 0x05, 0x1016, 1) // get the data index 1016 subindex 1 of node 5
  while (getReadResultNetworkDict (0, 0x05, &data, &size) != SDO_IN_PROGRESS);
*/
u8 getReadResultNetworkDict (u8 bus_id, u8 nodeId, void* data, u8 *size);


/**
  Use this function after a writeNetworkDict to get the result of the write.
  Returns : SDO_SUCCESS = 1    // data have been written in the server's directory.
            SDO_ABORTED = 0x80 // Aborted transfer (The server have sent an abort SDO)
            SDO_IN_PROGRESS = 0x0 // Writing the data.
  example :
  u32 data = 0x50;
  u8 size
  writeNetworkDict(0, 0x05, 0x1016, 1, size, &data) // write the data index 1016 subindex 1 of node 5
  while ( getWriteResultNetworkDict (0, 0x05) != SDO_IN_PROGRESS);  
*/
u8 getWriteResultNetworkDict (u8 bus_id, u8 nodeId);

/**
  This function can be used by the client SDO after a getWriteResultNetworkDict or a getReadResultNetworkDict if
one of those function have returned a SDO_ABORTED.
It returns the SDO abort code, the index and subindex where the error occurs.
*/
s_SDOabort getSDOerror (u8 bus_id, u8 nodeId);

/**
  Used when a SDO is received to test, if it is intended to map a variable,
  if this variables can be mappable.
  Do not use this function unless you know what it does.
  Returns OD_SUCCESSFUL or OD_NOT_MAPPABLE
*/
u32 testMappingPDO(u16 index, u8 subindex, void * data);

#endif
