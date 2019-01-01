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
 *  @date    01-JAN-2019
 *  @brief   kSerial packet format :
 *           byte 1   : header 'K' (75)   [HK]
 *           byte 2   : header 'S' (83)   [HS]
 *           byte 3   : total length      [L ]
 *           byte 4   : data type         [T ]
 *           byte 5   : parameter 1       [P1]
 *           byte 6   : parameter 2       [P2]
 *           byte 7   : checksum          [CK]
 *            ...
 *           byte L-1 : data              [DN]
 *           byte L   : finish '\r' (13)  [ER]
 */

/* Includes --------------------------------------------------------------------------------*/

#include "serial.h"
#include "kSerial.h"

/** @addtogroup KS_Application
  * @{
  */

/* Private typedef -------------------------------------------------------------------------*/
/* Private define --------------------------------------------------------------------------*/
#define KS_MAX_SEND_BUFF_SIZE   (1024)
#define KS_MAX_RECV_BUFF_SIZE   (1024 + 1024)

/* Private macro ---------------------------------------------------------------------------*/
#define kSerial_Send(__DATA, __LENS)    Serial_SendBytes(__DATA, __LENS)
#define kSerial_Recv()                  Serial_RecvByte()

/* Private variables -----------------------------------------------------------------------*/
static uint8_t ksSendBuff[KS_MAX_SEND_BUFF_SIZE] = {0};
static uint8_t ksRecvBuff[KS_MAX_RECV_BUFF_SIZE] = {0};

/* Private function prototypes -------------------------------------------------------------*/
/* Private functions -----------------------------------------------------------------------*/

/**
 *  @brief  kSerial_Check
 */
uint32_t kSerial_Check( uint8_t *packet, void *param, uint8_t *type, uint16_t *lens )
{
    uint16_t checksum = 0;

    if ((packet[0] == 'K') && (packet[1] == 'S'))
    {
        *lens = (((uint16_t)packet[3] << 2) & 0x0300) | packet[2];
        if (*lens < 8)
        {
            return KS_ERROR;
        }
        for (uint32_t i = 0; i < 6; i++)
        {
            checksum += packet[i];
        }
        checksum &= 0x00FF;
        if ((packet[6] == checksum) && (packet[*lens - 1] == '\r') )
        {
            *type = packet[3] & 0x3F;
            ((uint8_t*)param)[0] = packet[4];
            ((uint8_t*)param)[1] = packet[5];
            return KS_OK;
        }
    }

    return KS_ERROR;
}

/**
 *  @brief  kSerial_Pack
 */
uint32_t kSerial_Pack( uint8_t *packet, void *param, const uint8_t type, const uint16_t lens, void *pdata )
{
    const uint16_t packetDataBytes = lens * getTypeSize(type);
    const uint16_t packetTotalBytes = packetDataBytes + 8;
    const uint8_t lensHiBit = (packetTotalBytes & 0x0300) >> 2;

    uint16_t checksum = 0;

    packet[0] = 'K';                          /* header 'K'   */
    packet[1] = 'S';                          /* header 'S'   */
    packet[2] = packetTotalBytes;             /* total length */
    packet[3] = lensHiBit | type;             /* data type    */

    if (param != NULL)
    {
        packet[4] = ((uint8_t*)param)[0];     /* parameter 1  */
        packet[5] = ((uint8_t*)param)[1];     /* parameter 2  */
    }
    else
    {
        packet[4] = 0;
        packet[5] = 0;
    }
    for (uint32_t i = 0; i < 6; i++)
    {
        checksum += packet[i];
    }
    packet[6] = checksum & 0x00FF;            /* checksum     */

    for (uint32_t i = 0; i < packetDataBytes; i++)
    {
        packet[7 + i] = ((uint8_t*)pdata)[i]; /* data ....... */
    }

    packet[7 + packetDataBytes] = '\r';       /* finish '\r'  */

    return KS_OK;
}

/**
 *  @brief  kSerial_Unpack
 */
uint32_t kSerial_Unpack( uint8_t *packet, void *param, uint8_t *type, uint16_t *lens, void *pdata )
{
    uint16_t checksum = 0;

    if ((packet[0] == 'K') && (packet[1] == 'S'))
    {
        *lens = (((uint16_t)packet[3] << 2) & 0x0300) | packet[2];
        if (*lens < 8)
        {
            return KS_ERROR;
        }
        for (uint32_t i = 0; i < 6; i++)
        {
            checksum += packet[i];
        }
        checksum &= 0x00FF;
        if ((packet[6] == checksum) && (packet[*lens - 1] == '\r') )
        {
            *type = packet[3] & 0x3F;
            ((uint8_t*)param)[0] = packet[4];
            ((uint8_t*)param)[1] = packet[5];
            for (uint32_t i = 0; i < *lens; i++ )
            {
                ((uint8_t*)pdata)[i] = packet[7 + i];
            }
            return KS_OK;
        }
    }

    return KS_ERROR;
}

/**
 *  @brief  kSerial_SendPacket
 */
uint32_t kSerial_SendPacket( void *param, void *sdata, const uint16_t lens, const uint8_t type )
{
    uint32_t totalBytes;

    kSerial_Pack(ksSendBuff, param, type, lens, sdata);
    totalBytes = lens * getTypeSize(type) + 8;
    kSerial_Send(ksSendBuff, totalBytes);

    return KS_OK;
}

/**
 *  @brief  kSerial_RecvPacket
 */
uint32_t kSerial_RecvPacket( void *param, void *rdata, uint16_t *lens, uint8_t *type )
{
    static uint16_t point = 0;
    static uint16_t index = 0;
    static uint16_t bytes = 0;

    uint32_t state;

    ksRecvBuff[point] = kSerial_Recv();

    if (point > 1)
    {
        if ((ksRecvBuff[point - 2] == 'K') && (ksRecvBuff[point - 1] == 'S'))
        {
            index = point - 2;
            bytes = (((uint16_t)ksRecvBuff[3] << 2) & 0x0300) | ksRecvBuff[2];
        }
        if ((point - index + 1) == bytes)
        {
            state = kSerial_Unpack(&(ksRecvBuff[index]), param, type, lens, rdata);
            if (state == KS_OK)
            {
                point = 0;
                index = 0;
                bytes = 0;
                return KS_OK;
            }
        }
    }

    if (++point >= KS_MAX_RECV_BUFF_SIZE)
    {
        point = 0;
    }

    return KS_ERROR;
}

/**
 *  @brief  kSerial_GetPacketDataLens
 */
uint16_t kSerial_GetPacketDataLens( uint16_t lens, uint8_t type )
{
    switch (type & 0x0F)
    {
        case 0x01:  break;
        case 0x02:  lens >>= 1; break;
        case 0x04:  lens >>= 2; break;
        case 0x08:  lens >>= 3; break;
        default:    return 0;
    }
    return lens;
}

/*************************************** END OF FILE ****************************************/
