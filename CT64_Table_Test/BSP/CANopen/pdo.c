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
           File : pdo.c
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
#include <pdo.h>
#include "ucos_ii.h"


/**************extern variables declaration*********************************/
extern e_nodeState    nodeState;	    /* slave's state       */
extern u8             bDeviceNodeId;    /* NodeId              */
/********************variables declaration*********************************/
// buffer used by PDO
s_process_var process_var;

/****************************************************************************/
u8 sendPDO(u8 bus_id, s_PDO pdo, u8 req)
{
  u8 i;
  CanTxMsg canmsg;


  if( nodeState == Operational ) 
  {
     /* Message copy for sending */
		canmsg.IDE = CAN_ID_STD;
    canmsg.StdId = pdo.cobId & 0x7FF; // Because the cobId is 11 bytes length
    if ( req == DONNEES ) 
		{
      canmsg.RTR = CAN_RTR_DATA;
      canmsg.DLC = pdo.len;
      //memcpy(&m.data, &pdo.data, m.len);
      // This Memcpy depends on packing structure. Avoid
      for (i = 0 ; i < pdo.len ; i++)
				canmsg.Data[i] = pdo.data[i];
    }
    else 
		{
      canmsg.RTR = CAN_RTR_REMOTE;
      canmsg.DLC = 0;
    }

	CAN_Transmit(CAN2, &canmsg);
	return 0;
  } // end if 
  return 0xFF;
}



/***************************************************************************/
u8 PDOmGR(u8 bus_id, u32 cobId) //PDO Manager
{
  u8 res;
  u8 i;
  s_PDO pdo;

  /* MSG_WAR(0x3903, "PDOmGR",0); */
	
  /* if PDO is waiting for transmission,
     preparation of the message to send */
  if(process_var.state == (TS_DOWNLOAD & ~TS_WAIT_SERVER))
  {
    pdo.cobId = cobId;
    pdo.len =  process_var.count;
    //memcpy(&(pdo.data), &(process_var.data), pdo.len);
    // Ce memcpy devrait être portable
    for ( i = 0 ; i < pdo.len ; i++) 
      pdo.data[i] = process_var.data[i];

    res = sendPDO(bus_id, pdo, DONNEES);
    process_var.state |= TS_WAIT_SERVER;
    return res;
  }
  return 0xFF;
}




