/*
 * Parse file in hex format into internal representation.
 * Author: Vlastimil Kosar <kosar@rehivetrch.com>
 * License: TBD
 */
#include <vector>
#include <string>
#include <fstream>
#include <array>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <iomanip>

#include <string.h>

#include "string_operations.h"
#include "HexFmtParser.h"
#include "TrFmtException.h"

#include "Trace.h"

const size_t MIN_LINE_LEN = 11;
const size_t MAX_LINE_LEN = 521;

/************************************/
/* Private constants                */
/************************************/
#define	IQRF_PGM_SUCCESS              200
#define IQRF_PGM_FLASH_BLOCK_READY    220
#define IQRF_PGM_EEPROM_BLOCK_READY   221
#define	IQRF_PGM_ERROR                222

#define IQRF_PGM_FILE_DATA_READY      0
#define IQRF_PGM_FILE_SHORT_LINE      1
#define IQRF_PGM_FILE_LONG_LINE       2
#define IQRF_PGM_FILE_EVEN_LENGTH     3
#define IQRF_PGM_FILE_INVALID_CHAR    4
#define IQRF_PGM_FILE_MISSING_START   5
#define IQRF_PGM_FILE_INVALID_SUM     6
#define IQRF_PGM_END_OF_FILE          7

#define IQRF_SIZE_OF_FLASH_BLOCK      64
#define IQRF_LICENCED_MEMORY_BLOCKS   96
#define IQRF_MAIN_MEMORY_BLOCKS       48

#define IQRF_CFG_MEMORY_BLOCK         (IQRF_LICENCED_MEMORY_BLOCKS - 2)

#define SERIAL_EEPROM_MIN_ADR         0x0200
#define SERIAL_EEPROM_MAX_ADR         0x09FF
#define IQRF_LICENCED_MEM_MIN_ADR     0x2C00
#define IQRF_LICENCED_MEM_MAX_ADR     0x37BF
#define IQRF_CONFIG_MEM_L_ADR         0x37C0
#define IQRF_CONFIG_MEM_H_ADR         0x37D0
#define IQRF_MAIN_MEM_MIN_ADR         0x3A00
#define IQRF_MAIN_MEM_MAX_ADR         0x3FFF
#define PIC16LF1938_EEPROM_MIN        0xF000
#define PIC16LF1938_EEPROM_MAX        0xF0BF

#define SIZE_OF_CODE_LINE_BUFFER      256

/************************************/
/* Private functions predeclaration */
/************************************/
bool verify_record_csum(const std::string& str);
uint8_t iqrfPgmReadHEXFileLine(std::ifstream &infile, uint16_t *LineNumber);
void iqrfPgmMoveOverflowedData(void);
uint8_t iqrfPgmPrepareMemBlock(std::ifstream &infile, uint16_t *LineNumber);

/************************************/
/* Private variables                */
/************************************/
typedef struct {
    uint32_t  HiAddress;
    uint16_t  Address;
    uint16_t  MemoryBlockNumber;
    uint8_t MemoryBlockProcessState;
    uint8_t DataInBufferReady;
    uint8_t DataOverflow;
    uint8_t MemoryBlock[68];
} PREPARE_MEM_BLOCK;

PREPARE_MEM_BLOCK PrepareMemBlock;

uint8_t IqrfPgmCodeLineBuffer[SIZE_OF_CODE_LINE_BUFFER];

bool verify_record_csum(const std::string& str)
{
    size_t len = str.length() - 1;
    unsigned int sum = 0;

    std::string data = str.substr(1, len);
    for (unsigned int i = 0; i < len / 2; i++) {
        sum += std::stoul(data.substr(i * 2, 2), nullptr, 16);
    }

    return (sum & 0xff) == 0;
}

