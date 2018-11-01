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


#ifndef __pdo_h__
#define __pdo_h__

#include "def.h"
#include "CAN_Cfg.h"

/** Transmit a PDO data frame on the bus bus_id
 * pdo is a structure which contains the pdo to transmit
 * bus_id is hardware dependant
 * return f_can_send(bus_id,&m) or 0xFF if error
 * request can take the value  REQUETE or DONNEES
 */
u8 sendPDO (u8 bus_id, s_PDO pdo, u8 request);

/** Prepare a PDO frame transmission, 
 * whose different parameters are stored in process_var table,
 * to the slave.
 * bus_id is hardware dependant
 * call the function sendPDO
 * return the result of the function sendPDO or 0xFF if error
 */
u8 PDOmGR (u8 bus_id, u32 cobId);

/** Prepare the PDO defined at index to be sent by  PDOmGR
 * Copy all the data to transmit in process_var
 * *pwCobId : returns the value of the cobid. (subindex 1)
 * Return 0 or 0xFF if error.
 */
u8 buildPDO (u16 index, u32 **pwCobId);

/** Transmit a PDO request frame on the bus bus_id
 * to the slave.
 * bus_id is hardware dependant
 */
void sendPDOrequest (u8 bus_id,  u32 cobId);



/** Compute a PDO frame reception
 * bus_id is hardware dependant
 * return 0xFF if error, else return 0
 */
u8 proceedPDO (u8 bus_id, Message *m);



/* used by the application to send a variable by PDO.
 * Check in which PDO the variable is mapped, and send the PDO. 
 * of course, the others variables mapped in the PDO are also sent !
 * ( ie when a specific event occured)
 * bus_id is hardware dependant
 * variable is a pointer to the variable which has to be sent. Must be
 * defined in the object dictionary
 * return 0xFF if error, else return 0
 */
u8 sendPDOevent (u8 bus_id, void * variable);


#endif
