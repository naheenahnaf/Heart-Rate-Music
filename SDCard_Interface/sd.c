/*!**********************************************************************************************************************
@file sd.c                                                                
@brief Basic SD card driver for the ENCM 369 firmware. 

 Original author: Paul Kathol
 Modified by: Ahnaf Naheen
***********************************************************************************************************************/

#include "configuration.h"

/***********************************************************************************************************************
Global variable definitions with scope across entire project.
All Global variable names shall start with "G_<type>SD"
***********************************************************************************************************************/
/* New variables */
volatile u8 G_u8SDResp8 = 0xFF;
volatile u8 G_u8SDCurrentRxBuffer = 0;

u8 G_au8SDResp40[5] = {0xFF,0xFF,0xFF,0xFF,0xFF};
u8 G_au8SDWriteBuffer[512];
u8 G_au8SDReadBuffer0[512];
u8 G_au8SDReadBuffer1[512];

/*--------------------------------------------------------------------------------------------------------------------*/
/* Existing variables (defined in other files -- should all contain the "extern" keyword) */
extern volatile u32 G_u32SystemTime1ms;                   /*!< @brief From main.c */
extern volatile u32 G_u32SystemTime1s;                    /*!< @brief From main.c */
extern volatile u32 G_u32SystemFlags;                     /*!< @brief From main.c */



/***********************************************************************************************************************
Global variable definitions with scope limited to this local application.
Variable names shall start with "SD_" and be declared as static.
***********************************************************************************************************************/


//REQUIRES: SPI interface initialized using SPI_Init.
//PROMISES: Performs the SD Card initialization process over SPI:
//          1. Sets CS high and writes 0xFF for 80 cycles.
//          2. Send CMD0 with argument 0x00000000 until response is 0x01
//          3. Send CMD8 with argument 0x000001AA until response is 0x01000001AA
//          4a.Send CMD55 with argument 0x00000000 until response ix 0x00
//          4b.Send Send CMD41 with argument 0x40000000, if response isn't 0x00
//             go back to step 4a.
void SD_Init(void)
{
    //Step 1:
    //Set Chip Select high, and write 0xFF for at least 74 cycles.
    SD_SET_CS_HIGH();
    
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    
    SD_SET_CS_LOW();

    //Step 2:
    //Send CMD0 with argument 0x00000000.
    //Expect 8-bit response 0x01.
    //On any other response, retry CMD0.
    do {
        SD_SendCommand(0, 0x00, 0x00, 0x00, 0x00);
        SD_Read8bitResponse();
        asm("NOP");
    } while (SD_Check8bitResponse(0x01) == false);
    
    //Step 3:
    //Send CMD8 with argument 0x000001AA.
    //Expect 40-bit response 0x01000001AA.
    //On any other response, retry CMD8.
    do {
        SD_SendCommand(8, 0x00, 0x00, 0x01, 0xAA);
        SD_Read40bitResponse();
        asm("NOP");
    } while (SD_Check40bitResponse(0x01, 0x00, 0x00, 0x01, 0xAA) == false);

    //Step 4a
    //Send CMD55 with argument 0x00000000.
    //Expect 8-bit response 0x01.
    //On any other response, retry CMD55.
    do 
    {
      do 
      {
          SD_SendCommand(55, 0x00, 0x00, 0x00, 0x00);
          SD_Read8bitResponse();
          asm("NOP");
      } while (SD_Check8bitResponse(0x01) == false);

      //Step 4b:
      //If the CMD55 response is good,
      //Send CMD41 with argument 0x40000000
      //Expect 8-bit response 0x00
      //On any other response, go back to CMD55.
      SD_SendCommand(41, 0x40, 0x00, 0x00, 0x00);
      SD_Read8bitResponse();
      asm("NOP");
    } while (SD_Check8bitResponse(0x00) == false);
}

