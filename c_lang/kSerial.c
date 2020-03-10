/**
 *      __            ____
 *     / /__ _  __   / __/                      __  
 *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
 *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
 *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
 *                    /_/   github.com/KitSprout    
 * 
 *  @file    kSerial.c
 *  @author  KitSprout
 *  @date    Mar-2020
 *  @brief   kSerial packet format :
 *           byte 1   : header 'K' (75)       [HK]
 *           byte 2   : header 'S' (83)       [HS]
 *           byte 3   : data bytes (12-bit)   [L ]
 *           byte 4   : data type             [T ]
 *           byte 5   : parameter 1           [P1]
 *           byte 6   : parameter 2           [P2]
 *           byte 7   : checksum              [CK]
 *            ...
 *           byte L-1 : data                  [DN]
 *           byte L   : finish '\r' (13)      [ER]
 */

/* Includes --------------------------------------------------------------------------------*/
#include "kSerial.h"
#if KSERIAL_SEND_ENABLE || KSERIAL_RECV_ENABLE
#include <stdlib.h>
#include <string.h>
#include "serial.h"
#endif

/* Define ----------------------------------------------------------------------------------*/
/* Macro -----------------------------------------------------------------------------------*/

#if KSERIAL_SEND_ENABLE
#ifndef kSerial_Send
#define kSerial_Send(__DATA, __LENS)    Serial_SendData(&s, __DATA, __LENS)
#endif
#endif
#if KSERIAL_RECV_ENABLE
#define kSerial_Recv(__DATA, __LENS)    Serial_RecvData(&s, __DATA, __LENS)
#define kSerial_RecvByte()              Serial_RecvByte(&s)
#define kSerial_RecvFlush()             Serial_Flush(&s)
#endif
#if KSERIAL_SEND_ENABLE || KSERIAL_RECV_ENABLE
#define kSerial_Delay(__MS)             Serial_Delay(__MS)
#endif

/* Typedef ---------------------------------------------------------------------------------*/
/* Variables -------------------------------------------------------------------------------*/

#if KSERIAL_SEND_ENABLE
uint8_t ksSendBuff[KS_MAX_SEND_BUFF_SIZE] = {0};
#endif
#if KSERIAL_RECV_ENABLE
uint8_t ksRecvBuff[KS_MAX_RECV_BUFF_SIZE] = {0};
#endif

/* Prototypes ------------------------------------------------------------------------------*/
/* Functions -------------------------------------------------------------------------------*/

/**
 *  @brief  kSerial_GetTypeSize
 */
uint32_t kSerial_GetTypeSize( uint32_t type )
{
    type &= 0x0F;
    if ((type > KS_F64) || (type == KS_R0))
    {
        return (0);
    }
    else
    {
        return (1 << (type & 0x03));
    }
}

/**
 *  @brief  kSerial_CheckHeader
 */
uint32_t kSerial_CheckHeader( const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte )
{
    uint32_t checksum = 0;

    if ((packet[0] == 'K') && (packet[1] == 'S'))
    {
        *type = packet[3] & 0x0F;
        *nbyte = (((uint32_t)packet[3] << 4) & 0x0F00) | packet[2];
        for (uint32_t i = 2; i < 6; i++)
        {
            checksum += packet[i];
        }
        checksum &= 0xFF;
        if (packet[6] == checksum)
        {
            ((uint8_t*)param)[0] = packet[4];
            ((uint8_t*)param)[1] = packet[5];
            return KS_OK;
        }
    }

    return KS_ERROR;
}

/**
 *  @brief  kSerial_CheckEnd
 */
uint32_t kSerial_CheckEnd( const uint8_t *packet, const uint32_t nbyte )
{
    if (packet[nbyte + 8 - 1] == '\r')
    {
        return KS_OK;
    }

    return KS_ERROR;
}

/**
 *  @brief  kSerial_Check
 */
uint32_t kSerial_Check( const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte )
{
    uint32_t checksum = 0;

    if ((packet[0] == 'K') && (packet[1] == 'S'))
    {
        *type = packet[3] & 0x0F;
        *nbyte = (((uint32_t)packet[3] << 4) & 0x0F00) | packet[2];
        for (uint32_t i = 2; i < 6; i++)
        {
            checksum += packet[i];
        }
        checksum &= 0xFF;
        if ((packet[6] == checksum) && (packet[*nbyte + 8 - 1] == '\r'))
        {
            ((uint8_t*)param)[0] = packet[4];
            ((uint8_t*)param)[1] = packet[5];
            return KS_OK;
        }
    }

    return KS_ERROR;
}

/**
 *  @brief  kSerial_GetBytesData
 */
