/**
 *      __            ____
 *     / /__ _  __   / __/                      __  
 *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
 *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
 *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
 *                    /_/   github.com/KitSprout    
 * 
 *  @file    kserial.h
 *  @author  KitSprout
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
#include "kstatus.h"
#ifndef KSERIAL_RECONFIG
#include "kserial_conf.h"
#else
#include "kserial_reconf.h"
#endif

/* Define ----------------------------------------------------------------------------------*/

#ifndef KSERIAL_VERSION_DEFINE
#define KSERIAL_VERSION_DEFINE                          "1.1.0"
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

typedef struct
{
    uint32_t type;
    uint32_t nbyte;
    uint8_t param[2];
    uint8_t data[8];

} kserial_ack_t;

typedef enum
{
    KSCMD_R0_NULL               = 0x00,
    KSCMD_R0_DEVICE_ID          = 0xD0,
    KSCMD_R0_DEVICE_BAUDRATE    = 0xD1,
    KSCMD_R0_DEVICE_RATE        = 0xD2,
    KSCMD_R0_DEVICE_MDOE        = 0xD3,
    KSCMD_R0_DEVICE_GET         = 0xE3

} kserial_r0_command_t;

typedef enum
{
    KSCMD_R2_TWI_SCAN_DEVICE    = 0xA1,
    KSCMD_R2_TWI_SCAN_REGISTER  = 0xA2

} kserial_r2_command_t;

typedef void (*pkserial_callback_t)(kserial_packet_t *pk, uint8_t *data, uint32_t count, uint32_t total);

/* Extern ----------------------------------------------------------------------------------*/

extern const uint32_t KS_TYPE_SIZE[KSERIAL_TYPE_LENS];
extern const char KS_TYPE_STRING[KSERIAL_TYPE_LENS][4];
extern const char KS_TYPE_FORMATE[KSERIAL_TYPE_LENS][8];

/* Functions -------------------------------------------------------------------------------*/

uint32_t    kserial_get_typesize(uint32_t type);

uint32_t    kserial_check_header(const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte);
uint32_t    kserial_check_end(const uint8_t *packet, uint32_t nbyte);
uint32_t    kserial_check(const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte);
void        kserial_get_bytesdata(const uint8_t *packet, void *pdata, uint32_t nbyte);

uint32_t    kserial_pack(uint8_t *packet, const void *param, uint32_t type, uint32_t lens, const void *pdata);
uint32_t    kserial_unpack(const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte, void *pdata);
uint32_t    kserial_unpack_buffer(const uint8_t *buffer, uint32_t buffersize, kserial_packet_t *ksp, uint32_t *count);

uint32_t    kserial_send_packet(void *param, void *pdata, uint32_t lens, uint32_t type);
uint32_t    kserial_recv_packet(uint8_t input, void *param, void *pdata, uint32_t *lens, uint32_t *type);

uint32_t    kserial_read(kserial_t *ks );
void        kserial_flush_read(kserial_t *ks );
void        kserial_get_packetdata(kserial_packet_t *ksp, void *pdata, uint32_t index);
uint32_t    kserial_read_continuous(kserial_packet_t *ksp, uint32_t *index, uint32_t *count, uint32_t *total);

uint32_t    kserial_send_command(uint32_t type, uint32_t param1, uint32_t param2, kserial_ack_t *ack);
uint32_t    kserial_check_device(uint32_t *id);

uint32_t    kserial_twi_readregs(uint8_t slaveaddr, uint8_t regaddr, uint8_t *regdata, uint8_t lens);
uint32_t    kserial_twi_writeregs(uint8_t slaveaddr, uint8_t regaddr, uint8_t *regdata, uint8_t lens);
uint32_t    kserial_twi_scandevice( uint8_t *slaveaddr);
uint32_t    kserial_twi_scanregister(uint8_t slaveaddr, uint8_t reg[256]);

#ifdef __cplusplus
}
#endif

#endif

/*************************************** END OF FILE ****************************************/
