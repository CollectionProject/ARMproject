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
           File : timer.h
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/

#ifndef __timer_h__
#define __timer_h__


#include <applicfg.h>

/** Function called when interruption
*/
void timerInterrupt (u8 unused);



/** This function returns the correct value of the actual time. If the time
 *  would be read without this function, its possible to get the wrong time,
 *  because the interrupt service routine could be executed during reading
 *  this value, and the LSB of the time value is incremented, so an overflow
 *  of the LSB occures, and the program read the wrong value.\n
 *  This function calls itself (recursive) if it seams that the ISR was called
 *  during reading the time.
 *  \param time The pointer to the variable thats points to the time that
 *              should be used in the program
 */
/** I am not sure that these functions are usefull (FD Nov 2004)*/
u16 getTime16 (u16* time);
u32 getTime32 (u32* time);

/** This function sets the correct value of the actual time. If the time
 *  would be written without this function, its possible to set the wrong time,
 *  because the interrupt service routine could be executed during writing
 *  this value.\n
 *  This function calls itself (recursive) if it seams that the ISR was called
 *  during setting the time.
 *  \param time The pointer to the variable thats points to the time that
 *              should be set in the program
 *  \param value The time that should be set.
 */
/** I am not sure that these functions are usefull (FD Nov 2004)*/
void setTime16 (u16* time, u16 value);
void setTime32 (u32* time, u32 value);

#endif // #define __timer_h__
