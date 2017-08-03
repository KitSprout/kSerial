/**
  *      __            ____
  *     / /__ _  __   / __/                      __  
  *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
  *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
  *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
  *                    /_/   github.com/KitSprout    
  * 
  * @file    kSerial.h
  * @author  KitSprout
  * @date    02-Jul-2017
  * @brief   
  * 
  */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef __KSERIAL_H
#define __KSERIAL_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------*/
#include "stm32f4xx.h"
#include "algorithms\mathUnit.h"

/* Exported types --------------------------------------------------------------------------*/
/* Exported constants ----------------------------------------------------------------------*/
/* Exported functions ----------------------------------------------------------------------*/  
void    kSerial_Config( USART_TypeDef *USARTx );
int8_t  kSerial_Pack( uint8_t *packet, void *param, void *data, const uint8_t lens, const uint8_t type );
int8_t  kSerial_Unpack( uint8_t *packet, void *param, void *data, uint8_t *lens, uint8_t *type );
int8_t  kSerial_SendPacket( void *param, void *data, const uint8_t lens, const uint8_t type );
int8_t  kSerial_RecvPacket( void *param, void *data, uint8_t *lens, uint8_t *type );
uint8_t kSerial_GetPacketDataLens( uint8_t lens, uint8_t type );

#ifdef __cplusplus
}
#endif

#endif

/*************************************** END OF FILE ****************************************/
