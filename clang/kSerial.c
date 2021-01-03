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
 *  @brief   kSerial packet format :
 *           byte 1   : header 'K' (75)       [HK]
 *           byte 2   : header 'S' (83)       [HS]
 *           byte 3   : data type (4-bit)     [TP]
 *           byte 4   : data bytes (12-bit)   [LN]
 *           byte 5   : parameter 1           [P1]
 *           byte 6   : parameter 2           [P2]
 *           byte 7   : checksum              [CK]
 *            ...
 *           byte L-1 : data                  [DN]
 *           byte L   : finish '\r' (13)      [ER]
 */

/* Includes --------------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include "kSerial.h"

/* Define ----------------------------------------------------------------------------------*/
/* Macro -----------------------------------------------------------------------------------*/
/* Typedef ---------------------------------------------------------------------------------*/
/* Variables -------------------------------------------------------------------------------*/

#if KSERIAL_SEND_ENABLE
uint8_t ksSendBuf[KS_MAX_SEND_BUFFER_SIZE] = {0};
#endif
#if KSERIAL_RECV_ENABLE
uint8_t ksRecvBuf[KS_MAX_RECV_BUFFER_SIZE] = {0};
#endif

#if KSERIAL_RECV_TREAD_ENABLE
uint8_t ksPacketBuf[KSERIAL_RECV_PACKET_BUFFER_LENS] = {0};
kserial_packet_t ksPacket[KSERIAL_MAX_PACKET_LENS] = {0};
kserial_t ks =
{
    .size = KSERIAL_RECV_PACKET_BUFFER_LENS,
    .count = 0,
    .buffer = ksPacketBuf,
    .packet = ksPacket
};
#endif

const uint32_t KS_TYPE_SIZE[KSERIAL_TYPE_LENS] =
{
    1, 2, 4, 8,
    1, 2, 4, 8,
    0, 2, 4, 8,
    0, 0, 0, 0
};

const char KS_TYPE_STRING[KSERIAL_TYPE_LENS][4] =
{
    "U8", "U16", "U32", "U64",
    "I8", "I16", "I32", "I64",
    "R0", "F16", "F32", "F64",
    "R1", "R2",  "R3",  "R4",
};

const char KS_TYPE_FORMATE[KSERIAL_TYPE_LENS][8] =
{
    "%4d", "%6d", "%11d", "%20d",
    "%4d", "%6d", "%11d", "%20d",
    "",    "%.6f", "%.6f", "%.6f",
    "", "", "", ""
};

/* Prototypes ------------------------------------------------------------------------------*/
/* Functions -------------------------------------------------------------------------------*/

/**
 *  @brief  kSerial_GetTypeSize
 */
#if 1
#define kSerial_GetTypeSize(__TYPE)     KS_TYPE_SIZE[__TYPE]
#else
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
#endif

/**
 *  @brief  kSerial_CheckHeader
 */
