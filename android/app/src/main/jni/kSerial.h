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
 *  @date    Jan-2020
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
#include "kStatus.h"

/* Define ----------------------------------------------------------------------------------*/

#ifndef KSERIAL_SEND_ENABLE
#define KSERIAL_SEND_ENABLE             (0U)
#ifndef KS_MAX_SEND_BUFF_SIZE
#define KS_MAX_SEND_BUFF_SIZE           (4096 + 32)
#endif
#endif

#ifndef KSERIAL_RECV_ENABLE
#define KSERIAL_RECV_ENABLE             (0U)
#ifndef KS_MAX_RECV_BUFF_SIZE
#define KS_MAX_RECV_BUFF_SIZE           (4096 + 1024 + 32)
#endif
#endif

#ifndef KSERIAL_TWI_ENABLE
#define KSERIAL_TWI_ENABLE              (0U)
#endif
#if KSERIAL_TWI_ENABLE
#if !(KSERIAL_SEND_ENABLE && KSERIAL_RECV_ENABLE)
#error "Need to enable send and recv"
#endif
#endif

/* Macro -----------------------------------------------------------------------------------*/
/* Typedef ---------------------------------------------------------------------------------*/

typedef struct
{
    uint8_t param[2];
    uint32_t type;
    uint32_t lens;
    uint32_t nbyte;
    void *data;

} kserial_packet_t;

typedef struct
{
    uint32_t size;
    uint32_t count;
    uint8_t *buffer;

    uint32_t pkcnt;
    kserial_packet_t *packet;

} kserial_t;

/* Extern ----------------------------------------------------------------------------------*/
/* Functions -------------------------------------------------------------------------------*/

uint32_t    kSerial_GetTypeSize( uint32_t type );

uint32_t    kSerial_CheckHeader( const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte );
uint32_t    kSerial_CheckEnd( const uint8_t *packet, const uint32_t nbyte );
uint32_t    kSerial_Check( const uint8_t *packet, const uint32_t lens, void *param, uint32_t *type, uint32_t *nbyte );
void        kSerial_GetBytesData( const uint8_t *packet, void *pdata, const uint32_t nbyte );

uint32_t    kSerial_Pack( uint8_t *packet, const void *param, const uint32_t type, const uint32_t lens, const void *pdata );
uint32_t    kSerial_Unpack( const uint8_t *packet, const uint32_t bufsize, void *param, uint32_t *type, uint32_t *nbyte, void *pdata );
uint32_t    kSerial_UnpackBuffer( const uint8_t *buffer, const uint32_t buffersize, kserial_packet_t *packet, uint32_t *packetcnt );
void        kSerial_GetPacketData( kserial_packet_t *ksp, void *pdata, const uint32_t index );

uint32_t    kSerial_SendPacket( void *param, void *sdata, const uint32_t lens, const uint32_t type );
uint32_t    kSerial_RecvPacket( void *param, void *rdata, uint32_t *lens, uint32_t *type );

uint32_t    kSerial_Read( kserial_t *ks );
void        kSerial_ReadFlush( kserial_t *ks );
void        kSerial_GetPacketData( kserial_packet_t *ksp, void *pdata, const uint32_t index );

uint32_t    kSerial_TwiWriteReg( const uint8_t slaveAddr, const uint8_t regAddr, const uint8_t regData );
uint32_t    kSerial_TwiReadReg( const uint8_t slaveAddr, const uint8_t regAddr, uint8_t *regData );
uint32_t    kSerial_TwiReadRegs( const uint8_t slaveAddr, const uint8_t regAddr, uint8_t *regData, const uint8_t lens );
uint32_t    kSerial_TwiCheck( void );
uint32_t    kSerial_TwiScanDevice( uint8_t *slaveAddr );
uint32_t    kSerial_TwiScanRegister( const uint8_t slaveAddr, uint8_t reg[256] );


#ifdef __cplusplus
}
#endif

#endif

/*************************************** END OF FILE ****************************************/