//REQUIRES: SPI interface initialized using SPI_Init.
//          Argument u8CMD6bit is the command code from 0 to 64.
//          Arguments u8Arg3_, u8Arg2_, u8Arg1_ and u8Arg0_ are the four bytes of the
//          specific command's argument.
//PROMISES: Sends a 48-bit command to the SD card over SPI.
//          Calculates and sends the 7-bit checksum for the command.
void SD_SendCommand(u8 u8CMD6bit_, u8 u8Arg3_, u8 u8Arg2_, u8 u8Arg1_, u8 u8Arg0_)
{
    u8 u8ByteToSend = 0xFF;
    u8 u8Checksum = 0x00;
    
    //First byte is 01XXXXXX, where X are the 6 command bits.
    u8ByteToSend = (u8CMD6bit_ & 0x3F) | 0x40;
    SPI_Write(u8ByteToSend);
    UPDATE_CRC(u8Checksum, u8ByteToSend);
     
    //Next 4 bytes are the 32 argument bits.
    u8ByteToSend = u8Arg3_;
    SPI_Write(u8ByteToSend);
    UPDATE_CRC(u8Checksum, u8ByteToSend);
    
    u8ByteToSend = u8Arg2_;
    SPI_Write(u8ByteToSend);
    UPDATE_CRC(u8Checksum, u8ByteToSend);
    
    u8ByteToSend = u8Arg1_;
    SPI_Write(u8ByteToSend);
    UPDATE_CRC(u8Checksum, u8ByteToSend);
    
    u8ByteToSend = u8Arg0_;
    SPI_Write(u8ByteToSend);
    UPDATE_CRC(u8Checksum, u8ByteToSend);
    
    //Last byte is XXXXXXX1, where X are the 7 Cyclic Redundancy Check bits.
    u8ByteToSend = (u8)(u8Checksum << 1) | 0x01;
    SPI_Write(u8ByteToSend);
}

//REQUIRES: SPI interface initialized using SPI_Init.
//PROMISES: For the SD card, 0xFF is 'no data'. Sends 0xFF to the device 
//          repeatedly until a response other than 0xFF is received. Then stores
//          the one-byte response in the global variable G_u8SDResp8.
void SD_Read8bitResponse(void)
{
    u8 u8ReadMessage = 0xFF;
    
    //We don't know when the response will come, but the first bit is always
    //a zero. Read bytes until the response contains at least one zero.
    do 
    {
      u8ReadMessage = SPI_Read();
    } 
    while (u8ReadMessage == 0xFF);   
    
    //Read the one message bytes into the global variable
    G_u8SDResp8 = u8ReadMessage;
    
    //Read once more and ignore the result.
    SPI_Read();
        
    return;
}

//REQUIRES: Nothing.
//PROMISES: Compares the input argument u8Byte_ to the global variable G_u8SDResp8
//          Returns true (1) if they are equal, and false (0) otherwise.
bool SD_Check8bitResponse(u8 u8Byte_)
{
    bool bMatch = true;
    
    if (u8Byte_ != G_u8SDResp8) 
    {
      bMatch = false;
    }
    
    return bMatch;
}
//REQUIRES: SPI interface initialized using SPI_Init.
//PROMISES: For the SD card, 0xFF is 'no data'. Sends 0xFF to the device 
//          repeatedly until a response other than 0xFF is recieved. Then stores
//          the five-byte response in the global array G_au8SDResp40.
void SD_Read40bitResponse(void)
{
    u8 u8ReadMessage = 0xFF;
    
    //We don't know when the response will come, but the first bit is always
    //a zero. Read bytes until the response contains at least one zero.
    do 
    {
      u8ReadMessage = SPI_Read();
    } 
    while (u8ReadMessage == 0xFF);   
    
    //Read the five message bytes into the global array
    G_au8SDResp40[0] = u8ReadMessage;
    G_au8SDResp40[1] = SPI_Read();
    G_au8SDResp40[2] = SPI_Read();
    G_au8SDResp40[3] = SPI_Read();
    G_au8SDResp40[4] = SPI_Read();
    
    //Read once more and ignore the result.
    SPI_Read();
    
    return;
}

//REQUIRES: Nothing.
//PROMISES: Compares the input arguments u8Byte4_ through u8Byte0_ to the global 
//          variable G_u8SDResp8. Returns true if all comparisons are equal,
//          and false (0) otherwise.
bool SD_Check40bitResponse(u8 u8Byte4_, u8 u8Byte3_, u8 u8Byte2_, u8 u8Byte1_, u8 u8Byte0_)
{
    bool bMatch = true;
    if (u8Byte4_ != G_au8SDResp40[0]) bMatch = false;
    if (u8Byte3_ != G_au8SDResp40[1]) bMatch = false;
    if (u8Byte2_ != G_au8SDResp40[2]) bMatch = false;
    if (u8Byte1_ != G_au8SDResp40[3]) bMatch = false;
    if (u8Byte0_ != G_au8SDResp40[4]) bMatch = false;  
    
    return bMatch;
}

