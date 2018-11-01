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
 *             Zakaria_belamri@hotmail.com               *
 *  Contact : bossard.ca@voila.fr                        *
 *            francis.dupin@inrets.fr                    *
 *            Zakaria_belamri@hotmail.com                *
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
           File : sync.c
 *-------------------------------------------------------*
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
#include <can.h>
#include <def.h>
#include <objdictdef.h>
#include <objacces.h>
#include <timer.h>
#include <pdo.h>
#include <sync.h>
#include "ucos_ii.h"
/**************extern variables declaration*********************************/
extern u8 bDeviceNodeId;	      // module node_id 
extern e_nodeState nodeState;	      // slave's state in the state machine 
extern s_process_var process_var;

/********************variables declaration*********************************/


s_sync_values SyncValues ; 



/****************************************************************************/

/*****************************************************************************/
u8 proceedSYNC(u8 bus_id, Message *m)
{
  u8 	pdoNum,       // number of the actual processed pdo-nr.
    prp_j;

  u8 *     pMappingCount = NULL;      // count of mapped objects...
  // pointer to the var which is mapped to a pdo
  void *   pMappedAppObject = NULL; 
  // pointer fo the var which holds the mapping parameter of an mapping entry  
  u32 *    pMappingParameter = NULL;  
  // pointer to the transmissiontype...
  u8 *     pTransmissionType = NULL;  
  u32 *    pwCobId = NULL;	

  u8 *   	 pSize;
  u8       size;
  u16 		 index;
  u8 			 subIndex;
  u8 			 offset;
  u8 			 status;
  u8 			 sizeData;
  u32   	 objDict;
  pSize = &size;
	
  status = state3;
  pdoNum=(u8)0x00;
  prp_j=(u8)0x00;
  offset = 0x00;


  /* only operational state allows PDO transmission */
  if( nodeState != Operational ) 
    return 0;
  
  /* So, the node is in operational state */
  /* study all PDO stored in the objects dictionary */	
 
  while( pdoNum < dict_cstes.max_count_of_PDO_transmit ) {  
    switch( status ) {
                    
    case state3:    /* get the PDO transmission type */
      objDict = getODentry( (u16)0x1800 + pdoNum, (u8)2,
			    (void * *)&pTransmissionType, pSize, 0 );
      if( objDict == OD_SUCCESSFUL ){
         status = state4; 
      }
      else {
	     return 0xFF;
      }
      break;
    case state4:	/* check if transmission type is after (this) SYNC */
                        /* The message may not be transmited every SYNC but every n SYNC */	      
      if( (*pTransmissionType >= TRANS_SYNC_MIN) && (*pTransmissionType <= TRANS_SYNC_MAX) &&
          (++count_sync[pdoNum] == *pTransmissionType) ) {	
	     count_sync[pdoNum] = 0;
	     status = state5;
      }
      else {
     	pdoNum++;
    	status = state11;
      }
      break;
    case state5:	/* get PDO CobId */
      objDict = getODentry( (u16)0x1800 + pdoNum, (u8)1, (void * *)&pwCobId, pSize, 0 );
      if( objDict == OD_SUCCESSFUL ){
				status = state7;
      }
      else {
				return 0xFF;	
      }
     break;      
    case state7:  /* get mapped objects number to transmit with this PDO */
      objDict = getODentry( (u16)0x1A00 + pdoNum, (u8)0,
			    (void * *)&pMappingCount, pSize, 0 );
      if( objDict == (u32)OD_SUCCESSFUL ){
				status = state8;
      }
      else {
				return 0xFF;
      }
       break;
    case state8:	/* get mapping parameters */
      objDict = getODentry( (u16)0x1A00 + pdoNum, prp_j + (u8)1,
			    (void * *)&pMappingParameter, pSize, 0 );
      if( objDict == OD_SUCCESSFUL ){
				status = state9;
      }
      else {
				return 0xFF;
      }
      break;
    case state9:	/* get data to transmit */ 
      index = (u16)((*pMappingParameter) >> 16);
      subIndex = (u8)(( (*pMappingParameter) >> (u8)8 ) & (u32)0x000000FF);
      // <<3 because in *pMappingParameter the size is in bits
      sizeData = (u8) ((*pMappingParameter & (u32)0x000000FF) >> 3) ;
      objDict = getODentry(index, subIndex, (void * *)&pMappedAppObject, pSize, 0 ); 
      if( objDict == OD_SUCCESSFUL ){
				if (sizeData != *pSize) {
					//	  MSG_WAR(0x2052, "  Size of data different than (size in mapping / 8) : ", sizeData);
				}
				memcpy(&process_var.data[offset], pMappedAppObject, sizeData);
	
				offset += sizeData ;
				process_var.count = offset;
				prp_j++;
				status = state10;	 
			break;					
      } // end if
      else {
				;//MSG_ERR(0x1013, " Couldn't find mapped variable at index-subindex-size : ", (u16)(*pMappingParameter));
				return 0xFF;
      }
      
    case state10:	/* loop to get all the data to transmit */
      if( prp_j < *pMappingCount ){
//	MSG_WAR(0x3014, "  next variable mapped : ", prp_j);
			status = state8;
			break;
      }
      else {
//	MSG_WAR(0x3015, "  End scan mapped variable", 0);
				process_var.state = TS_DOWNLOAD & (~TS_WAIT_SERVER);
				PDOmGR( bus_id, *pwCobId );	
//	MSG_WAR(0x3016, "  End of this pdo. Should have been sent", 0);
				pdoNum++;
				offset = 0x00;
				prp_j = 0x00;
				status = state11;
			break;
      }
      
    case state11:     
//      MSG_WAR(0x3017, "next pdo index : ", pdoNum);
      status = state3;
      break;
      
    default:
//      MSG_ERR(0x1019,"Unknown state has been reached : %d",status);
      return 0xFF;
    }// end switch case
    
  }// end while( prp_i<dict_cstes.max_count_of_PDO_transmit )
   
  return 0;
}


