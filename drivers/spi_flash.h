#ifndef __SPI_FLASH_H__
#define __SPI_FLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "board.h"

/**
 * @brief  SPI Flash supported commands
 */
#define sFLASH_CMD_WRITE          0x02 /*!< Write to Memory instruction */
#define sFLASH_CMD_WRSR           0x01 /*!< Write Status Register instruction */
#define sFLASH_CMD_WREN           0x06 /*!< Write enable instruction */
#define sFLASH_CMD_READ           0x03 /*!< Read from Memory instruction */
#define sFLASH_CMD_RDSR           0x05 /*!< Read Status Register instruction  */
#define sFLASH_CMD_RDID           0x9F /*!< Read identification */
#define sFLASH_CMD_SE             0x20 /*!< Sector Erase instruction */
#define sFLASH_CMD_BE             0x52 /*!< Block Erase instruction (32k)*/
#define sFLASH_CMD_CE             0xC7 /*!< Chip Erase instruction */
// #define sFLASH_CMD_BE    0xD8 /*!< Block Erase instruction (64k)*/

#define sFLASH_WIP_FLAG           0x01 /*!< Write In Progress (WIP) flag */

#define sFLASH_DUMMY_BYTE         0xA5
#define sFLASH_SPI_PAGESIZE       0x100 /*!< 256 bytes */

#define sFLASH_GD25Q128_ID        0xC84018

#define sFLASH_SPI                SPI1
#define sFLASH_SPI_CLK            RCC_APB2_PERIPH_SPI1
#define sFLASH_SPI_SCK_PIN        GPIO_PIN_5 /* PA.05 */
#define sFLASH_SPI_SCK_GPIO_PORT  GPIOA      /* GPIOA */
#define sFLASH_SPI_SCK_GPIO_CLK   RCC_APB2_PERIPH_GPIOA
#define sFLASH_SPI_MISO_PIN       GPIO_PIN_6 /* PA.06 */
#define sFLASH_SPI_MISO_GPIO_PORT GPIOA      /* GPIOA */
#define sFLASH_SPI_MISO_GPIO_CLK  RCC_APB2_PERIPH_GPIOA
#define sFLASH_SPI_MOSI_PIN       GPIO_PIN_7 /* PA.07 */
#define sFLASH_SPI_MOSI_GPIO_PORT GPIOA      /* GPIOA */
#define sFLASH_SPI_MOSI_GPIO_CLK  RCC_APB2_PERIPH_GPIOA
#define sFLASH_CS_PIN             GPIO_PIN_4 /* PA.04 */
#define sFLASH_CS_GPIO_PORT       GPIOA      /* GPIOA */
#define sFLASH_CS_GPIO_CLK        RCC_APB2_PERIPH_GPIOA
#define sFLASH_PWR_PIN            GPIO_PIN_9 /* PC.09 */
#define sFLASH_PWR_GPIO_PORT      GPIOC      /* GPIOC */
#define sFLASH_PWR_GPIO_CLK       RCC_APB2_PERIPH_GPIOC

/**
 * @brief  Select sFLASH: Chip Select pin low
 */
#define sFLASH_CS_LOW()           GPIO_ResetBits(sFLASH_CS_GPIO_PORT, sFLASH_CS_PIN)
/**
 * @brief  Deselect sFLASH: Chip Select pin high
 */
#define sFLASH_CS_HIGH()          GPIO_SetBits(sFLASH_CS_GPIO_PORT, sFLASH_CS_PIN)

/**
 * @brief  High layer functions
 */
void     sFLASH_DeInit(void);
void     sFLASH_Init(void);
void     sFLASH_EraseSector(uint32_t SectorAddr);
void     sFLASH_EraseBulk(void);
void     sFLASH_WritePage(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void     sFLASH_WriteBuffer(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void     sFLASH_ReadBuffer(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
uint32_t sFLASH_ReadID(void);
void     sFLASH_StartReadSequence(uint32_t ReadAddr);

/**
 * @brief  Low layer functions
 */
uint8_t  sFLASH_ReadByte(void);
uint8_t  sFLASH_SendByte(uint8_t byte);
uint16_t sFLASH_SendHalfWord(uint16_t HalfWord);
void     sFLASH_WriteEnable(void);
void     sFLASH_WaitForWriteEnd(void);

#ifdef __cplusplus
}
#endif

#endif // __SPI_FLASH_H__