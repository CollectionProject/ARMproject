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
           File : timerhw.h
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/


#ifndef  __TIMER_HW__
#define  __TIMER_HW__

/// The timer is responsible to start an interruption every  TIMER_RESOLUTION_MS
/// millisecond.
/// On Linux, the minimum delay is 10 ms
/// On HC12, the timer is configured for 1 ms
#define TIMER_RESOLUTION_MS 1
/// The same in micro-seconds.
#define TIMER_RESOLUTION_US 1000

/* For the configuration of the timer, see also timer.c */ 
 



/** Initiates the timer. Normally the timer should create an overflow every 1ms.
 *  but on linux it is a bit complicated to do this. Thats the reason why
 *  it only generates an overflow every 100ms at the moment
 */
void initTimer (void);


/** Resets the timer. This is mainly needed for microcontrollers where interrupt
 *  flags have to be set back and initial values for the timer registers have
 *  to be set.
 */
void resetTimer (void);




#endif