/**************************************************************************/
u8 buildPDO(u16 index, u32 **pwCobId)
{
  u16    ind;
  u8     subInd;

  u8*    pMappingCount = NULL;      // count of mapped objects...
  // pointer to the variable which is mapped to a pdo
  void*  pMappedAppObject = NULL; 
  // pointer fo the variable which holds the mapping parameter of an mapping entry  
  u32 *  pMappingParameter = NULL;  

  u8 *   pSize;
  u8     size;
  u8     offset;
  u8     status;

  u32    objDict;

  pSize = &size;
  	
  status = state1;
  subInd = (u8)0x00;
  offset = 0x00;
  ind    = index - 0x1800;

  
  //MSG_WAR(0x3904,"Prepare PDO to send index :", index);

  /* only operational state allows PDO transmission */
  if( nodeState != Operational ) 
  {
  //MSG_WAR(0x2905, "Unable to send the PDO (node not in OPERATIONAL mode). Node : ", index);
    return 0xFF;
  }
  while (1) 
  {
    switch( status ) 
	{
      case state1:	/* get PDO CobId */
      	objDict = getODentry( index, (u8)1,(void * *)pwCobId, pSize, 0 );
      	if( objDict == OD_SUCCESSFUL )
		{
          status = state2;
          break;
        }
      	else 
          ;//MSG_ERR(0x1906,"Error in dictionnary subIndex 1, index : ", index);
      return 0xFF;
      
    case state2:  /* get mapped objects number to transmit with this PDO */

      	objDict = getODentry( (u16)0x1A00 + ind, (u8)0, (void * *)&pMappingCount, pSize, 0 );
		//MSG_WAR(0x3907, "Nb maped objects : ",* pMappingCount);
		//MSG_WAR(0x3908, "        at index : ", 0x1A00 + ind);
      	if( objDict == (u32)OD_SUCCESSFUL )
		{        
          status = state3;
          break;
      	}
      	else
		  ;//MSG_ERR(0x1906,"Error in dictionnary subIndex 1, index : ", index); 
        return 0xFF;
      
    case state3:	/* get mapping parameters */
      	objDict = getODentry( (u16)0x1A00 + ind, subInd + 1,(void * *)&pMappingParameter, pSize, 0 );
        if( objDict == OD_SUCCESSFUL )
		{
          status = state4;
          break;
        }
      	else 
          ;//MSG_ERR(0x1906,"Error in dictionnary subIndex 1, index : ", index);
		return 0xFF;
      
    case state4:	/* get data to transmit */
      	objDict = getODentry( (u16)((*pMappingParameter) >> 16),(u8)(((*pMappingParameter) >> 8 ) & 0x000000FF),(void * *)&pMappedAppObject, pSize, 0 ); 
	    //MSG_WAR(0x390C, "Write in PDO the variable : ",((*pMappingParameter) >> 16));
        //MSG_WAR(0x390D, "subIndex : ",(u8)(((*pMappingParameter) >> 8 ) & 0x000000FF));/* Comment when the code is debugged */
	    //#define DEBUG_CAN
	    //#define DEBUG_WAR_CONSOLE_ON
	    //#define DEBUG_ERR_CONSOLE_ON
	    /* end Comment when the code is debugged */
      	if( objDict == OD_SUCCESSFUL )
		{        
		  //MSG_WAR(0x390E, "value    : ", *(u32*)pMappedAppObject);
          memcpy(  &process_var.data[offset], pMappedAppObject,(*pMappingParameter & (u32)0x000000FF) >> 3 );

//		  #ifdef CANOPEN_BIG_ENDIAN
//		  {
//	  		// data must be transmited with low byte first
//	  		u8 pivot, i;
//	  		u8 sizeData = (*pMappingParameter & (u32)0x000000FF) >> 3 ; // in bytes
//	  		for ( i = 0 ; i < ( sizeData >> 1)  ; i++) 
//			{
//	    	  pivot = process_var.data[offset + (sizeData - 1) - i];
//	    	  process_var.data[offset + (sizeData - 1) - i] = process_var.data[offset + i];
//	    	  process_var.data[offset + i] = pivot ;
//			}
//	      }
//		  #endif
          offset += (u8) ((*pMappingParameter & (u32)0x000000FF) >> 3) ;
          process_var.count = offset;
          subInd++;
          status = state5;
          break;					
       }
       else 
	      ;//MSG_ERR(0x1906,"Error in dictionnary subIndex 1, index : ", index);
       return 0xFF;
      
    case state5:	/* loop to get all the data to transmit */
       if( subInd < *pMappingCount )
	   {
        status = state3;
        break;
       }
       else 
	   {
        status = state6;	
        break;
       }
    case state6:
      return 0;
    }// end switch case
  } // end while
}



/**************************************************************************/
void sendPDOrequest( u8 bus_id, u32 cobId )
{		
  u8           ind = 0;		/*index*/
  u32 *	 pwCobId;	
  u8 *    pSize;
  u8      size;
  u32           objDict;  
  pSize = &size;

  // MSG_WAR(0x3910, "sendPDOrequest ",0);  
  // Sending the request only if the cobid have been found on the PDO receive
  // part dictionary
  for( ind = (u8)0x00; ind < (u8)dict_cstes.max_count_of_PDO_transmit; ind++) { 
    /*get the CobId*/
    // *pwCobId & 0x7f : to have only the  7 bits of the node Id
      objDict = getODentry( (u16)0x1400 + ind, (u8)1, (void * *)&pwCobId, pSize, 0 );
    if( ( objDict == OD_SUCCESSFUL) && ((*pwCobId ) == cobId ) ) {
      s_PDO pdo;
      pdo.cobId = *pwCobId;
      pdo.len = 0;
      sendPDO(bus_id, pdo, REQUETE);	
    }
  }
}



