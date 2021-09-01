/*!**********************************************************************************************************************
@file spi.c                                                                
@brief Basic SPI driver for the ENCM 369 firmware. 

 Original author: Paul Kathol, February 2019
 Modified by: Ahnaf Naheen, July 2021
***********************************************************************************************************************/

#include "configuration.h"


/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_<type>Spi"
***********************************************************************************************************************/
/* New variables */


/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemTime1ms;                   /*!< @brief From main.c */
extern volatile u32 G_u32SystemTime1s;                    /*!< @brief From main.c */
extern volatile u32 G_u32SystemFlags;                     /*!< @brief From main.c */



/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "Spi_" and be declared as static.
***********************************************************************************************************************/


// REQUIRES: Nothing.
// PROMISES: Configure SPI peripheral for SD card communication
void SPI_Init(void)
{
  SPI1CON0bits.EN = 0;

   /* SPI1CON0: disabled, MSb first, host, BMODE 1 */
  SPI1CON0 = 0x03;  // b'00000011'
  
  /* SPI1CON1: End sample, data change on Idle to Active, SCK idle high,
   no fast start, SS active-low, SDI & SDO active high */
  SPI1CON1 = 0xA4;  //b'10100100'
  
  /* Continuous CS, no FIFOs */
  SPI1CON2 = 0x07;  // b'00000111'
  
  /* SPI clock is FOsc / 4 */
  SPI1CLK = 0x00;
  SPI1BAUD = 3;
      
  /* Enable SPI */
  SPI1CON0bits.EN = 1;
}

// REQUIRES: SPI interface initialized using SPI_Init.
//           Argument u8DataByte_ is one byte of data to send over SPI.
// PROMISES: Transmits u8DataByte_ on the MOSI line.
void SPI_Write(u8 u8DataByte_)
{
  /* Queue the byte and wait until sent */
  SPI1TXB = u8DataByte_;
  __nop();
  __nop();
  while(SPI1CON2bits.BUSY == 1);
  
  /* Read the received dummy byte to clear the buffer */
  u8DataByte_ = SPI1RXB;
}

// REQUIRES: SPI interface initialized using SPI_Init.
// PROMISES: Transmits the byte 0xFF on the MOSI line.
//           Returns one byte of data recieved on the MISO line.
u8 SPI_Read(void)
{
  /* Initiate transfer with dummy write then wait for Rx byte. */
  SPI1TXB = 0xFF; 
  __nop();
  __nop();
   while(SPI1CON2bits.BUSY == 1);
  
  return SPI1RXB;
}