//REQUIRES: SPI interface initialized using SPI_Init.
//          SD Card initialized using SD_Init.
//          Global 512-byte array G_au8SDWriteBuffer contains the data to write. 
//PROMISES: Writes the 512 bytes stored in G_au8SDWriteBuffer to the SD card
//          at the address given by u8Addr3-0_.
//          Returns true if the write was successful, false otherwise.
bool SD_WriteBlock(u8 u8Addr3_, u8 u8Addr2_, u8 u8Addr1_, u8 u8Addr0_)
{
    //Send the block write command to the SD card
    SD_SendCommand(24, u8Addr3_, u8Addr2_, u8Addr1_, u8Addr0_);
    
    //If the response is anything but 0x00, we cannot write.
    SD_Read8bitResponse();
    if(SD_Check8bitResponse(0x00) == false) 
    {
      return false;
    }
    
    //Write a few empty cycles to give the SD card time.
    SPI_Write(0xFF);
    SPI_Write(0xFF);
    
    //Write the Data Token to signify the start of the packet
    SPI_Write(0xFE);
    
    //Write the contents of the block write buffer.
    for(u16 i = 0; i < 512; i++)
    {
        SPI_Write(G_au8SDWriteBuffer[i]);
    }
    
    //Read the Data Response byte
    SD_Read8bitResponse();
    
    //Check the data response byte. We expect 0xE5 on a successful write.
    if(SD_Check8bitResponse(0xE5) == false) 
    {
      return false;
    }
    
    // Write was a success, so return true.
    return true;
}

//REQUIRES: SPI interface initialized using SPI_Init.
//          SD Card initialized using SD_Init.
//          Global 512-byte array G_au8SDWriteBuffer contains the data to write. 
//          G_u8SDCurrentRxBuffer holds the index of the current buffer (so the other buffer will be updated with this read)
//PROMISES: Reads the 512 bytes stored in the SD card block given by u8Addr3_-0
//          and saves them to the global array G_au8SDReadBuffer0 or G_au8SDReadBuffer1.
//          Returns true if the read was successful, false otherwise.
//          Does NOT verify the checksum of the read data.
bool SD_ReadBlock(u8 u8Addr3_, u8 u8Addr2_, u8 u8Addr1_, u8 u8Addr0_)
{
    u8 u8ReadMessage = 0xFF;
    u8* pu8RxBuffer;
    
    //Send the block read command (CMD17) to the SD card.
    //The 32 bit argument is which 512-byte sector to read.
    SD_SendCommand(17, u8Addr3_, u8Addr2_, u8Addr1_, u8Addr0_);
    
    //Wait for the SD card to respond to the command.
    SD_Read8bitResponse();
    
    //If the response is anything but 0x00, we cannot read.
    if(SD_Check8bitResponse(0x00) == false) return false;
    
    //We don't know when the SD card will start sending data, but the first byte
    //is always 0xFE. Read bytes until the response contains at least one zero.
    do 
    {
      u8ReadMessage = SPI_Read();
    } 
    while (u8ReadMessage == 0xFF);   

    //If the message is anything but 0xFE, we cannot read.
    if (u8ReadMessage != 0xFE) 
    {
      return false;
    }
    
    /* Set the destination to the correct buffer */
    if(G_u8SDCurrentRxBuffer == 0)
    {
      pu8RxBuffer = &G_au8SDReadBuffer0[0];
      G_u8SDCurrentRxBuffer = 1;
    }
    else
    {
      pu8RxBuffer = &G_au8SDReadBuffer1[0];
      G_u8SDCurrentRxBuffer = 0;
    }
    
    // Read all 512 bytes into the current read buffer.
    for(u16 i = 0; i < 512; i++)
    {
     *(pu8RxBuffer + i) = SPI_Read();
    }
    
    //The next two bytes are the block's 16-bit CRC. We won't worry about it but it needs to be read.
    SPI_Read();
    SPI_Read();
        
    // Final read to close the SD card read session.
    SPI_Read();
    
    //Read was a success, so return true.
    return true;
}
    