void kSerial_GetBytesData( const uint8_t *packet, void *pdata, const uint32_t nbyte )
{
    for (uint32_t i = 0; i < nbyte; i++)
    {
        ((uint8_t*)pdata)[i] = packet[7 + i];
    }
}

/**
 *  @brief  kSerial_Pack
 */
uint32_t kSerial_Pack( uint8_t *packet, const void *param, const uint32_t type, const uint32_t lens, const void *pdata )
{
    uint32_t lensHiBit;
    uint32_t packetDataBytes;  // in bytes
    uint32_t checksum = 0;
    uint32_t typeSize = kSerial_GetTypeSize(type);

    packetDataBytes = (typeSize > 1) ? (lens * typeSize) : (lens);
    lensHiBit = (packetDataBytes & 0x0F00) >> 4;

    packet[0] = 'K';                            /* header 'K'  */
    packet[1] = 'S';                            /* header 'S'  */
    packet[2] = packetDataBytes;                /* data bytes  */
    packet[3] = lensHiBit | (type & 0x0F);      /* data type   */

    if (param != NULL)
    {
        packet[4] = ((uint8_t*)param)[0];       /* parameter 1 */
        packet[5] = ((uint8_t*)param)[1];       /* parameter 2 */
    }
    else
    {
        packet[4] = 0;
        packet[5] = 0;
    }
    for (uint32_t i = 2; i < 6; i++)
    {
        checksum += packet[i];
    }
    packet[6] = checksum;                       /* checksum    */

    for (uint32_t i = 0; i < packetDataBytes; i++)
    {
        packet[7 + i] = ((uint8_t*)pdata)[i];   /* data ...... */
    }
    packet[7 + packetDataBytes] = '\r';         /* finish '\r' */

    return (packetDataBytes + 8);
}

/**
 *  @brief  kSerial_Unpack
 */
uint32_t kSerial_Unpack( const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte, void *pdata )
{
    uint32_t status;

    status = kSerial_Check(packet, param, type, nbyte);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < *nbyte; i++)
        {
            ((uint8_t*)pdata)[i] = packet[7 + i];
        }
    }

    return status;
}

/**
 *  @brief  kSerial_UnpackBuffer
 */