uint32_t kSerial_CheckHeader( const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte )
{
    uint32_t checksum = 0;

    if ((packet[0] == 'K') && (packet[1] == 'S'))
    {
        *type = packet[2] >> 4;
        *nbyte = (((uint16_t)packet[2] << 8) | packet[3]) & 0x0FFF;
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
uint32_t kSerial_CheckEnd( const uint8_t *packet, uint32_t nbyte )
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
        *type = packet[2] >> 4;
        *nbyte = (((uint16_t)packet[2] << 8) | packet[3]) & 0x0FFF;
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
void kSerial_GetBytesData( const uint8_t *packet, void *pdata, uint32_t nbyte )
{
    for (uint32_t i = 0; i < nbyte; i++)
    {
        ((uint8_t*)pdata)[i] = packet[7 + i];
    }
}

/**
 *  @brief  kSerial_Pack
 */
uint32_t kSerial_Pack( uint8_t *packet, const void *param, uint32_t type, uint32_t lens, const void *pdata )
{
    uint32_t packetDataBytes;  // in bytes
    uint32_t checksum = 0;
    uint32_t typeSize = kSerial_GetTypeSize(type);

    packetDataBytes = (typeSize > 1) ? (lens * typeSize) : (lens);

    packet[0] = 'K';                                    // header 'K'
    packet[1] = 'S';                                    // header 'S'
    packet[2] = (type << 4) | (packetDataBytes >> 8);   // data type  (4bit)
    packet[3] = packetDataBytes;                        // data bytes (12bit)

    if (param != NULL)
    {
        packet[4] = ((uint8_t*)param)[0];               // parameter 1
        packet[5] = ((uint8_t*)param)[1];               // parameter 2
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
    packet[6] = checksum;                               // checksum

    if (pdata != NULL)
    {
        for (uint32_t i = 0; i < packetDataBytes; i++)
        {
            packet[7 + i] = ((uint8_t*)pdata)[i];       // data ...
        }
    }
    packet[7 + packetDataBytes] = '\r';                 // finish '\r'

    return (packetDataBytes + 8);
}

/**
 *  @brief  kSerial_Unpack
 */
uint32_t kSerial_Unpack( const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte, void *pdata )
{
    uint32_t status;

    status = kSerial_Check(packet, param, type, nbyte);
    if ((status == KS_OK) && (pdata != NULL))
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
uint32_t kSerial_UnpackBuffer( const uint8_t *buffer, uint32_t buffersize, kserial_packet_t *ksp, uint32_t *count )
{
    uint32_t status;
    uint32_t offset = 0;
    uint32_t newindex = 0;
    uint32_t typesize;

    *count = 0;

    while ((buffersize - offset) > 7)   // min packet bytes = 8
    {
        status = kSerial_Check(&buffer[offset], ksp[*count].param, &ksp[*count].type, &ksp[*count].nbyte);
        if (status == KS_OK)
        {
            if ((offset + ksp[*count].nbyte + 8) > buffersize)
            {
                break;
            }
            ksp[*count].data = (void *)malloc(ksp[*count].nbyte * sizeof(uint8_t));
            kSerial_GetBytesData(&buffer[offset], ksp[*count].data, ksp[*count].nbyte);
            typesize = kSerial_GetTypeSize(ksp[*count].type);
            if (typesize > 1)
            {
                ksp[*count].lens = ksp[*count].nbyte / typesize;
            }
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
}

/**
 *  @brief  kSerial_SendPacket
 */
uint32_t kSerial_SendPacket( void *param, void *pdata, uint32_t lens, uint32_t type )
{
#if KSERIAL_SEND_ENABLE
    uint32_t nbytes;
    nbytes = kSerial_Pack(ksSendBuf, param, type, lens, pdata);
    kSerial_Send(ksSendBuf, nbytes);
    // TODO: fix return
    return nbytes;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_RecvPacket
 */
uint32_t kSerial_RecvPacket( uint8_t input, void *param, void *pdata, uint32_t *lens, uint32_t *type )
{
#if KSERIAL_RECV_ENABLE
    static uint32_t index = 0;
    static uint32_t bytes = 0;
    static uint32_t point = 0;

    uint32_t state;
    uint32_t typeSize;

    ksRecvBuf[point] = input;
    if (point > 6)
    {
        if ((ksRecvBuf[point - 7] == 'K') && (ksRecvBuf[point - 6] == 'S'))
        {
            index = point - 7;
            bytes = (((ksRecvBuf[index + 2] << 8) | ksRecvBuf[index + 3]) & 0x0FFF) + 8;
        }
        if ((point - index + 1) == bytes)
        {
            state = kSerial_Unpack(&ksRecvBuf[index], param, type, lens, pdata);
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
    if (++point >= KS_MAX_RECV_BUFFER_SIZE)
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
 *  @brief  kSerial_GetFrequence
 */
// float kSerial_GetFrequence( uint32_t lens, uint32_t time, uint32_t count )
// {
//     // static uint64_t kslensLast = 0;
//     // static uint64_t ksTimeLast = 0;
//     static float frequence = 0;
//     // if ((time - ksTimeLast) >= count)
//     // {
//     //     frequence = ((float)lens - kslensLast) / (time - ksTimeLast) * 1000.0f;
//     //     kslensLast = lens;
//     //     ksTimeLast = time;
//     // }
//     return frequence;
// }

/**
 *  @brief  kSerial_GetPacketData
 */
void kSerial_GetPacketData( kserial_packet_t *ksp, void *pdata, uint32_t index )
{
    if (pdata != NULL)
    {
        memcpy(pdata, ksp[index].data, ksp[index].nbyte);
    }
    free(ksp[index].data);
}

/**
 *  @brief  kSerial_ContinuousRead
 */
uint32_t kSerial_ContinuousRead( kserial_packet_t *ksp, uint32_t *index, uint32_t *count, uint32_t *total )
{
#if KSERIAL_RECV_TREAD_ENABLE
    if ((*count == 0) || (*index >= *count))
    {
        *count = kSerial_Read(&ks);
        if (*count == 0)
        {
            return KS_ERROR;
        }
        *index = 0;
    }
    kSerial_GetPacketData(ks.packet, ksp->data, *index);
    ksp->param[0] = ks.packet[*index].param[0];
    ksp->param[1] = ks.packet[*index].param[1];
    ksp->type = ks.packet[*index].type;
    ksp->lens = ks.packet[*index].lens;
    ksp->nbyte = ks.packet[*index].nbyte;
    (*total)++;
    (*index)++;
    return KS_OK;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_SendCommand
 *  Send packet ['K', 'S', type, 0, param1, param2, ck, '\r']
 *  Recv packet ['K', 'S', type, 0, param1, param2, ck, '\r']
 */
uint32_t kSerial_SendCommand( uint32_t type, uint32_t p1, uint32_t p2, kserial_ack_t *ack )
{
#if KSERIAL_SEND_ENABLE
    uint8_t param[2] = {p1, p2};
    uint32_t nbytes;
    uint32_t status = KS_OK;

#if KSERIAL_RECV_ENABLE
    if (ack != NULL)
    {
        kSerial_RecvFlush();
    }
#endif
    nbytes = kSerial_Pack(ksSendBuf, param, type, 0, NULL);
    kSerial_Send(ksSendBuf, nbytes);
#if KSERIAL_RECV_ENABLE
    if (ack != NULL)
    {
#if 0
        nbytes = 0;
        while (nbytes == 0)
        {
            kSerial_Delay(50);
            nbytes = kSerial_Recv(ksRecvBuf, KS_MAX_RECV_BUFFER_SIZE);
        }
#else
        kSerial_Delay(50);
        nbytes = kSerial_Recv(ksRecvBuf, KS_MAX_RECV_BUFFER_SIZE);
#endif
        status = kSerial_Unpack(ksRecvBuf, ack->param, &ack->type, &ack->nbyte, ack->data);
    }
#endif
    return status;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_DeviceCheck
 *  Send packet ['K', 'S', R0, 0, 0xD0,   0, ck, '\r']
 *  Recv packet ['K', 'S', R0, 0,  IDL, IDH, ck, '\r']
 */
uint32_t kSerial_DeviceCheck( uint32_t *id )
{
#if KSERIAL_CMD_ENABLE
    kserial_ack_t ack = {0};
    if (kSerial_SendCommand(KS_R0, KSCMD_R0_DEVICE_ID, 0x00, &ack) != KS_OK)
    {
        return KS_ERROR;
    }
    if (ack.type != KS_R0)
    {
        return KS_ERROR;
    }
    if (id != NULL)
    {
        *id = (uint32_t)*(uint16_t*)ack.param;
    }
    return KS_OK;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_TwiWriteReg
 *  Send packet ['K', 'S', R1, 1, slaveAddress(8-bit), regAddress, ck, regData, '\r']
 */
uint32_t kSerial_TwiWriteReg( uint8_t slaveAddr, uint8_t regAddr, uint8_t regData )
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {slaveAddr << 1, regAddr};
    uint32_t type = KS_R1;
    uint32_t nbytes;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuf, param, type, 1, &regData);
    kSerial_Send(ksSendBuf, nbytes);
#if 0
    klogd("[W] param = %02X, %02X, type = %d, bytes = %d, data = %02X\n", param[0], param[1], type, nbytes, wdata);
#endif
    return nbytes;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_TwiReadRegs
 *  Send packet ['K', 'S', R1,    1, slaveAddress(8-bit)+1, regAddress, ck, lens, '\r']
 *  Recv packet ['K', 'S', R1, lens, slaveAddress(8-bit)+1, regAddress, ck, regData ..., '\r']
 */
uint32_t kSerial_TwiReadRegs( uint8_t slaveAddr, uint8_t regAddr, uint8_t *regData, uint8_t lens )
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {(slaveAddr << 1) + 1, regAddr};
    uint32_t type = KS_R1;
    uint32_t nbytes;
    uint32_t status;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuf, param, type, 1, &lens);
    kSerial_Send(ksSendBuf, nbytes);

    nbytes = 0;
    while (nbytes == 0)
    {
        kSerial_Delay(100);
        nbytes = kSerial_Recv(ksRecvBuf, KS_MAX_RECV_BUFFER_SIZE);
    }

    // TODO: check i2cbuff first 'KS'
    status = kSerial_Unpack(ksRecvBuf, param, &type, &nbytes, ksSendBuf);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < nbytes; i++)
        {
            regData[i] = ksSendBuf[i];
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
 *  @brief  kSerial_TwiWriteRegs
 *  Send packet ['K', 'S', R1, lens, slaveAddress(8-bit), regAddress, ck, regData ... , '\r']
 */
uint32_t kSerial_TwiWriteRegs( uint8_t slaveAddr, uint8_t regAddr, uint8_t *regData, uint8_t lens )
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {slaveAddr << 1, regAddr};
    uint32_t type = KS_R1;
    uint32_t nbytes;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuf, param, type, lens, regData);
    kSerial_Send(ksSendBuf, nbytes);
#if 0
    klogd("[W] param = %02X, %02X, type = %d, bytes = %d, data = %02X\n", param[0], param[1], type, nbytes, wdata);
#endif
    return nbytes;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kSerial_TwiScanDevice
 *  Send packet ['K', 'S', R2,    0, 0xA1, 0, ck, '\r']
 *  Recv packet ['K', 'S', R2, lens, 0xA1, 0, ck, address ..., '\r']
 */
uint32_t kSerial_TwiScanDevice( uint8_t *slaveAddr )
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {KSCMD_R2_TWI_SCAN_DEVICE, 0};
    uint32_t type = KS_R2;
    uint32_t nbytes;
    uint32_t status;
    uint32_t count;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuf, param, type, 0, NULL);
    kSerial_Send(ksSendBuf, nbytes);

    kSerial_Delay(100);
    nbytes = kSerial_Recv(ksRecvBuf, KS_MAX_RECV_BUFFER_SIZE);

    // TODO: check i2cbuff first 'KS'
    status = kSerial_Unpack(ksRecvBuf, param, &type, &count, ksSendBuf);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < count; i++)
        {
            slaveAddr[i] = ksSendBuf[i];
        }
#if 0
        klogd(" >> i2c device list (found %d device)\n\n", count);
        klogd("    ");
        for (uint32_t i = 0; i < count; i++)
        {
            klogd(" %02X", slaveAddr[i]);
        }
        klogd("\n\n");
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
 *  Send packet ['K', 'S', R2,   0, 0xA2, slaveAddress, ck, '\r']
 *  Recv packet ['K', 'S', R2, 256, 0xA2, slaveAddress, ck, address ..., '\r']
 */
uint32_t kSerial_TwiScanRegister( uint8_t slaveAddr, uint8_t reg[256] )
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {KSCMD_R2_TWI_SCAN_REGISTER, slaveAddr << 1};
    uint32_t type = KS_R2;
    uint32_t nbytes;
    uint32_t status;

    kSerial_RecvFlush();

    nbytes = kSerial_Pack(ksSendBuf, param, type, 0, NULL);
    kSerial_Send(ksSendBuf, nbytes);

    kSerial_Delay(100);
    nbytes = kSerial_Recv(ksRecvBuf, KS_MAX_RECV_BUFFER_SIZE);

    // TODO: check i2cbuff first 'KS'
    status = kSerial_Unpack(ksRecvBuf, param, &type, &nbytes, ksSendBuf);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < 256; i++)
        {
            reg[i] = ksSendBuf[i];
        }
#if 0
        klogd("\n");
        klogd(" >> i2c device register (address 0x%02X)\n\n", slaveAddr);
        prklogdintf("      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
        for (uint32_t i = 0; i < 256; i += 16)
        {
            klogd(" %02X: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n",
                i,
                reg[i +  0], reg[i +  1], reg[i +  2], reg[i +  3],
                reg[i +  4], reg[i +  5], reg[i +  6], reg[i +  7],
                reg[i +  8], reg[i +  9], reg[i + 10], reg[i + 11],
                reg[i + 12], reg[i + 13], reg[i + 14], reg[i + 15]
            );
        }
        klogd("\n\n");
#endif
    }
    return status;
#else
    return KS_ERROR;
#endif
}

/*************************************** END OF FILE ****************************************/
