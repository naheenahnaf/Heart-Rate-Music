#ifndef LAB3_SD_H
#define	LAB3_SD_H

#include "configuration.h"



/* -------------------------- Function Prototypes --------------------------- */
void SD_Init(void);
void SD_SendCommand(u8 u8CMD6bit_, u8 u8Arg3_, u8 u8Arg2_, u8 u8Arg1_, u8 ARG0);
void SD_Read8bitResponse(void);
bool SD_Check8bitResponse(u8 Byte);
void SD_Read40bitResponse(void);
bool SD_Check40bitResponse(u8 Byte4, u8 Byte3, u8 Byte2, u8 Byte1, u8 Byte0);
bool SD_WriteBlock(u8 ADDR3, u8 ADDR2, u8 ADDR1, u8 ADDR0);
bool SD_ReadBlock(u8 ADDR3, u8 ADDR2, u8 ADDR1, u8 ADDR0);
    

/* ------------------ #define based Function Declarations ------------------- */

//REQUIRES: SPI interface initialized using SPI_Init.
//PROMISES: Sets the Chip Select line for the SD Card high.       
#define SD_SET_CS_HIGH()  (LATCbits.LATC7 = 1)

//REQUIRES: SPI interface initialized using SPI_Init.
//PROMISES: Sets the Chip Select line for the SD Card low.         
#define SD_SET_CS_LOW()   (LATCbits.LATC7 = 0)

/* -------------------------------------------------------------------------- */

#endif	/* LAB3_SD_H */

