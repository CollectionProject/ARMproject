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
 *             Zakaria BELAMRI                           *
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
           File : objacces.c
 *-------------------------------------------------------*
 *                                                       *      
 *                                                       *
 *********************************************************/

/* Comment when the code is debugged */
#define DEBUG_CAN
#define DEBUG_WAR_CONSOLE_ON
#define DEBUG_ERR_CONSOLE_ON
/* end Comment when the code is debugged */



#include <string.h>
#include <stdio.h>
#include <objdictdef.h>
#include <objacces.h>



extern const dict_cste dict_cstes;	/*see in objdictdef.h*/



u8 accessDictionaryError(u16 index, u8 subIndex, u8 sizeDataDict, u8 sizeDataGiven, u32 code)
{
//  MSG_WAR(0x2B09,"Dictionary index : ", index);
//  MSG_WAR(0X2B10,"           subindex : ", subIndex);
  switch (code) {
    case  OD_NO_SUCH_OBJECT: 
//      MSG_WAR(0x2B11,"Index not found ", index);
      break;
    case OD_NO_SUCH_SUBINDEX :
//      MSG_WAR(0x2B12,"SubIndex not found ", subIndex);
      break;   
    case OD_WRITE_NOT_ALLOWED :
//      MSG_WAR(0x2B13,"Write not allowed, data is read only ", index);
      break;         
    case OD_LENGTH_DATA_INVALID :    
//      MSG_WAR(0x2B14,"Conflict size data. Should be (bytes)  : ", sizeDataDict);
//      MSG_WAR(0x2B15,"But you have given the size  : ", sizeDataGiven);
      break;
    case OD_NOT_MAPPABLE :
//      MSG_WAR(0x2B16,"Not mappable data in a PDO at index    : ", index);
      break;
  default :
//    MSG_WAR(0x2B16, "Unknown error code : ", code);
      break;
  }
  return 0; 
}	


u32 getODentry( u16 wIndex,
		  u8 bSubindex,
		  void * * ppbData,
		  u8 * pdwSize,
		  u8 checkAccess)
{
  u32 errorCode;
  const indextable *ptrTable;
  ptrTable = scanOD(wIndex, bSubindex, 0, &errorCode);
  if (errorCode != OD_SUCCESSFUL)
    return errorCode;

  if( ptrTable->bSubCount <= bSubindex ) {
    // Subindex not found
    //accessDictionaryError(wIndex, bSubindex, 0, 0, OD_NO_SUCH_SUBINDEX);
    return OD_NO_SUCH_SUBINDEX;
  }
  
  if (checkAccess && (ptrTable->pSubindex[bSubindex].bAccessType == WO)) {
    //accessDictionaryError(wIndex, bSubindex, 0, 0, OD_WRITE_NOT_ALLOWED);
    return OD_READ_NOT_ALLOWED;
  }

  *ppbData = ptrTable->pSubindex[bSubindex].pObject;
  *pdwSize = ptrTable->pSubindex[bSubindex].size;
 

  return OD_SUCCESSFUL;
}

u32 setODentry( u16 wIndex, u8 bSubindex, void * pbData, u8 dwSize, u8 checkAccess)
{
  u8 szData;
  u32 errorCode;
  const indextable *ptrTable;
  ptrTable = scanOD(wIndex, bSubindex, dwSize, &errorCode);
  if (errorCode != OD_SUCCESSFUL)
    return errorCode;

  if( ptrTable->bSubCount <= bSubindex ) 
  {
    // Subindex not found
    //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_SUBINDEX);
    return OD_NO_SUCH_SUBINDEX;
  }
  szData = ptrTable->pSubindex[bSubindex].size;
  if (szData != dwSize) 
  {
    //accessDictionaryError(wIndex, bSubindex, szData, dwSize, OD_LENGTH_DATA_INVALID);
    return OD_LENGTH_DATA_INVALID;
  }
  if (checkAccess && (ptrTable->pSubindex[bSubindex].bAccessType == RO)) 
  {
    //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_WRITE_NOT_ALLOWED);
    return OD_WRITE_NOT_ALLOWED;
  }
  memcpy( ptrTable->pSubindex[bSubindex].pObject, pbData, dwSize );

  return OD_SUCCESSFUL;
}




