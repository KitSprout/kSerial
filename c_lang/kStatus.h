/**
 *      __            ____
 *     / /__ _  __   / __/                      __  
 *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
 *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
 *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
 *                    /_/   github.com/KitSprout    
 * 
 *  @file    kStatus.h
 *  @author  KitSprout
 *  @date    Jan-2020
 *  @brief   
 * 
 */

/* Define to prevent recursive inclusion ---------------------------------------------------*/
#ifndef __KSTATUS_H
#define __KSTATUS_H

#ifdef __cplusplus
    extern "C" {
#endif

/* Includes --------------------------------------------------------------------------------*/
/* Define ----------------------------------------------------------------------------------*/

#ifndef NULL
#ifdef __cplusplus
#define NULL                            0
#else
#define NULL                            ((void *)0)
#endif
#endif

#ifndef KSSTATUS
#define KSSTATUS
#define KS_OK                           (0U)
#define KS_ERROR                        (1U)
#define KS_BUSY                         (2U)
#define KS_TIMEOUT                      (3U)
#endif

#ifndef KSUNIT
#define KSUNIT
#define KS_U8                           (0x0)  /* 4'b 0000 */
#define KS_U16                          (0x1)  /* 4'b 0001 */
#define KS_U32                          (0x2)  /* 4'b 0010 */
#define KS_U64                          (0x3)  /* 4'b 0011 */
#define KS_I8                           (0x4)  /* 4'b 0100 */
#define KS_I16                          (0x5)  /* 4'b 0101 */
#define KS_I32                          (0x6)  /* 4'b 0110 */
#define KS_I64                          (0x7)  /* 4'b 0111 */
#define KS_F16                          (0x9)  /* 4'b 1001 */
#define KS_F32                          (0xA)  /* 4'b 1010 */
#define KS_F64                          (0xB)  /* 4'b 1011 */
#define KS_R0                           (0x8)  /* 4'b 1000 */
#define KS_R1                           (0xC)  /* 4'b 1100 */
#define KS_R2                           (0xD)  /* 4'b 1101 */
#define KS_R3                           (0xE)  /* 4'b 1110 */
#define KS_R4                           (0xF)  /* 4'b 1111 */
#endif

/* Macro -----------------------------------------------------------------------------------*/
/* Typedef ---------------------------------------------------------------------------------*/
/* Extern ----------------------------------------------------------------------------------*/
/* Functions -------------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif

/*************************************** END OF FILE ****************************************/
