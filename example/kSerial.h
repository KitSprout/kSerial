/**
 *      __            ____
 *     / /__ _  __   / __/                      __  
 *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
 *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
 *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
 *                    /_/   github.com/KitSprout    
 * 
 *  @file    kSerial.h
 *  @author  KitSprout
 *  @date    01-JAN-2019
 *  @brief   
 * 
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef __KSERIAL_H
#define __KSERIAL_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------*/
#include <stdint.h>

#include "stm32f4xx.h"
#include "algorithms\mathUnit.h"

/* Exported types --------------------------------------------------------------------------*/
/* Exported constants ----------------------------------------------------------------------*/
/* Exported functions ----------------------------------------------------------------------*/  
uint32_t  kSerial_Check( uint8_t *packet, void *param, uint8_t *type, uint16_t *lens );
uint32_t  kSerial_Pack( uint8_t *packet, void *param, const uint8_t type, const uint16_t lens, void *pdata );
uint32_t  kSerial_Unpack( uint8_t *packet, void *param, uint8_t *type, uint16_t *lens, void *pdata );
uint32_t  kSerial_SendPacket( void *param, void *sdata, const uint16_t lens, const uint8_t type );
uint32_t  kSerial_RecvPacket( void *param, void *rdata, uint16_t *lens, uint8_t *type );
uint16_t  kSerial_GetPacketDataLens( uint16_t lens, uint8_t type );

#ifdef __cplusplus
}
#endif

#endif

/*************************************** END OF FILE ****************************************/