/***********************************************************************/
u8 proceedPDO(u8 bus_id, Message *m)
{			
	
  u8   numPdo, numMap;  // Number of the mapped varable                           
  u8 i;
  u8 *     pMappingCount = NULL;    // count of mapped objects...
  // pointer to the var which is mapped to a pdo...
  void *     pMappedAppObject = NULL;  
  // pointer fo the var which holds the mapping parameter of an mapping entry
  u32 *    pMappingParameter = NULL;  
  u8  *    pTransmissionType = NULL; // pointer to the transmission type
  u32 *    pwCobId = NULL;
  u8  *    pSize;
  u8       size;
  u8            offset;
  u8            status;
  u32            objDict;

  pSize = &size;
  status = state1; //one status of state machines

  /* operational state only allows PDO reception */
  if(nodeState == Operational) { 
//20120317    u8 fc;	// the function code
    status = state1;

    //Hack to map PDOtx 1, 2, 3, & 4 in the same table
    //nodeId = (u8) (GET_NODE_ID((*m))); // The id can be different than the node Id. I must change that.
    //MSG_WAR(0x3931, "proceedPDO, cobID : ", ((*m).cob_id.w & 0x7ff)); 
    
//20120317    fc = (u8) (GET_FUNCTION_CODE((*m)));   //(m.cob_id.w >> 7),get the fct code ,then know it as PDO, SDO, or NMT
    offset = 0x00;
    numPdo = 0;
    numMap = 0;

    if((*m).rtr == DONNEES ) { // The PDO received is not a request.
      //MSG_WAR(0x3930, "max count of PDO received = ", dict_cstes.max_count_of_PDO_receive);

      /* study of all the PDO stored in the dictionary */   
      while( numPdo < dict_cstes.max_count_of_PDO_receive){
				
		switch( status ){
					
			case state1:	/* data are stored in process_var array */
	  			//memcpy(&(process_var.data), &m->data, (*m).len);
	  			// Ce memcpy devrait être portable.
	  			for ( i = 0 ; i < m->len ; i++) 
	    			process_var.data[i] = m->data[i];
	    		process_var.count = (*m).len;
	    		process_var.state = ~TS_WAIT_SERVER;

	    		status = state2; 
	    		break;

			case state2:
	  			/* get CobId of the dictionary correspondant to the received PDO */
	  			objDict = getODentry( (u16)0x1400 + numPdo, (u8)1, (void * *)&pwCobId, pSize, 0); 
	  			if( objDict == OD_SUCCESSFUL ){
	    			status = state3;
				// MSG_WAR(0x3936, "PDO read in the object dictionnary : 0x1400 + ", numPdo);
	    			break;	
	  			}
	  			else {
	    		//	MSG_ERR(0x1949, "PDO cannot be read in the object dictionnary : 0x1400 + ", numPdo);
	    			return 0xFF;
	  			}

			case state3:	/* check the CobId coherance */
	  			//*pwCobId is the cobId read in the dictionary at the state 3
	  			if( *pwCobId == ((*m).cob_id.w & 0x7ff) ){
	    		// The cobId is recognized
	    			status = state4;
				// MSG_WAR(0x3937, "the cobId is recognized :  ",*pwCobId );
	    			break;
	  			}
	  			else {
			    // cobId received does not match with those write in the dictionnary
				// MSG_WAR(0x3950, "the cobId received isn't those in dict : ", *pwCobId);
	    			numPdo++;
	    			status = state2;
	    
	    		/* you need to read again the data  transfered in PDO because 
	       		   process_var is modified in MSG_ERR:sending PDO error 
               
	       		   The lines following are very strange and should be removed. (FD)
	    		*/
	    			for ( i = 0 ; i < m->len ; i++) 
	    				process_var.data[i] = m->data[i];
	    			process_var.count = (*m).len;
	    			process_var.state = ~TS_WAIT_SERVER;
	    			break;
	  			}

			case state4:	/* get mapped objects number */
	  			// The cobId of the message received has been found in the dictionnary.
	  			objDict = getODentry( (u16)0x1600 + numPdo, (u8)0,(void * *)&pMappingCount, pSize, 0); 
	  			// pMappingCount : number of mapped variables. It is a u8
	  			if( objDict  == OD_SUCCESSFUL ){	
	    			status = state5;
				// MSG_WAR(0x3938, "Number of objects mapped : ",*pMappingCount );
	   				 break;
	 			}
	  			else {
	    			;//MSG_ERR(0x1951, "Failure while trying to get map count : 0X1600 + ", numPdo);	
	    			return 0xFF;
	  			}

			case state5:	/* get mapping parameters. Read the subindex (numMap + 1)*/
	  						/* ex : for numPdo = 0 and numMap = 0,
	   						 * pMappingParameter points to Index1600[1].pobject, ie  RxMap1.object[0].
	                         * defined in objdict.c
	   						 * RxMap1.object[i] (32 bits) contains the index, subindex where the mapped 
	                         * variable is defined, and its size. (cf ds301 V.4.02 object 1600h p.109).
	                         * pSize points to  Index1600[1].size, which contains the size of RxMap1.object[i]
	                         */
	    		objDict = getODentry( (u16)0x1600 + numPdo, numMap + 1,(void * *)&pMappingParameter, pSize, 0);
	  			if ( objDict == OD_SUCCESSFUL ) {
				// MSG_WAR(0x3939, "got mapping parameter : ", *pMappingParameter);
	    			status = state6;
	    			break;
	  			}
	  			else {
	    			;//MSG_ERR(0x1952, "Couldn't get mapping parameter for subindex : ", numMap + (u8)1);
	    			return 0xFF;
	  			}				
		   case state6:
	                        /* ex : for numPdo = 0 and numMap = 0,
	                         * *pMappingParameter points to  RxMap1.object[0] (RxMap1.object[1] if numMap = 1)
	                         * where the 16 MSB bits
	                         * contains the index where the value is defined.
	   						 * The medium 8 bits contains the subindex where the value is defined.
	   						 * The lower 8 bits contains the  length of the variable.
	   						 * (See ds301 V.4.02 object 1600h p.109 fig 66)
	   						 */
	  			objDict = getODentry((u16)((*pMappingParameter) >> 16),(u8)(((*pMappingParameter) >> 8 ) & 0xFF),(void * *)&pMappedAppObject, pSize, 0 );
	  			/* pMappedAppObject points to the variable mapped, and pSize to its size. 
	   			 */
	  			if( objDict == OD_SUCCESSFUL ) {
	    			status = state7;
				// MSG_WAR(0x393A, "Receiving data", 0);
	    			break;
	  			}
	  			else 
	    			return 0xFF;
					
		 case state7: /*attribution of the data to the corresponding data*/
	  				  /* the variable of the application is updated with the value
	   				   * received in the PDO. OUF !
	         		   */

//#ifdef CANOPEN_BIG_ENDIAN
//	{
//	  u8 pivot, i;
//	  u8 sizeData = (*pMappingParameter & 0xFF) >> 3 ; // in bytes
//	  for ( i = 0 ; i < ( sizeData >> 1)  ; i++) {
//	    pivot = process_var.data[offset + (sizeData - 1) - i];
//	    process_var.data[offset + (sizeData - 1) - i] = process_var.data[offset + i];
//	    process_var.data[offset + i] = pivot ;
//	  }
//	}
//#endif

	  			memcpy( pMappedAppObject, &process_var.data[offset],(((*pMappingParameter) & 0xFF ) >> 3));
	  
				// MSG_WAR(0x3932, "Variable updated with value received by PDO cobid : ", m->cob_id.w);  
				// MSG_WAR(0x3933, "         Mapped at index : ", (*pMappingParameter) >> 16);
				// MSG_WAR(0x3934, "                subindex : ", ((*pMappingParameter) >> 8 ) & 0xFF);
				// MSG_WAR(0x393B, "                data : ",*((u32 *)pMappedAppObject));
	  			offset += (u8) (((*pMappingParameter) & 0xFF) >> 3) ;
	  			numMap++;	
	  			status = state8;
	  			break;

		case state8:	
	  			/*loop to attribuate all data to the corresponding variables*/
	  		if( numMap < *pMappingCount ) {
	    		status = state5;	
	   			break;
	  		}
	  		else {
	    		offset=0x00;		
	    		numMap = 0;
	    		return 0;
	  		}
	}// end switch status		 
      }// end while	
    }// end if Donnees 


    else if ((*m).rtr == REQUETE ){  
		//MSG_WAR(0x3935, "Receive a PDO request cobId : ", m->cob_id.w);
    	while( numPdo < dict_cstes.max_count_of_PDO_transmit){ 
		/* study of all PDO stored in the objects dictionary */

		switch( status ){

			case state1:	/* check the CobId */
			/* get CobId of the dictionary which match to the received PDO */
	  			objDict = getODentry( (u16)0x1800 + numPdo, (u8)1,(void * *)&pwCobId, pSize, 0);
	  			if ( objDict == OD_SUCCESSFUL && ( *pwCobId == (*m).cob_id.w )) {
	    			status = state4;
	    			break;
	  			}
	  			else
	    			numPdo++;
	  			status = state1;
	  			break;


			case state4:	/* check transmission type (after request?) */
	  			objDict = getODentry( (u16)0x1800 + numPdo, 2,(void * *)&pTransmissionType, pSize, 0);
	  			if ( (objDict == OD_SUCCESSFUL) && ((*pTransmissionType == TRANS_RTR) || (*pTransmissionType == TRANS_RTR_SYNC ) || (*pTransmissionType == TRANS_EVENT)) ) {
	    			status = state5;
	    			break;
	  			}
	  			else
	    			// The requested PDO is not to send on request. So, does nothing.
	    			return 0xFF;

			case state5:	/* get mapped objects number */
	  			objDict = getODentry( (u16)0x1A00 + numPdo, 0,(void * *)&pMappingCount, pSize, 0 );
	  			if( objDict  == OD_SUCCESSFUL ) {
	    			status = state6;
	    			break;
	  			}
	  			else
	    			return 0xFF;

			case state6:	/* get mapping parameters */
	  						/* state 6, 7, 8 compute a message with the variables of the application,
	   						 * to send them in a mapping PDO
	   						 */
	  			objDict = getODentry( (u16)0x1A00 + numPdo, numMap + (u8)1,(void * *)&pMappingParameter, pSize, 0 );
	  			if( objDict == OD_SUCCESSFUL ) {		
	    			status = state7;
	    			break;
	  			}
	  			else
	    			return 0xFF;

			case state7:
	  			objDict = getODentry( (u16)((*pMappingParameter) >> (u8)16), (u8)(( (*pMappingParameter) >> (u8)8 ) & 0xFF),(void * *)&pMappedAppObject, pSize, 0 );
	  			if( objDict == OD_SUCCESSFUL ){
	    		// Comme on copie vers un tableau d'octets, il n'y a pas de contrainte
	    		// d'alignement. Ce memcpy devrait être portable.
	    		memcpy(  &process_var.data[offset], pMappedAppObject, ((*pMappingParameter) & 0xFF) >> 3 ); 
//# ifdef CANOPEN_BIG_ENDIAN
//	{
//	  u8 pivot, i;
//	  u8 sizeData = (*pMappingParameter & (u32)0x000000FF) >> 3 ; // in bytes
//	  for ( i = 0 ; i < ( sizeData >> 1)  ; i++) {
//	    pivot = process_var.data[offset + (sizeData - 1) - i];
//	    process_var.data[offset + (sizeData - 1) - i] = process_var.data[offset + i];
//	    process_var.data[offset + i] = pivot ;
//	  }
//	}
//# endif
	    		offset += (u8) (((*pMappingParameter) & 0xFF) >> 3)  ;
	    		process_var.count = offset;
	    		numMap++;
	    		status = state8;
	    		break;
	  			}
	  			else
	    			return 0xFF;

		case state8:	/* loop to get all mapped objects */
	  		if( numMap < *pMappingCount ){
	    		status = state6;	
	    		break;
	  		}
	  		else {
	    		process_var.state =  (TS_DOWNLOAD & ~TS_WAIT_SERVER);
	    		PDOmGR( bus_id, *pwCobId ); // Transmit the PDO
	    		// I think we should quit the function here by a return 0 (FD)
	    		status = state9;
	   			break;
	  		}

		case state9:	
	  		return 0;
	  

	}// end switch status
      }// end while				
    }// end if Requete
		
    return 0;
   
  }// end if( nodeState == Operational)
  
  return 0;
}

