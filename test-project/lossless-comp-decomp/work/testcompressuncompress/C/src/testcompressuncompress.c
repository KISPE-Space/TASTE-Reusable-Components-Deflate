/*
 * @copyright 2026 KISPE Space Systems Ltd.
 *
 * Licensed under the ESA Public License (ESA-PL) Permissive (Type 3),
 * Version 2.4 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://essr.esa.int/license/list
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "testcompressuncompress.h"
#include <stdio.h>
#include <string.h>

char iDebugString[300];

unsigned int bTCU_ReadyToCompress = 0;

void transmit_bytes_over_uart(char* pBytes, unsigned int iNumBytes);
void register_uart_byte_transmitter_function(void (*pByteTransmitterFunction)(const asn1SccT_UInt8*));

#define CHUNK_SIZE 32u

void testcompressuncompress_startup(void)
{
    register_uart_byte_transmitter_function(testcompressuncompress_RI_samrh71tx);
    transmit_bytes_over_uart("[TCU] This console is driven by a test interface designed to test the\n", 0);
    transmit_bytes_over_uart("TASTE reusable components that perform lossless compression\n", 0);
    transmit_bytes_over_uart("and decompression\n", 0);
    transmit_bytes_over_uart("Stream data onto this UART link and terminate with \"####\" to start compression\n", 0);
    transmit_bytes_over_uart("Decompression should then follow\n", 0);
}

void testcompressuncompress_PI_RunOnce(void)
{

    asn1SccCompressorState teUncompressionState = CompressorState_compressor_not_initialized;
    asn1SccCompressorState teCompressionState = CompressorState_compressor_not_initialized;


    asn1SccT_UInt32 compressedLength = 0;
    asn1SccT_UInt32 uncompressedLength = 0;
    unsigned int iMappingCounter = 0;

    typedef struct tsEnumMapping
    {
        asn1SccCompressorState state_number;
        char state_as_string[30];
    } tsEnumMapping;

    tsEnumMapping tsStateMapping[6] =
    {
        {CompressorState_compressor_not_initialized, " Not Initialised"},
        {CompressorState_compressor_idle, " Idle"},
        {CompressorState_compressor_writing_data, " Writing Data"},
        {CompressorState_compressor_working, " Working"},
        {CompressorState_compressor_success, " Success"},
        {CompressorState_compressor_fail, " Fail"}
    };


    // every 5 seconds, print the status of compressor and uncompressor

    // find uncompressor state
    testcompressuncompress_RI_Uncomp_Status(&teUncompressionState, &uncompressedLength);

    // run through the LUT to find the right string
    for (iMappingCounter = 0UL ; iMappingCounter < 6 ; iMappingCounter++)
    {
        // if the state is found
        if (tsStateMapping[iMappingCounter].state_number == teUncompressionState)
        {
            // print the state
            sprintf(iDebugString, "\n** Uncompressor State is%s, length %i", tsStateMapping[iMappingCounter].state_as_string, uncompressedLength);
            transmit_bytes_over_uart(iDebugString, 0);
        }
    }

    // find compressor state
    testcompressuncompress_RI_Comp_Status(&teCompressionState, &compressedLength);

    // run through the LUT to find the right string
    for (iMappingCounter = 0UL ; iMappingCounter < 6 ; iMappingCounter++)
    {
        // if the state is found
        if (tsStateMapping[iMappingCounter].state_number == teCompressionState)
        {
            // print the state
            sprintf(iDebugString, "\n** Compressor State is%s, length %i\n", tsStateMapping[iMappingCounter].state_as_string, compressedLength);
            transmit_bytes_over_uart(iDebugString, 0);
        }
    }


    // Check if the UART buffer has received data to compress
    if (1 == bTCU_ReadyToCompress)
    {
        // clear the decompression buffers
        testcompressuncompress_RI_Uncomp_ClearBuffer();

        bTCU_ReadyToCompress = 0;

        // perform compression
        testcompressuncompress_RI_Comp_StartCompression();

        // Check the status
        testcompressuncompress_RI_Comp_Status(&teCompressionState, &compressedLength);

        // If compression was successful
        if (CompressorState_compressor_success == teCompressionState)
        {
            // retrieve and print the compressed data
            uint32_t totalRead = 0;
            sprintf (iDebugString, "[TCU] Compression successful to the following form:\n");
            transmit_bytes_over_uart(iDebugString,0 );

            asn1SccCompressorDataBuffer chunk;
            asn1SccT_UInt32 bytesWritten = 0;

            while (totalRead < compressedLength)
            {

                asn1SccT_UInt32 bytesRemaining = compressedLength - totalRead;
                asn1SccT_UInt32 bytesToRead =
                    bytesRemaining < CHUNK_SIZE ? bytesRemaining : CHUNK_SIZE;
                asn1SccT_UInt32 byteOffset = totalRead;


                testcompressuncompress_RI_Comp_ReadData(&chunk, &byteOffset, &bytesToRead, &bytesWritten);

                if (bytesWritten == 0)
                {
                    sprintf(iDebugString, "\n[TCU] Comp_ReadData returned 0 bytes at offset %u\n", (unsigned)totalRead);
                    transmit_bytes_over_uart(iDebugString, 0);
                }
                // print the compressed data
                for (unsigned int i = 0 ; i < bytesWritten ; i++)
                {
                    sprintf (iDebugString, "0x%x ", chunk.arr[i]);
                    transmit_bytes_over_uart(iDebugString, 0);

                    // print a newline every 16 chars
                    if (!((i+1) % 16))
                    {
                        transmit_bytes_over_uart("\n", 0);
                    }
                }

                // Pass each chunk to the uncompressor
                testcompressuncompress_RI_Uncomp_WriteBuffer(&chunk, &bytesWritten);
                totalRead += bytesWritten;
            }


            // now try and uncompress stuff.
            transmit_bytes_over_uart("\n[TCU] Now attempting to decompress data\n", 0);
            testcompressuncompress_RI_Uncomp_StartUncompression();

            // read the uncompressed state
            testcompressuncompress_RI_Uncomp_Status(&teUncompressionState, &uncompressedLength);

            if (CompressorState_compressor_success == teUncompressionState)
            {
                // Now print the decompressed output

                // retrieve and print the decompressed data
                totalRead = 0;
                sprintf (iDebugString, "\n[TCU] Decompression successful to the following form:\n");
                transmit_bytes_over_uart(iDebugString,0 );

                bytesWritten = 0;

                while (totalRead < uncompressedLength)
                {

                    asn1SccT_UInt32 bytesRemaining = uncompressedLength - totalRead;
                    asn1SccT_UInt32 bytesToRead =
                    bytesRemaining < CHUNK_SIZE ? bytesRemaining : CHUNK_SIZE;
                    asn1SccT_UInt32 byteOffset = totalRead;


                    testcompressuncompress_RI_Uncomp_ReadData(&chunk, &byteOffset, &bytesToRead, &bytesWritten);

                    if (bytesWritten == 0)
                    {
                        sprintf(iDebugString, "\n[TCU] Uncomp_ReadData returned 0 bytes at offset %u\n", (unsigned)totalRead);
                        transmit_bytes_over_uart(iDebugString, 0);
                    }
                    // print the uncompressed data
                    for (unsigned int i = 0 ; i < bytesWritten ; i++)
                    {
                        transmit_bytes_over_uart((char *)&chunk.arr[i], 1);
                    }
                    totalRead += bytesWritten;
                }
            }
        }
        // clear the compression buffer
        testcompressuncompress_RI_Comp_ClearBuffer();
    }
}

// From this point on is UART handling s/w

// This function is called when a single byte is received on the UART.
void testcompressuncompress_PI_samrh71rx(const asn1SccT_UInt8 *iInByte)
{
    static unsigned int iNumberOfDelimiterChars = 0UL;
    unsigned int iCounter = 0UL;
    unsigned long iLengthToWrite = 1;
    asn1SccCompressorDataBuffer tsDataToWrite;

    // look for the 4 delimiter characters "#" that mark the end of the data
    if (iInByte[0] == '#')
    {
        iNumberOfDelimiterChars++;

        // IF this is the end of the data THEN
        if (4UL == iNumberOfDelimiterChars)
        {
            iNumberOfDelimiterChars = 0UL;
            // Trigger the compression in the cyclic code
            bTCU_ReadyToCompress = 1;
        }
    }

    // ELSE
    else
    {
        // IF there was a delimiter character in the data THEN
        if (iNumberOfDelimiterChars != 0UL)
        {
            // Send the delimiters for compression together with the current character
            for (iCounter = 0UL ; iCounter < iNumberOfDelimiterChars ; iCounter++)
            {
                // send a '#'
                tsDataToWrite.arr[0] = '#';
                testcompressuncompress_RI_Comp_WriteBuffer(&tsDataToWrite, (unsigned int *)&iLengthToWrite);
            }
            // send a char
            tsDataToWrite.arr[0] = iInByte[0];
            testcompressuncompress_RI_Comp_WriteBuffer(&tsDataToWrite, (unsigned int *)&iLengthToWrite);
        }
        // ELSE
        else
        {
            // Send the next character
             tsDataToWrite.arr[0] = iInByte[0];
            testcompressuncompress_RI_Comp_WriteBuffer(&tsDataToWrite, (unsigned int *)&iLengthToWrite);
        }
        // END IF
    }
    // END IF
}


static void (*pMainByteTransmitterFunction)(const asn1SccT_UInt8*) = 0;

void register_uart_byte_transmitter_function(void (*pByteTransmitterFunction)(const asn1SccT_UInt8*))
{
    pMainByteTransmitterFunction = pByteTransmitterFunction;
}

// Transmit a number of bytes on the UART.
// If the number of bytes is 0, this function looks for the first null char to identify length.
void transmit_bytes_over_uart(char* pBytes, unsigned int iNumBytes)
{
    unsigned long charBuff;
    unsigned int iStringLength = 0UL;

    // If the fucntion pointer is not initialised, handle gracefully
    if (0 == pMainByteTransmitterFunction)
    {
        return;
    }
    if (0 == iNumBytes)
    {
        iStringLength = strlen(pBytes);
    }
    else
    {
        iStringLength = iNumBytes;
    }
    for (unsigned long i=0; i<iStringLength; i++)
    {
        charBuff = pBytes[i];
        pMainByteTransmitterFunction((asn1SccT_UInt8 *)&charBuff);
    }
}

void report_test_result_over_uart(char* pTestId, bool bIsPass, char* pFailReason)
{
    char aTestIdBuff[100];
    char aFailReasonBuff[200];
    sprintf(aTestIdBuff, "<TEST_ID>%s\n", pTestId);
    transmit_bytes_over_uart(aTestIdBuff, 0);
    if (bIsPass) {
        transmit_bytes_over_uart("<TEST_RESULT>PASS\n", 0);
    } else {
        transmit_bytes_over_uart("<TEST_RESULT>FAIL\n", 0);
        sprintf(aFailReasonBuff, "<TEST_FAIL_REASON>%s\n", pFailReason);
        transmit_bytes_over_uart(aFailReasonBuff, 0);
    }
}


