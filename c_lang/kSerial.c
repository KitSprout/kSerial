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
 *  @date    Jan-2020
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

/* Define ----------------------------------------------------------------------------------*/
/* Macro -----------------------------------------------------------------------------------*/

#if KSERIAL_SEND_ENABLE
#define kSerial_Send(__DATA, __LENS)    Serial_SendData(__DATA, __LENS)
#endif
#if KSERIAL_RECV_ENABLE
#define kSerial_Recv()                  Serial_RecvByte()
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
uint32_t kSerial_CheckHeader( uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte )
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
uint32_t kSerial_CheckEnd( uint8_t *packet, const uint32_t nbyte )
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
uint32_t kSerial_Check( uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte )
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
void kSerial_GetBytesData( uint8_t *packet, void *pdata, uint32_t *nbyte )
{
    for (uint32_t i = 0; i < *nbyte; i++)
    {
        ((uint8_t*)pdata)[i] = packet[7 + i];
    }
}

/**
 *  @brief  kSerial_Pack
 */
uint32_t kSerial_Pack( uint8_t *packet, const void *param, const uint32_t type, const uint32_t lens, void *pdata )
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
uint32_t kSerial_Unpack( uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte, void *pdata )
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
 *  @brief  kSerial_SendPacket
 */
uint32_t kSerial_SendPacket( void *param, void *sdata, const uint32_t lens, const uint32_t type )
{
#if KSERIAL_SEND_ENABLE
    uint32_t sendBytes;
    sendBytes = kSerial_Pack(ksSendBuff, param, type, lens, sdata);
    kSerial_Send(ksSendBuff, sendBytes);
    return sendBytes;
#else
    return 0;
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

    ksRecvBuff[point] = kSerial_Recv();
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
#endif

    return KS_ERROR;
}

/*************************************** END OF FILE ****************************************/