/*********************************************************************/
u8 sendPDOevent( u8 bus_id, void * variable )
{	
  u32      objDict = 0;
  u8       ind, sub_ind;
  u8       status; 
  //u8       offset;
  u8 *     pMappingCount = NULL;
  u32 *    pMappingParameter = NULL;
  void *   pMappedAppObject = NULL;
  u8 *     pTransmissionType = NULL; // pointer to the transmission type
  u32 *    pwCobId = NULL;
  u8 *     pSize;
  u8       size;

  
  ind     = 0x00;
  sub_ind = 1; 
  //offset  = 0x00;
  pSize   = &size;
  status  = state1;

  // look for the index and subindex where the variable is mapped
  // Then, send the pdo which contains the variable.

  //  MSG_WAR (0x193A, "sendPDOevent", 0);
  while(1)
	{
		switch ( status )
		{
      case state1 : 
      	//get the number of entries, i.e. get the number of mapped objects
      	objDict = getODentry( (u16)0x1A00 + ind, 0,(void * *)&pMappingCount, pSize, 0);
      	if( objDict == OD_SUCCESSFUL )
				{
          // Some objects are mapped at the index 0x1A00 + ind
          status = state2;
          break;
      	}
      	else
				{
					//	MSG_WAR(0x393B,"Unable to send variable on event :  not mapped in a PDO to send on event", 0);
          return 0xFF;
        }     	
      case state2 : //get the Mapping Parameters 
      	if ( sub_ind <= *pMappingCount )
				{
          objDict = getODentry( (u16)0x1A00 + ind, sub_ind,(void * *)&pMappingParameter, pSize, 0);
          status = state3;
          // pMappingParamete contains the index (16b), subindex(8b) and the
          // length (8b) of the mapped object
        }
      	else
				{
          // all the mapped objects at this index have been scanned
          ind++;
          sub_ind = 1;
          status = state1;
        }
				break;

      case state3 :
        if( objDict == OD_SUCCESSFUL )
				{
          status = state4;
          break;
        }
        else
				{
          // should never happend
          //MSG_ERR(0x193D, "Error in dict. at index ... : probably nb on subindex (index 0) > subindex defined. Index : ", 0x1A00 + ind);
          return 0xFF;
        }
      
      case state4 :  //get the address of the corresponding variable
      	objDict = getODentry( (u16)((*pMappingParameter) >> 16),(u8)(( (*pMappingParameter) >> (u8)8 ) & (u32)0x000000FF),(void * *)&pMappedAppObject, pSize, 0 );
      	if( objDict == OD_SUCCESSFUL )
				{
          status = state5;
          break;
        }
        else
				{
          ;
					//MSG_ERR(0x193E, "Error in dict. at index : ", (*pMappingParameter) >> (u8)16);
					//MSG_ERR(0x193F, "               subindex : ", ((*pMappingParameter) >> (u8)8 ) & (u32)0x000000FF);
        }
        return 0xFF;

      case state5 :
        if ( pMappedAppObject == variable )
				{
					// MSG_WAR(0x3940, "Variable to send found at index : ", (*pMappingParameter) >> 16);
					// MSG_WAR(0x3941, "                       subIndex : ", ((*pMappingParameter) >> 8 ) & 0x000000FF);
          sub_ind = 0;
          //Mapped PDO at index (0x1A00 + ind) use the PDO parameters defined at (0x1800 + ind).
          objDict = getODentry( (u16)0x1800 + ind, (u8)1, (void * *)&pwCobId, pSize, 0);
          if( objDict == OD_SUCCESSFUL )
					{
            status = state6;
            //MSG_WAR(0x3042, "                          cobid : ", *pwCobId & 0x7FF);
            //MSG_WAR(0x3043, " is definded at index : ", 0x1800 + ind);
          }
          else
					{
            ;//MSG_ERR(0x1944, "Error in dict at subIndex : 1, index : ", 0x1800 + ind);
            return 0xFF;
          }
          break;
				} // end if ( pMappedAppObject == variable )
        else
				{
          sub_ind++;
          status = state2;
        }
        break;     

      case state6 :	/* check transmission type (after request?) */
      	objDict = getODentry( (u16)0x1800 + ind, (u8)2, (void * *)&pTransmissionType, pSize, 0);
      	if( objDict == OD_SUCCESSFUL)
				{
          status = state7;
          break;
      	}
        else
				{
          ;//MSG_ERR(0x1945, "Error in dict at subIndex : 2, index : ", 0x1800 + ind);
          return 0xFF;
        }

      case state7:
        if(*pTransmissionType == TRANS_EVENT)
				{
          status = state8;
          break;
        }
        else
        {
					// The variable have been found in a PDO, but this PDO is not to send on request. But ...
					// Perhaps the variable is mapped in an other PDO, which is to send on request ?
					// So we must continue to scan the PDO Transmit entries.
					ind++;
					status = state1;
					// MSG_WAR(0x2946, "Not allowed to send the variable on event : transmission type does not match 255 at subindex : 2, index : ", 0x1800 + ind);
					// MSG_WAR(0x2947, "      Transmission type is : ", *pTransmissionType);
          return 0xFF;
        }

      case state8:
      	buildPDO(0x1800 + ind, &pwCobId);
      	process_var.state = (TS_DOWNLOAD & ~TS_WAIT_SERVER);
      	PDOmGR( bus_id, *pwCobId ); // Send the PDO
      	return 0;
      
    } // end switch
  } // end while(1)
}

