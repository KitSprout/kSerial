/**
  *      __            ____
  *     / /__ _  __   / __/                      __  
  *    / //_/(_)/ /_ / /  ___   ____ ___  __ __ / /_ 
  *   / ,<  / // __/_\ \ / _ \ / __// _ \/ // // __/ 
  *  /_/|_|/_/ \__//___// .__//_/   \___/\_,_/ \__/  
  *                    /_/   github.com/KitSprout    
  * 
  * @file    kSerial.c
  * @author  KitSprout
  * @date    02-Jul-2017
  * @brief   kSerial packet format :
  *          byte 1   : header 'K' (75)   [HK]
  *          byte 2   : header 'S' (83)   [HS]
  *          byte 3   : total length      [L ]
  *          byte 4   : data type         [T ]
  *          byte 5   : parameter 1       [P1]
  *          byte 6   : parameter 2       [P2]
  *           ...
  *          byte L-2 : data              [DN]
  *          byte L-1 : finish '\r' (13)  [ER]
  *          byte L   : finish '\n' (10)  [EN]
  */

/* Includes --------------------------------------------------------------------------------*/
#include "drivers\stm32f4_system.h"
#include "modules\kSerial.h"

/** @addtogroup STM32_Application
  * @{
  */

/* Private typedef -------------------------------------------------------------------------*/
/* Private define --------------------------------------------------------------------------*/
#define KS_MAX_SEND_BUFF_SIZE   256
#define KS_MAX_RECV_BUFF_SIZE   256

/* Private macro ---------------------------------------------------------------------------*/
/* Private variables -----------------------------------------------------------------------*/
static USART_TypeDef *kSerialUart = NULL;
static uint8_t ksSendBuffer[KS_MAX_SEND_BUFF_SIZE] = {0};
static uint8_t ksRecvBuffer[KS_MAX_RECV_BUFF_SIZE] = {0};

/* Private function prototypes -------------------------------------------------------------*/
/* Private functions -----------------------------------------------------------------------*/

/**
  * @brief  kSerial_Send
  */
static void kSerial_Send( uint8_t *sendData, uint16_t lens )
{
  do {
    kSerialUart->DR = (*sendData++ & (uint16_t)0x01FF);
    while (!(kSerialUart->SR & UART_FLAG_TXE));
  } while (--lens);
}

/**
  * @brief  kSerial_Recv
  */
static uint8_t kSerial_Recv( void )
{
  while (!(kSerialUart->SR & UART_FLAG_RXNE));
  return ((uint16_t)(kSerialUart->DR & (uint16_t)0x01FF));
}

/**
  * @brief  kSerial_Config
  */
void kSerial_Config( USART_TypeDef *USARTx )
{
  kSerialUart = USARTx;

  ksSendBuffer[0] = 'K';  /* header 'K'   */
  ksSendBuffer[1] = 'S';  /* header 'S'   */
  ksSendBuffer[2] = 8;    /* total length */
  ksSendBuffer[3] = 0;    /* data type    */
  ksSendBuffer[4] = 0;    /* parameter 1  */
  ksSendBuffer[5] = 0;    /* parameter 2  */
                          /* data ....... */
  ksSendBuffer[6] = '\r'; /* finish '\r'  */
  ksSendBuffer[7] = '\n'; /* finish '\n'  */
}

/**
  * @brief  kSerial_Pack
  */
int8_t kSerial_Pack( uint8_t *packet, void *param, void *data, const uint8_t lens, const uint8_t type )
{
  const uint8_t packetDataLens = lens * (type & (uint8_t)0x0F);

  packet[0] = 'K';                        /* header 'K'   */
  packet[1] = 'S';                        /* header 'S'   */
  packet[2] = 8 + packetDataLens;         /* total length */
  packet[3] = type;                       /* data type    */
  if (param != NULL) {
    packet[4] = ((uint8_t*)param)[0];     /* parameter 1  */
    packet[5] = ((uint8_t*)param)[1];     /* parameter 2  */
  }
  else {
    packet[4] = 0;
    packet[5] = 0;
  }

  for (uint8_t i = 0; i < packetDataLens; i++) {
    packet[6 + i] = ((uint8_t*)data)[i];  /* data ....... */
  }

  packet[6 + packetDataLens] = '\r';      /* finish '\r'  */
  packet[7 + packetDataLens] = '\n';      /* finish '\n'  */

  return HAL_OK;
}

/**
  * @brief  kSerial_Unpack
  */
int8_t kSerial_Unpack( uint8_t *packet, void *param, void *data, uint8_t *lens, uint8_t *type )
{
  if ((packet[0] == 'K') && (packet[1] == 'S')) {
    *lens = packet[2];
    if (*lens < 8) {
      return HAL_ERROR;
    }
    if ((packet[*lens - 2] == '\r') && (packet[*lens - 1] == '\n')) {
      *type  = packet[3];
      ((uint8_t*)param)[0] = packet[4];
      ((uint8_t*)param)[1] = packet[5];
      for (uint8_t i = 0; i < *lens; i++ ) {
        ((uint8_t*)data)[i] = packet[6 + i];
      }
      return HAL_OK;
    }
  }

  return HAL_ERROR;
}

/**
  * @brief  kSerial_SendPacket
  */
int8_t kSerial_SendPacket( void *param, void *data, const uint8_t lens, const uint8_t type )
{
  int8_t state;

  state = kSerial_Pack(ksSendBuffer, param, data, lens, type);
  if (state != HAL_OK) {
    return HAL_ERROR;
  }
  kSerial_Send(ksSendBuffer, ksSendBuffer[2]);

  return HAL_OK;
}

/**
  * @brief  kSerial_RecvPacket
  */
int8_t kSerial_RecvPacket( void *param, void *data, uint8_t *lens, uint8_t *type )
{
  static uint8_t point = 0;
  static uint8_t index = 0;
  static uint8_t bytes = 0;

  int8_t state;

  ksRecvBuffer[point] = kSerial_Recv();

  if (point > 1) {
    if ((ksRecvBuffer[point - 2] == 'K') && (ksRecvBuffer[point - 1] == 'S')) {
      index = point - 2;
      bytes = ksRecvBuffer[point];
    }
    if ((point - index + 1) == bytes) {
      state = kSerial_Unpack(&(ksRecvBuffer[index]), param, data, lens, type);
      if (state == HAL_OK) {
        point = 0;
        index = 0;
        bytes = 0;
        return HAL_OK;
      }
    }
  }
  point++;

  return HAL_ERROR;
}

/**
  * @brief  kSerial_GetPacketDataLens
  */
uint8_t kSerial_GetPacketDataLens( uint8_t lens, uint8_t type )
{
  switch (type & 0x0F) {
    case 0x01:  break;
    case 0x02:  lens >>= 1; break;
    case 0x04:  lens >>= 2; break;
    case 0x08:  lens >>= 3; break;
    default:    return 0;
  }
  return lens;
}

/*************************************** END OF FILE ****************************************/