uint8_t iqrfPgmReadHEXFileLine(std::ifstream &infile, uint16_t *LineNumber)
{
    std::string line;
    size_t i;
    size_t len;

    if (std::getline(infile, line)) {
        *LineNumber++;

        // Trim whitespace
        line = trim(line);

        len = line.length();

        // Every line in hex file must be at least 11 characters long
        if (len < MIN_LINE_LEN) {
            return IQRF_PGM_FILE_SHORT_LINE;
        }

        // Every line in hex file must be at most 521 characters long
        if (len > MAX_LINE_LEN) {
            return IQRF_PGM_FILE_LONG_LINE;
        }

        // Every line must be odd
        if (len % 2 != 1) {
            return IQRF_PGM_FILE_EVEN_LENGTH;
        }

        // Check for invalid characters
        if ((line.find_first_not_of(":0123456789abcdefABCDEF")) != std::string::npos) {
            return IQRF_PGM_FILE_INVALID_CHAR;
        }

        // Check for record start code
        if (line[0] != ':') {
            return IQRF_PGM_FILE_MISSING_START;
        }

        // Check checksum
        if (!verify_record_csum(line)) {
            return IQRF_PGM_FILE_INVALID_SUM;
        }

        for (i = 0; i < (len-1) / 2; i++) {
            IqrfPgmCodeLineBuffer[i] = std::stoul(line.substr(1 + i * 2, 2), nullptr, 16);
        }

        return IQRF_PGM_FILE_DATA_READY;

    } else {
        return IQRF_PGM_END_OF_FILE;
    }

}

/**
 * Move overflowed data to active block ready to programming
 */
void iqrfPgmMoveOverflowedData(void)
{
    uint16_t MemBlock;
    // move overflowed data to active block
    memcpy((uint8_t *)&PrepareMemBlock.MemoryBlock[34], (uint8_t *)&PrepareMemBlock.MemoryBlock[0], 34);
    // clear block of memory for overfloved data
    memset((uint8_t *)&PrepareMemBlock.MemoryBlock[0], 0, 34);
    // calculate the data block index
    MemBlock = ((uint16_t)PrepareMemBlock.MemoryBlock[35] << 8) | PrepareMemBlock.MemoryBlock[34];
    PrepareMemBlock.MemoryBlockNumber = MemBlock + 0x10;
    MemBlock++;
    PrepareMemBlock.MemoryBlock[0] = MemBlock & 0x00FF;         // write next block index to image
    PrepareMemBlock.MemoryBlock[1] = MemBlock >> 8;
    PrepareMemBlock.DataOverflow = 0;
    // initialize block process counter (block will be written to TR module in 1 write packet)
    PrepareMemBlock.MemoryBlockProcessState = 1;
}

/**
 * Reading and preparing a block of data to be programmed into the TR module
 * @param none
 * @return result of data preparing operation
 */
