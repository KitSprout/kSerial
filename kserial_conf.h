/**
 *      __            ____
 *     / /__ _  __   / __/                      __  
 *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
 *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
 *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
 *                    /_/   github.com/KitSprout    
 * 
 *  @file    kserial_conf.h
 *  @author  KitSprout
 *  @brief   
 * 
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef __KSERIAL_CONF_H
#define __KSERIAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* Define ----------------------------------------------------------------------------------*/

#ifndef KSERIAL_SEND_ENABLE
#define KSERIAL_SEND_ENABLE                             (1U)
#ifndef KS_MAX_SEND_BUFFER_SIZE
#define KS_MAX_SEND_BUFFER_SIZE                         (4096 + 32)
#endif
#endif

#ifndef KSERIAL_RECV_ENABLE
#define KSERIAL_RECV_ENABLE                             (1U)
#ifndef KS_MAX_RECV_BUFFER_SIZE
#define KS_MAX_RECV_BUFFER_SIZE                         (4096 + 1024 + 32)
#endif
#endif

#ifndef KSERIAL_RECV_TREAD_ENABLE
#define KSERIAL_RECV_TREAD_ENABLE                       (1U)
#define KSERIAL_MAX_PACKET_LENS                         (4096)
#define KSERIAL_RECV_PACKET_BUFFER_LENS                 (64 * 1024)
#endif

#ifndef KSERIAL_CMD_ENABLE
#define KSERIAL_CMD_ENABLE                              (1U)
#endif

#if KSERIAL_RECV_TREAD_ENABLE
#if !(KSERIAL_RECV_ENABLE)
#error "Need to enable recv"
#endif
#endif
#if KSERIAL_CMD_ENABLE
#if !(KSERIAL_SEND_ENABLE && KSERIAL_RECV_ENABLE)
#error "Need to enable send and recv"
#endif
#endif

#define KSERIAL_TYPE_LENS                               (16)

/* Includes --------------------------------------------------------------------------------*/

#if (KSERIAL_SEND_ENABLE || KSERIAL_RECV_ENABLE)
#include "serial.h"
#endif

/* Macro -----------------------------------------------------------------------------------*/

#if KSERIAL_SEND_ENABLE
#ifndef kserial_send
#define kserial_send(__DATA, __LENS)                    serial_send_data(&s, __DATA, __LENS)
#define kserial_sendbyte(__DATA)                        serial_send_byte(&s, __DATA)
#endif
#endif
#if KSERIAL_RECV_ENABLE
#define kserial_recv(__DATA, __LENS)                    serial_recv_data(&s, __DATA, __LENS)
#define kserial_recvbyte()                              serial_recv_byte(&s)
#define kserial_flush_recv()                            serial_flush(&s)
#endif
#if (KSERIAL_SEND_ENABLE || KSERIAL_RECV_ENABLE)
#define kserial_delay(__MS)                             serial_delay(__MS)
#endif

#ifdef __cplusplus
}
#endif

#endif

/*************************************** END OF FILE ****************************************/