uint32_t kSerial_UnpackBuffer( const uint8_t *buffer, const uint32_t index, kserial_packet_t *ksp, uint32_t *count )
{
#if KSERIAL_RECV_ENABLE
    uint32_t status;
    uint32_t offset = 0;
    uint32_t newindex = 0;

    *count = 0;

    while ((index - offset) > 7)   // min packet bytes = 8
    {
        status = kSerial_Check(&buffer[offset], ksp[*count].param, &ksp[*count].type, &ksp[*count].nbyte);
        if (status == KS_OK)
        {
            if ((offset + ksp[*count].nbyte + 7) > index)
            {
                break;
            }
            ksp[*count].data = (void *)malloc(ksp[*count].nbyte * sizeof(uint8_t));
            kSerial_GetBytesData(&buffer[offset], ksp[*count].data, ksp[*count].nbyte);
            offset += ksp[*count].nbyte + 8;
            newindex = offset - 1;
            (*count)++;
        }
        else
        {
            offset++;
        }
    }
    // TODO: fix return
    return (newindex + 1);
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_SendPacket
 */
uint32_t kSerial_SendPacket( void *param, void *sdata, const uint32_t lens, const uint32_t type )
{
#if KSERIAL_SEND_ENABLE
    uint32_t nbytes;
    nbytes = kSerial_Pack(ksSendBuff, param, type, lens, sdata);
    kSerial_Send(ksSendBuff, nbytes);
    // TODO: fix return
    return nbytes;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_RecvPacket
 */
uint32_t kSerial_RecvPacket( void *param, void *rdata, uint32_t *lens, uint32_t *type )
{
#if KSERIAL_RECV_ENABLE
    static uint32_t index = 0;
    static uint32_t bytes = 0;
    static uint32_t point = 0;

    uint32_t state;
    uint32_t typeSize;

    ksRecvBuff[point] = kSerial_RecvByte();
    if (point > 6)
    {
        if ((ksRecvBuff[point - 7] == 'K') && (ksRecvBuff[point - 6] == 'S'))
        {
            index = point - 7;
            bytes = ((((uint32_t)ksRecvBuff[index + 3] << 4) & 0x0F00) | ksRecvBuff[index + 2]) + 8;
        }
        if ((point - index + 1) == bytes)
        {
            state = kSerial_Unpack(&ksRecvBuff[index], param, type, lens, rdata);
            if (state == KS_OK)
            {
                point = 0;
                index = 0;
                bytes = 0;
                typeSize = kSerial_GetTypeSize(*type);
                if (typeSize > 1)
                {
                    *lens /= typeSize;
                }
                return KS_OK;
            }
        }
    }
    if (++point >= KS_MAX_RECV_BUFF_SIZE)
    {
        point = 0;
    }
    return KS_ERROR;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_Read
 */
uint32_t kSerial_Read( kserial_t *ks )
{
#if KSERIAL_RECV_ENABLE
    uint32_t available = 0;
    uint32_t nbyte;
    uint32_t newindex;

    do
    {   // add rx data to packet buffer
        nbyte = kSerial_Recv(&ks->buffer[ks->count], ks->size - ks->count);
        if (nbyte)
        {
            available = 1;
            ks->count += nbyte;
        }
    }
    while (nbyte);

    ks->pkcnt = 0;
    if (available)
    {
        newindex = kSerial_UnpackBuffer(ks->buffer, ks->count, ks->packet, &ks->pkcnt);
        if (ks->pkcnt)
        {
            // update packet buffer
            ks->count -= newindex;
            memcpy(ks->buffer, &ks->buffer[newindex], ks->count);
            memset(&ks->buffer[ks->count], 0, ks->size - ks->count);
        }
    }
    // TODO: fix return
    return ks->pkcnt;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_ReadFlush
 */
void kSerial_ReadFlush( kserial_t *ks )
{
#if KSERIAL_RECV_ENABLE
    kSerial_RecvFlush();
    memset(ks->buffer, 0, ks->size);
    ks->count = 0;
#endif
}

/**
 *  @brief  kSerial_GetPacketData
 */
void kSerial_GetPacketData( kserial_packet_t *ksp, void *pdata, const uint32_t index )
{
#if KSERIAL_RECV_ENABLE
    if (pdata != NULL)
    {
        memcpy(pdata, ksp[index].data, ksp[index].nbyte);
    }
    free(ksp[index].data);
#endif
}

/**
 *  @brief  kSerial_TwiWriteReg
 *  Send packet ['K', 'S', 1, R1, slaveAddress(8-bit), regAddress, ck, regData, '\r']
 */
uint32_t kSerial_TwiWriteReg( const uint8_t slaveAddr, const uint8_t regAddr, const uint8_t regData )
{
#if KSERIAL_TWI_ENABLE
    uint8_t param[2] = {slaveAddr << 1, regAddr};
    uint32_t type = KS_R1;
    uint32_t nbytes;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuff, param, type, 1, &regData);
    kSerial_Send(ksSendBuff, nbytes);
#if 0
    klogd("[W] param = %02X, %02X, type = %d, bytes = %d, data = %02X\n", param[0], param[1], type, nbytes, wdata);
#endif
    return nbytes;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_TwiReadReg
 *  Send packet ['K', 'S', 1, R1, slaveAddress(8-bit)+1, regAddress, ck, 1, '\r']
 *  Recv packet ['K', 'S', 1, R1, slaveAddress(8-bit)+1, regAddress, ck, regData, '\r']
 */
uint32_t kSerial_TwiReadReg( const uint8_t slaveAddr, const uint8_t regAddr, uint8_t *regData )
{
#if KSERIAL_TWI_ENABLE
    uint8_t param[2] = {(slaveAddr << 1) + 1, regAddr};
    uint32_t type = KS_R1;
    uint32_t nbytes;
    uint32_t status;
    uint32_t singleRead = 1;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuff, param, type, 1, &singleRead);
    kSerial_Send(ksSendBuff, nbytes);

    nbytes = 0;
    while (nbytes == 0)
    {
        kSerial_Delay(100);
        nbytes = kSerial_Recv(ksRecvBuff, KS_MAX_RECV_BUFF_SIZE);
    }

    // TODO: check i2cbuff first 'KS'
    status = kSerial_Unpack(ksRecvBuff, param, &type, &nbytes, ksSendBuff);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < nbytes; i++)
        {
            regData[i] = ksSendBuff[i];
        }
#if 0
        klogd("[R] param = %02X, %02X, type = %d, bytes = %d, data =", param[0], param[1], type, nbytes + 8);
        for (uint32_t i = 0; i < nbytes; i++)
        {
            klogd(" %02X", i2cbuff[1][i]);
        }
        klogd("\n");
#endif
    }
    return status;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_TwiReadRegs
 *  Send packet ['K', 'S',    1, R1, slaveAddress(8-bit)+1, regAddress, ck, lens, '\r']
 *  Recv packet ['K', 'S', lens, R1, slaveAddress(8-bit)+1, regAddress, ck, regData ..., '\r']
 */
uint32_t kSerial_TwiReadRegs( const uint8_t slaveAddr, const uint8_t regAddr, uint8_t *regData, const uint8_t lens )
{
#if KSERIAL_TWI_ENABLE
    uint8_t param[2] = {(slaveAddr << 1) + 1, regAddr};
    uint32_t type = KS_R1;
    uint32_t nbytes;
    uint32_t status;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuff, param, type, 1, &lens);
    kSerial_Send(ksSendBuff, nbytes);

    nbytes = 0;
    while (nbytes == 0)
    {
        kSerial_Delay(100);
        nbytes = kSerial_Recv(ksRecvBuff, KS_MAX_RECV_BUFF_SIZE);
    }

    // TODO: check i2cbuff first 'KS'
    status = kSerial_Unpack(ksRecvBuff, param, &type, &nbytes, ksSendBuff);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < nbytes; i++)
        {
            regData[i] = ksSendBuff[i];
        }
#if 0
        klogd("[R] param = %02X, %02X, type = %d, bytes = %d, data =", param[0], param[1], type, nbytes + 8);
        for (uint32_t i = 0; i < nbytes; i++)
        {
            klogd(" %02X", i2cbuff[1][i]);
        }
        klogd("\n");
#endif
    }
    return status;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_TwiCheck
 *  Send packet ['K', 'S', 1, R1, 1, 0, ck,       1, '\r']
 *  Recv packet ['K', 'S', 1, R1, 1, 0, ck, regData, '\r']
 */