uint8_t iqrfPgmPrepareMemBlock(std::ifstream &infile, uint16_t *LineNumber)
{
    uint16_t MemBlock;
    uint8_t DataCounter;
    uint8_t DestinationIndex;
    uint8_t ValidAddress;
    uint8_t OperationResult;
    uint8_t Cnt;

    // initialize memory block for flash programming
    if (!PrepareMemBlock.DataOverflow) {
        for (Cnt=0; Cnt<sizeof(PrepareMemBlock.MemoryBlock); Cnt+=2) {
            PrepareMemBlock.MemoryBlock[Cnt] = 0xFF;
            PrepareMemBlock.MemoryBlock[Cnt+1] = 0x3F;
        }
    }
    PrepareMemBlock.MemoryBlockNumber = 0;

    while(1) {
        // if no data ready in file buffer
        if (!PrepareMemBlock.DataInBufferReady) {
            OperationResult = iqrfPgmReadHEXFileLine(infile, LineNumber);       // read one line from HEX file
            // check result of file reading operation
            if (OperationResult != IQRF_PGM_FILE_DATA_READY && OperationResult != IQRF_PGM_END_OF_FILE) {
                return(OperationResult);
            } else {
                if (OperationResult == IQRF_PGM_END_OF_FILE) {
                    // if any data are ready to programm to FLASH
                    if (PrepareMemBlock.MemoryBlockNumber) {
                        return(IQRF_PGM_FLASH_BLOCK_READY);
                    } else {
                        if (PrepareMemBlock.DataOverflow) {
                            iqrfPgmMoveOverflowedData();
                            return(IQRF_PGM_FLASH_BLOCK_READY);
                        } else {
                            return(IQRF_PGM_SUCCESS);
                        }
                    }
                }
            }
            PrepareMemBlock.DataInBufferReady = 1;            // set flag, data ready in file buffer
        }

        if (IqrfPgmCodeLineBuffer[3] == 0) {                // data block ready in file buffer
            // read destination address for data in buffer
            PrepareMemBlock.Address = (PrepareMemBlock.HiAddress + ((uint16_t)IqrfPgmCodeLineBuffer[1] << 8) + IqrfPgmCodeLineBuffer[2]) / 2;
            if (PrepareMemBlock.DataOverflow)
                iqrfPgmMoveOverflowedData();
            // data for external serial EEPROM
            if (PrepareMemBlock.Address >= SERIAL_EEPROM_MIN_ADR && PrepareMemBlock.Address <= SERIAL_EEPROM_MAX_ADR) {
                // if image of data block is not initialized
                if (PrepareMemBlock.MemoryBlockNumber == 0) {
                    MemBlock = (PrepareMemBlock.Address - 0x200) / 32;          // calculate data block index
                    memset((uint8_t *)&PrepareMemBlock.MemoryBlock[0], 0, 68);  // clear image of data block
                    PrepareMemBlock.MemoryBlock[34] = MemBlock & 0x00FF;        // write block index to image
                    PrepareMemBlock.MemoryBlock[35] = MemBlock >> 8;
                    MemBlock++;                                                 // next block index
                    PrepareMemBlock.MemoryBlock[0] = MemBlock & 0x00FF;         // write next block index to image
                    PrepareMemBlock.MemoryBlock[1] = MemBlock >> 8;
                    PrepareMemBlock.MemoryBlockNumber = PrepareMemBlock.Address / 32;   // remember actual memory block
                    // initialize block process counter (block will be written to TR module in 1 write packet)
                    PrepareMemBlock.MemoryBlockProcessState = 1;
                }

                MemBlock = PrepareMemBlock.Address / 32;                        // calculate actual memory block
                // calculate offset from start of image, where data to be written
                DestinationIndex = (PrepareMemBlock.Address % 32) + 36;
                DataCounter = IqrfPgmCodeLineBuffer[0] / 2;                     // read number of data bytes in file buffer

                // if data in file buffer are from different memory block, write actual image to TR module
                if (PrepareMemBlock.MemoryBlockNumber != MemBlock)
                    return(IQRF_PGM_FLASH_BLOCK_READY);
                // check if all data are inside the image of data block
                if (DestinationIndex + DataCounter > sizeof(PrepareMemBlock.MemoryBlock))
                    PrepareMemBlock.DataOverflow = 1;
                // copy data from file buffer to image of data block
                for (Cnt=0; Cnt < DataCounter; Cnt++) {
                    PrepareMemBlock.MemoryBlock[DestinationIndex++] = IqrfPgmCodeLineBuffer[2*Cnt+4];
                    if (DestinationIndex == 68)
                        DestinationIndex = 2;
                }
                // if all data are not inside the image of data block
                if (PrepareMemBlock.DataOverflow) {
                    PrepareMemBlock.DataInBufferReady = 0;                      // process next line from HEX file
                    return(IQRF_PGM_FLASH_BLOCK_READY);
                }
            } else {  // check if data in file buffer are for other memory areas
                MemBlock = PrepareMemBlock.Address / 32;                        // calculate actual memory block
                // calculate offset from start of image, where data to be written
                DestinationIndex = (PrepareMemBlock.Address % 32) * 2;
                if (DestinationIndex < 32) DestinationIndex += 2;
                else DestinationIndex += 4;
                DataCounter = IqrfPgmCodeLineBuffer[0];                         // read number of data bytes in file buffer
                ValidAddress = 0;

                // check if data in file buffer are for main FLASH memory area in TR module
                if (PrepareMemBlock.Address >= IQRF_MAIN_MEM_MIN_ADR
                    && PrepareMemBlock.Address <= IQRF_MAIN_MEM_MAX_ADR)
                {
                    ValidAddress = 1;                                           // set flag, data are for FLASH memory area
                    // check if all data are in main memory area
                    if ((PrepareMemBlock.Address + DataCounter/2) > IQRF_MAIN_MEM_MAX_ADR)
                        DataCounter = (IQRF_MAIN_MEM_MAX_ADR - PrepareMemBlock.Address) * 2;
                    // check if all data are inside the image of data block
                    if (DestinationIndex + DataCounter > sizeof(PrepareMemBlock.MemoryBlock))
                        return(IQRF_PGM_ERROR);
                    // if data in file buffer are from different memory block, write actual image to TR module
                    if (PrepareMemBlock.MemoryBlockNumber) {
                        if (PrepareMemBlock.MemoryBlockNumber != MemBlock)
                            return(IQRF_PGM_FLASH_BLOCK_READY);
                    }
                } else {
                    // check if data in file buffer are for licenced FLASH memory area in TR module
                    if (PrepareMemBlock.Address >= IQRF_LICENCED_MEM_MIN_ADR
                        && PrepareMemBlock.Address <= IQRF_LICENCED_MEM_MAX_ADR)
                    {
                        ValidAddress = 1;                                       // set flag, data are for FLASH memory area
                        // check if all data are in licenced memory area
                        if ((PrepareMemBlock.Address + DataCounter/2) > IQRF_LICENCED_MEM_MAX_ADR)
                            DataCounter = (IQRF_LICENCED_MEM_MAX_ADR - PrepareMemBlock.Address) * 2;
                        // check if all data are inside the image of data block
                        if (DestinationIndex + DataCounter > sizeof(PrepareMemBlock.MemoryBlock))
                            return(IQRF_PGM_ERROR);
                        // if data in file buffer are from different memory block, write actual image to TR module
                        if (PrepareMemBlock.MemoryBlockNumber) {
                            if (PrepareMemBlock.MemoryBlockNumber != MemBlock)
                                return(IQRF_PGM_FLASH_BLOCK_READY);
                        }
                    } else {
                        // check if data in file buffer are for internal EEPROM of TR module
                        if (PrepareMemBlock.Address >= PIC16LF1938_EEPROM_MIN
                            && PrepareMemBlock.Address <= PIC16LF1938_EEPROM_MAX)
                        {
                            // if image of data block contains any data, write it to TR module
                            if (PrepareMemBlock.MemoryBlockNumber)
                                return(IQRF_PGM_FLASH_BLOCK_READY);
                            // prepare image of data block for internal EEPROM
                            PrepareMemBlock.MemoryBlock[0] = PrepareMemBlock.Address & 0x00FF;
                            PrepareMemBlock.MemoryBlock[1] = DataCounter / 2;
                            if (PrepareMemBlock.Address + PrepareMemBlock.MemoryBlock[1] > PIC16LF1938_EEPROM_MAX + 1
                                || PrepareMemBlock.MemoryBlock[1] > 32)
                            {
                                return(IQRF_PGM_ERROR);
                            }
                            for (uint8_t Cnt=0; Cnt < PrepareMemBlock.MemoryBlock[1]; Cnt++)
                                PrepareMemBlock.MemoryBlock[Cnt+2] = IqrfPgmCodeLineBuffer[2*Cnt+4];

                            PrepareMemBlock.DataInBufferReady = 0;
                            // initialize block process counter (block will be written to TR module in 1 write packet)
                            PrepareMemBlock.MemoryBlockProcessState = 1;
                            return(IQRF_PGM_EEPROM_BLOCK_READY);
                        }
                    }
                }
                // if destination address is from FLASH memory area
                if (ValidAddress) {
                    // remember actual memory block
                    PrepareMemBlock.MemoryBlockNumber = MemBlock;
                    // initialize block process counter (block will be written to TR module in 2 write packets)
                    PrepareMemBlock.MemoryBlockProcessState = 2;
                    // compute and write destination address of first half of image
                    MemBlock *= 32;
                    PrepareMemBlock.MemoryBlock[0] = MemBlock & 0x00FF;
                    PrepareMemBlock.MemoryBlock[1] = MemBlock >> 8;
                    // compute and write destination address of second half of image
                    MemBlock += 0x0010;
                    PrepareMemBlock.MemoryBlock[34] = MemBlock & 0x00FF;
                    PrepareMemBlock.MemoryBlock[35] = MemBlock >> 8;
                    // copy data from file buffer to image of data block
                    memcpy(&PrepareMemBlock.MemoryBlock[DestinationIndex], &IqrfPgmCodeLineBuffer[4], DataCounter);
                }
            }
        } else {
            if (IqrfPgmCodeLineBuffer[3] == 4)                                 // in file buffer is address info
                PrepareMemBlock.HiAddress = ((uint32_t)IqrfPgmCodeLineBuffer[4] << 24) + ((uint32_t)IqrfPgmCodeLineBuffer[5] << 16);
        }
        PrepareMemBlock.DataInBufferReady = 0;                                 // process next line from HEX file
    }
}


