/**
 *      __            ____
 *     / /__ _  __   / __/                      __  
 *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
 *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
 *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
 *                    /_/   github.com/KitSprout    
 * 
 *  @file    kserial.c
 *  @author  KitSprout
 *  @brief   kserial packet format :
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
#include "kserial.h"

/* Define ----------------------------------------------------------------------------------*/
/* Macro -----------------------------------------------------------------------------------*/
/* Typedef ---------------------------------------------------------------------------------*/
/* Variables -------------------------------------------------------------------------------*/

const char KSERIAL_VERSION[] = KSERIAL_VERSION_DEFINE;

#if KSERIAL_SEND_ENABLE
static uint8_t sbuffer[KS_MAX_SEND_BUFFER_SIZE] = {0};
#endif
#if KSERIAL_RECV_ENABLE
static uint8_t rbuffer[KS_MAX_RECV_BUFFER_SIZE] = {0};
#endif

#if KSERIAL_RECV_TREAD_ENABLE
static uint8_t pkbuffer[KSERIAL_RECV_PACKET_BUFFER_LENS] = {0};
static kserial_packet_t kspacket[KSERIAL_MAX_PACKET_LENS] = {0};
kserial_t ks =
{
    .size = KSERIAL_RECV_PACKET_BUFFER_LENS,
    .count = 0,
    .buffer = pkbuffer,
    .packet = kspacket
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
 *  @brief  kserial_get_typesize
 */
#if 1
#define kserial_get_typesize(__TYPE)     KS_TYPE_SIZE[__TYPE]
#else
uint32_t kserial_get_typesize(uint32_t type)
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
 *  @brief  kserial_check_header
 */
uint32_t kserial_check_header(const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte)
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
 *  @brief  kserial_check_end
 */
uint32_t kserial_check_end(const uint8_t *packet, uint32_t nbyte)
{
    if (packet[nbyte + 8 - 1] == '\r')
    {
        return KS_OK;
    }

    return KS_ERROR;
}

/**
 *  @brief  kserial_check
 */
uint32_t kserial_check(const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte)
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
 *  @brief  kserial_get_bytesdata
 */
void kserial_get_bytesdata(const uint8_t *packet, void *pdata, uint32_t nbyte)
{
    for (uint32_t i = 0; i < nbyte; i++)
    {
        ((uint8_t*)pdata)[i] = packet[7 + i];
    }
}

/**
 *  @brief  kserial_pack
 */
uint32_t kserial_pack(uint8_t *packet, const void *param, uint32_t type, uint32_t lens, const void *pdata)
{
    uint32_t databytes;  // in bytes
    uint32_t checksum = 0;
    uint32_t typesize = kserial_get_typesize(type);

    databytes = (typesize > 1) ? (lens * typesize) : (lens);

    packet[0] = 'K';                                // header 'K'
    packet[1] = 'S';                                // header 'S'
    packet[2] = (type << 4) | (databytes >> 8);     // data type  (4bit)
    packet[3] = databytes;                          // data bytes (12bit)

    if (param != NULL)
    {
        packet[4] = ((uint8_t*)param)[0];           // parameter 1
        packet[5] = ((uint8_t*)param)[1];           // parameter 2
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
    packet[6] = checksum;                           // checksum

    if (pdata != NULL)
    {
        for (uint32_t i = 0; i < databytes; i++)
        {
            packet[7 + i] = ((uint8_t*)pdata)[i];   // data ...
        }
    }
    packet[7 + databytes] = '\r';                   // finish '\r'

    return (databytes + 8);
}

/**
 *  @brief  kserial_unpack
 */
uint32_t kserial_unpack(const uint8_t *packet, void *param, uint32_t *type, uint32_t *nbyte, void *pdata)
{
    uint32_t status;

    status = kserial_check(packet, param, type, nbyte);
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
 *  @brief  kserial_unpack_buffer
 */
uint32_t kserial_unpack_buffer(const uint8_t *buffer, uint32_t buffersize, kserial_packet_t *ksp, uint32_t *count)
{
    uint32_t status;
    uint32_t offset = 0;
    uint32_t newindex = 0;
    uint32_t typesize;

    *count = 0;

    while ((buffersize - offset) > 7)   // min packet bytes = 8
    {
        status = kserial_check(&buffer[offset], ksp[*count].param, &ksp[*count].type, &ksp[*count].nbyte);
        if (status == KS_OK)
        {
            if ((offset + ksp[*count].nbyte + 8) > buffersize)
            {
                break;
            }
            ksp[*count].data = (void *)malloc(ksp[*count].nbyte * sizeof(uint8_t));
            kserial_get_bytesdata(&buffer[offset], ksp[*count].data, ksp[*count].nbyte);
            typesize = kserial_get_typesize(ksp[*count].type);
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
 *  @brief  kserial_send_packet
 */
uint32_t kserial_send_packet(void *param, void *pdata, uint32_t lens, uint32_t type)
{
#if KSERIAL_SEND_ENABLE
    uint32_t nbytes;
    nbytes = kserial_pack(sbuffer, param, type, lens, pdata);
    kserial_send(sbuffer, nbytes);
    // TODO: fix return
    return nbytes;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kserial_recv_packet
 */
uint32_t kserial_recv_packet(uint8_t input, void *param, void *pdata, uint32_t *lens, uint32_t *type)
{
#if KSERIAL_RECV_ENABLE
    static uint32_t index = 0;
    static uint32_t bytes = 0;
    static uint32_t point = 0;

    uint32_t state;
    uint32_t typesize;

    rbuffer[point] = input;
    if (point > 6)
    {
        if ((rbuffer[point - 7] == 'K') && (rbuffer[point - 6] == 'S'))
        {
            index = point - 7;
            bytes = (((rbuffer[index + 2] << 8) | rbuffer[index + 3]) & 0x0FFF) + 8;
        }
        if ((point - index + 1) == bytes)
        {
            state = kserial_unpack(&rbuffer[index], param, type, lens, pdata);
            if (state == KS_OK)
            {
                point = 0;
                index = 0;
                bytes = 0;
                typesize = kserial_get_typesize(*type);
                if (typesize > 1)
                {
                    *lens /= typesize;
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
 *  @brief  kserial_read
 */
uint32_t kserial_read(kserial_t *ks)
{
#if KSERIAL_RECV_ENABLE
    uint32_t available = 0;
    uint32_t nbyte;
    uint32_t newindex;

    do
    {   // add rx data to packet buffer
        nbyte = kserial_recv(&ks->buffer[ks->count], ks->size - ks->count);
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
        newindex = kserial_unpack_buffer(ks->buffer, ks->count, ks->packet, &ks->pkcnt);
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
 *  @brief  kserial_flush_read
 */
void kserial_flush_read(kserial_t *ks)
{
#if KSERIAL_RECV_ENABLE
    kserial_flush_recv();
    memset(ks->buffer, 0, ks->size);
    ks->count = 0;
#endif
}

/**
 *  @brief  kserial_get_frequence
 */
// float kserial_get_frequence(uint32_t lens, uint32_t time, uint32_t count)
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
 *  @brief  kserial_get_packetdata
 */
void kserial_get_packetdata(kserial_packet_t *ksp, void *pdata, uint32_t index)
{
    if (pdata != NULL)
    {
        memcpy(pdata, ksp[index].data, ksp[index].nbyte);
    }
    free(ksp[index].data);
}

/**
 *  @brief  kserial_read_continuous
 */
uint32_t kserial_read_continuous(kserial_packet_t *ksp, uint32_t *index, uint32_t *count, uint32_t *total)
{
#if KSERIAL_RECV_TREAD_ENABLE
    if ((*count == 0) || (*index >= *count))
    {
        *count = kserial_read(&ks);
        if (*count == 0)
        {
            return KS_ERROR;
        }
        *index = 0;
    }
    kserial_get_packetdata(ks.packet, ksp->data, *index);
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
 *  @brief  kscmd_send_command
 *  Send packet ['K', 'S', type, 0, param1, param2, ck, '\r']
 *  Recv packet ['K', 'S', type, 0, param1, param2, ck, '\r']
 */
uint32_t kscmd_send_command(uint32_t type, uint32_t param1, uint32_t param2, kserial_ack_t *ack)
{
#if KSERIAL_SEND_ENABLE
    uint8_t param[2] = {param1, param2};
    uint32_t nbytes;
    uint32_t status = KS_OK;

#if KSERIAL_RECV_ENABLE
    if (ack != NULL)
    {
        kserial_flush_recv();
    }
#endif
    nbytes = kserial_pack(sbuffer, param, type, 0, NULL);
    kserial_send(sbuffer, nbytes);
#if KSERIAL_RECV_ENABLE
    if (ack != NULL)
    {
#if 0
        nbytes = 0;
        while (nbytes == 0)
        {
            kserial_delay(50);
            nbytes = kserial_recv(rbuffer, KS_MAX_RECV_BUFFER_SIZE);
        }
#else
        kserial_delay(50);
        nbytes = kserial_recv(rbuffer, KS_MAX_RECV_BUFFER_SIZE);
#endif
        status = kserial_unpack(rbuffer, ack->param, &ack->type, &ack->nbyte, ack->data);
    }
#endif
    return status;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kscmd_check_device
 *  Send packet ['K', 'S', R0, 0, 0xD0,   0, ck, '\r']
 *  Recv packet ['K', 'S', R0, 0,  IDL, IDH, ck, '\r']
 */
uint32_t kscmd_check_device(uint32_t *id)
{
#if KSERIAL_CMD_ENABLE
    kserial_ack_t ack = {0};
    if (kscmd_send_command(KS_R0, KSCMD_R0_DEVICE_ID, 0x00, &ack) != KS_OK)
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
 *  @brief  kscmd_set_baudrate
 *  Send packet ['K', 'S', R0, 4, 0xD1, 4, ck, BAUD[0:7], BAUD[8:15], BAUD[16:23], BAUD[24:31], '\r']
 */
uint32_t kscmd_set_baudrate(int32_t baudrate)
{
    if (baudrate < 0)
    {
        return KS_ERROR;
    }
    uint8_t param[2] = {KSCMD_R0_DEVICE_BAUDRATE, 4};
    return kserial_send_packet(param, &baudrate, param[1], KS_R0);
}

/**
 *  @brief  kscmd_set_updaterate
 *  Send packet ['K', 'S', R0, 4, 0xD2, 4, ck, FREQ[0:7], FREQ[8:15], FREQ[16:23], FREQ[24:31], '\r']
 */
uint32_t kscmd_set_updaterate(int32_t updaterate)
{
    if (updaterate < 0)
    {
        return KS_ERROR;
    }
    uint8_t param[2] = {KSCMD_R0_DEVICE_RATE, 4};
    return kserial_send_packet(param, &updaterate, param[1], KS_R0);
}

/**
 *  @brief  kscmd_set_mode
 *  Send packet ['K', 'S', R0, 4, 0xD3, 4, ck, MODE[0:7], MODE[8:15], MODE[16:23], MODE[24:31], '\r']
 */
uint32_t kscmd_set_mode(int32_t mode)
{
    if (mode < 0)
    {
        return KS_ERROR;
    }
    return kscmd_send_command(KS_R0, KSCMD_R0_DEVICE_MDOE, mode, NULL);
}

/**
 *  @brief  kscmd_get_value
 *  Send packet ['K', 'S', R0, 0, 0xE3, ITEM, ck, '\r']
 *  Recv packet ['K', 'S', R0, 0, 0xE3, ITEM, ck, VAL[0:7], VAL[8:15], VAL[16:23], VAL[24:31], '\r']
 */
uint32_t kscmd_get_value(uint32_t item, int32_t *value)
{
    kserial_ack_t ack = {0};
    if (kscmd_send_command(KS_R0, KSCMD_R0_DEVICE_GET, item, &ack) != KS_OK)
    {
        *value = 0;
        return KS_ERROR;
    }
    *value = *(uint32_t*)ack.data;
    return KS_OK;
}

/**
 *  @brief  kscmd_twi_writereg
 *  Send packet ['K', 'S', R1, 1, slaveAddress(8-bit), regAddress, ck, regData, '\r']
 */
uint32_t kscmd_twi_writereg(uint8_t slaveaddr, uint8_t regaddr, uint8_t regdata)
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {slaveaddr << 1, regaddr};
    uint32_t type = KS_R1;
    uint32_t nbytes;

    kserial_flush_recv();

    nbytes = kserial_pack(sbuffer, param, type, 1, &regdata);
    kserial_send(sbuffer, nbytes);
#if 0
    klogd("[W] param = %02X, %02X, type = %d, bytes = %d, data = %02X\n", param[0], param[1], type, nbytes, wdata);
#endif
    return nbytes;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kscmd_twi_readregs
 *  Send packet ['K', 'S', R1,    1, slaveAddress(8-bit)+1, regAddress, ck, lens, '\r']
 *  Recv packet ['K', 'S', R1, lens, slaveAddress(8-bit)+1, regAddress, ck, regData ..., '\r']
 */
uint32_t kscmd_twi_readregs(uint8_t slaveaddr, uint8_t regaddr, uint8_t *regdata, uint8_t lens)
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {(slaveaddr << 1) + 1, regaddr};
    uint32_t type = KS_R1;
    uint32_t nbytes;
    uint32_t status;

    kserial_flush_recv();

    nbytes = kserial_pack(sbuffer, param, type, 1, &lens);
    kserial_send(sbuffer, nbytes);

    nbytes = 0;
    while (nbytes == 0)
    {
        kserial_delay(100);
        nbytes = kserial_recv(rbuffer, KS_MAX_RECV_BUFFER_SIZE);
    }

    // TODO: check i2cbuff first 'KS'
    status = kserial_unpack(rbuffer, param, &type, &nbytes, sbuffer);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < nbytes; i++)
        {
            regdata[i] = sbuffer[i];
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
 *  @brief  kscmd_twi_writeregs
 *  Send packet ['K', 'S', R1, lens, slaveAddress(8-bit), regAddress, ck, regData ... , '\r']
 */
uint32_t kscmd_twi_writeregs(uint8_t slaveaddr, uint8_t regaddr, uint8_t *regdata, uint8_t lens)
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {slaveaddr << 1, regaddr};
    uint32_t type = KS_R1;
    uint32_t nbytes;

    kserial_flush_recv();

    nbytes = kserial_pack(sbuffer, param, type, lens, regdata);
    kserial_send(sbuffer, nbytes);
#if 0
    klogd("[W] param = %02X, %02X, type = %d, bytes = %d, data = %02X\n", param[0], param[1], type, nbytes, wdata);
#endif
    return nbytes;
#else
    return KS_ERROR;
#endif
}

/**
 *  @brief  kscmd_twi_scandevice
 *  Send packet ['K', 'S', R2,    0, 0xA1, 0, ck, '\r']
 *  Recv packet ['K', 'S', R2, lens, 0xA1, 0, ck, address ..., '\r']
 */
uint32_t kscmd_twi_scandevice(uint8_t *slaveaddr)
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {KSCMD_R2_TWI_SCAN_DEVICE, 0};
    uint32_t type = KS_R2;
    uint32_t nbytes;
    uint32_t status;
    uint32_t count;

    kserial_flush_recv();

    nbytes = kserial_pack(sbuffer, param, type, 0, NULL);
    kserial_send(sbuffer, nbytes);

    kserial_delay(100);
    nbytes = kserial_recv(rbuffer, KS_MAX_RECV_BUFFER_SIZE);

    // TODO: check i2cbuff first 'KS'
    status = kserial_unpack(rbuffer, param, &type, &count, sbuffer);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < count; i++)
        {
            slaveaddr[i] = sbuffer[i];
        }
#if 0
        klogd(" >> i2c device list (found %d device)\n\n", count);
        klogd("    ");
        for (uint32_t i = 0; i < count; i++)
        {
            klogd(" %02X", slaveaddr[i]);
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
 *  @brief  kscmd_twi_scanregister
 *  Send packet ['K', 'S', R2,   0, 0xA2, slaveAddress, ck, '\r']
 *  Recv packet ['K', 'S', R2, 256, 0xA2, slaveAddress, ck, address ..., '\r']
 */
uint32_t kscmd_twi_scanregister(uint8_t slaveaddr, uint8_t reg[256])
{
#if KSERIAL_CMD_ENABLE
    uint8_t param[2] = {KSCMD_R2_TWI_SCAN_REGISTER, slaveaddr << 1};
    uint32_t type = KS_R2;
    uint32_t nbytes;
    uint32_t status;

    kserial_flush_recv();

    nbytes = kserial_pack(sbuffer, param, type, 0, NULL);
    kserial_send(sbuffer, nbytes);

    kserial_delay(100);
    nbytes = kserial_recv(rbuffer, KS_MAX_RECV_BUFFER_SIZE);

    // TODO: check i2cbuff first 'KS'
    status = kserial_unpack(rbuffer, param, &type, &nbytes, sbuffer);
    if (status == KS_OK)
    {
        for (uint32_t i = 0; i < 256; i++)
        {
            reg[i] = sbuffer[i];
        }
#if 0
        klogd("\n");
        klogd(" >> i2c device register (address 0x%02X)\n\n", slaveaddr);
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