uint32_t kSerial_TwiCheck( void )
{
#if KSERIAL_TWI_ENABLE
    uint8_t val;
    if (kSerial_TwiReadReg(0x00, 0x00, &val) != KS_OK)
    {
        return KS_ERROR;
    }
    return KS_OK;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_TwiScanDevice
 *  Send packet ['K', 'S',    0, R2, 0xAB, 0, ck, '\r']
 *  Recv packet ['K', 'S', lens, R2, 0xAB, 0, ck, address ..., '\r']
 */
uint32_t kSerial_TwiScanDevice( uint8_t *slaveAddr )
{
#if KSERIAL_TWI_ENABLE
    uint8_t param[2] = {0xAB, 0};
    uint32_t type = KS_R2;
    uint32_t nbytes;
    uint32_t status;
    uint32_t count;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuff, param, type, 0, NULL);
    kSerial_Send(ksSendBuff, nbytes);

    Serial_Delay(100);
    nbytes = kSerial_Recv(ksRecvBuff, KS_MAX_RECV_BUFF_SIZE);

    // TODO: check i2cbuff first 'KS'
    status = kSerial_Unpack(ksRecvBuff, param, &type, &count, ksSendBuff);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < count; i++)
        {
            slaveAddr[i] = ksSendBuff[i];
        }
#if 0
        printf(" >> i2c device list (found %d device)\n\n", count);
        printf("    ");
        for (uint32_t i = 0; i < count; i++)
        {
            printf(" %02X", slaveAddr[i]);
        }
        printf("\n\n");
#endif
    }
    else
    {
        return 0xFF;
    }
    return count;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_TwiScanRegister
 *  Send packet ['K', 'S',   0, R2, 0xCB, slaveAddress, ck, '\r']
 *  Recv packet ['K', 'S', 256, R2, 0xCB, slaveAddress, ck, address ..., '\r']
 */
uint32_t kSerial_TwiScanRegister( const uint8_t slaveAddr, uint8_t reg[256] )
{
#if KSERIAL_TWI_ENABLE
    uint8_t param[2] = {0xCB, slaveAddr << 1};
    uint32_t type = KS_R2;
    uint32_t nbytes;
    uint32_t status;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuff, param, type, 0, NULL);
    kSerial_Send(ksSendBuff, nbytes);

    Serial_Delay(100);
    nbytes = kSerial_Recv(ksRecvBuff, KS_MAX_RECV_BUFF_SIZE);

    // TODO: check i2cbuff first 'KS'
    status = kSerial_Unpack(ksRecvBuff, param, &type, &nbytes, ksSendBuff);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < 256; i++)
        {
            reg[i] = ksSendBuff[i];
        }
#if 0
        printf("\n");
        printf(" >> i2c device register (address 0x%02X)\n\n", slaveAddr);
        printf("      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
        for (uint32_t i = 0; i < 256; i += 16)
        {
            printf(" %02X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
                i,
                reg[i +  0], reg[i +  1], reg[i +  2], reg[i +  3],
                reg[i +  4], reg[i +  5], reg[i +  6], reg[i +  7],
                reg[i +  8], reg[i +  9], reg[i + 10], reg[i + 11],
                reg[i + 12], reg[i + 13], reg[i + 14], reg[i + 15]
            );
        }
        printf("\n\n");
#endif
    }
    return status;
#else
    return KS_ERROR;
#endif
}

/*************************************** END OF FILE ****************************************/