void HexFmtParser::parse() {
    std::ifstream infile(file_name);
    std::basic_string<uint8_t> prgData;

    uint16_t line_no = 0;
    uint16_t Address;
    uint16_t DataSize;

    uint8_t OperationResult;
    uint8_t *DataBuffer;

    // initialize variables
    PrepareMemBlock.DataInBufferReady = 0;
    PrepareMemBlock.DataOverflow = 0;
    PrepareMemBlock.MemoryBlockProcessState = 0;

    do {
        // prepare data to write
        OperationResult = iqrfPgmPrepareMemBlock(infile, &line_no);

        switch (OperationResult) {

        case IQRF_PGM_FLASH_BLOCK_READY:
        case IQRF_PGM_EEPROM_BLOCK_READY:

            // write prepared data to TR module
            while (PrepareMemBlock.MemoryBlockProcessState) {
                if (OperationResult == IQRF_PGM_FLASH_BLOCK_READY) {
                    if (PrepareMemBlock.MemoryBlockProcessState == 2) {
                        DataBuffer = (uint8_t *)&PrepareMemBlock.MemoryBlock[0];
                        DataSize = 32 + 2;
                    } else {
                        DataBuffer = (uint8_t *)&PrepareMemBlock.MemoryBlock[34];
                        DataSize = 32 + 2;
                    }
                } else {
                    DataBuffer = (uint8_t *)&PrepareMemBlock.MemoryBlock[0];
                    DataSize = PrepareMemBlock.MemoryBlock[1] + 2;
                }

                Address = ((uint16_t)DataBuffer[1] << 8) + DataBuffer[0];
                prgData.resize(DataSize-2);
                for (uint16_t i=0; i<DataSize-2; i++)
                    prgData[i] = DataBuffer[i+2];

                // write data to TR module
                if (OperationResult == IQRF_PGM_FLASH_BLOCK_READY) {
                    blines.push_back(HexDataRecord(Address, prgData, TrMemory::FLASH));
                } else {
                    blines.push_back(HexDataRecord(Address, prgData, TrMemory::INTERNAL_EEPROM));
                }

                PrepareMemBlock.MemoryBlockProcessState--;
            }
            break;

        case IQRF_PGM_ERROR:
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Error during hex file data processing!");
            break;

        case IQRF_PGM_FILE_SHORT_LINE:
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid length of record in hex file - line is too short!");
            break;

        case IQRF_PGM_FILE_LONG_LINE:
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid length of record in hex file - line is too long!");
            break;

        case IQRF_PGM_FILE_EVEN_LENGTH:
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid length of record in hex file - line length is not odd!");
            break;

        case IQRF_PGM_FILE_INVALID_CHAR:
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid character in hex file!");
            break;

        case IQRF_PGM_FILE_MISSING_START:
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 1, "Missing record start code : in hex file!");
            break;

        case IQRF_PGM_FILE_INVALID_SUM:
            TR_THROW_FMT_EXCEPTION(file_name, line_no, 0, "Invalid checksum of record in hex file!");
            break;
        }

    } while (OperationResult != IQRF_PGM_SUCCESS);
}