const indextable * scanOD (u16 wIndex, u8 bSubindex, u8 dwSize, u32 * errorCode)
{
  u16 offset = 0;
  const indextable *ptrTable;
  
  *errorCode = OD_SUCCESSFUL; // Default value.
  
  if( ( wIndex >= (u16)0x1000 ) && ( wIndex <= (u16)0x11FF ) ) {
    /***********************************************/
    /* we are in the communication profile area... */
    /***********************************************/
    offset = wIndex - (u16)0x1000;
    if( (wIndex > dict_cstes.comm_profile_last ) || 
	(CommunicationProfileArea[offset].pSubindex == NULL)){
      // This index is not defined. 
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = CommunicationProfileArea + offset;
    return ptrTable;
  }

  else if( ( wIndex >= (u16)0x1400 ) && ( wIndex <= (u16)0x15FF ) ) { 
    /***************************************/
    /* Receive PDO Communication Parameter */
    /***************************************/
    offset = wIndex - (u16)0x1400;
    if( (wIndex > dict_cstes.receive_PDO_last) ||
	(receivePDOParameter[offset].pSubindex == NULL)) {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = receivePDOParameter + offset;
    return ptrTable;
  }

  else if( ( wIndex >= (u16)0x1800 ) && ( wIndex <= (u16)0x19FF ) ) { 
    /****************************************/
    /* Transmit PDO Communication Parameter */
    /****************************************/
    offset = wIndex - (u16)0x1800;
    if( (wIndex > dict_cstes.transmit_PDO_last) ||
	(transmitPDOParameter[offset].pSubindex == NULL)) {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = transmitPDOParameter + offset;
    return ptrTable;
  }

  else if( ( wIndex >= (u16)0x1600 ) && ( wIndex <= (u16)0x17FF ) ) { 
    /****************************************/
    /* Receive mapping Parameters           */
    /****************************************/
    offset = wIndex - (u16)0x1600;
    if( (wIndex > dict_cstes.receive_PDO_mapping_last) ||
	(RxPDOMappingTable[offset].pSubindex == NULL)) {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = RxPDOMappingTable + offset;
    return ptrTable;
  }

  else if( ( wIndex >= (u16)0x1A00 ) && ( wIndex <= (u16)0x1BFF ) ) { 
    /****************************************/
    /* Transmit mapping Parameters          */
    /****************************************/
    offset = wIndex - (u16)0x1A00;
    if( (wIndex > dict_cstes.transmit_PDO_mapping_last) ||
	(TxPDOMappingTable[offset].pSubindex == NULL)) {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = TxPDOMappingTable + offset;
    return ptrTable;
  }

  else if( ( wIndex >= (u16)0x1200 ) && ( wIndex <= (u16)0x127F ) ) { 
    /****************************************/
    /* Server SDO Parameter                 */
    /****************************************/
    offset = wIndex - (u16)0x1200;
    if( (wIndex > dict_cstes.server_SDO_last) ||
	(serverSDOParameter[offset].pSubindex == NULL)) {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = serverSDOParameter + offset;
    return ptrTable;
  }

  else if( ( wIndex >= (u16)0x1280 ) && ( wIndex <= (u16)0x12FF ) ) { 
    /****************************************/
    /* Client SDO Parameter                 */
    /****************************************/
    offset = wIndex - (u16)0x1280;
    if( (wIndex > dict_cstes.client_SDO_last) ||
	(clientSDOParameter[offset].pSubindex == NULL)) {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = clientSDOParameter + offset;
    return ptrTable;
  }

  else if( ( wIndex >= (u16)0x2000 ) && ( wIndex <= (u16)0x5FFF ) ) { 
    /****************************************/
    /* Manufacturer specific profile        */
    /****************************************/
    offset = wIndex - (u16)0x2000;
    if( (wIndex > dict_cstes.manufacturerSpecificLastIndex) ||
	(manufacturerProfileTable[offset].pSubindex == NULL)) {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = manufacturerProfileTable + offset;
    return ptrTable;
  }

  else if( ( wIndex >= (u16)0x6000 ) && ( wIndex <= (u16)0x61FF ) ) { 
    /****************************************/
    /* Digital Input specific profile       */
    /****************************************/
    offset = wIndex - (u16)0x6000;
    if( (wIndex > dict_cstes.digitalInputTableLastIndex) ||
	(digitalInputTable[offset].pSubindex == NULL)) {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = digitalInputTable + offset;
    return ptrTable;
  }

  else if( ( wIndex >= (u16)0x6200 ) && ( wIndex <= (u16)0x9FFF ) ) { 
    /****************************************/
    /* Digital Output specific profile      */
    /****************************************/
    offset = wIndex - (u16)0x6200;
    if( (wIndex > dict_cstes.digitalOutputTableLastIndex) ||
	(digitalOutputTable[offset].pSubindex == NULL)) {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    }
    ptrTable = digitalOutputTable + offset;
    return ptrTable;
  }

  else
    {
      //accessDictionaryError(wIndex, bSubindex, 0, dwSize, OD_NO_SUCH_OBJECT);
      *errorCode = OD_NO_SUCH_OBJECT;
      return NULL;
    } 
  //return OD_SUCCESSFUL;
}

