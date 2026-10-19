# TASTE-Reusable-Components
This repository includes reusable components for inclusion in TASTE applications.

## Content
This repository contains the following reusable components:
- **DataCompressor** - For Deflate compression
- **DataUncompressor** - For Deflate decompression

Both of the above reusable components make use of the Zlib compression library. 

The repository also contains an example TASTE application (in the folder ```test-project```) which is designed to demonstrate the compression and decompression reusable components, over a UART interface running on either a SAMRH71 or SAMV71 processor.

## Depedencies
These reusable components depend upon on a recent TASTE tool chain installation, along with the runtime for the relevant processors - SAMV71 and/or SAMRH71. (See https://gitlab.esa.int/taste/taste-setup/-/wikis/home for details on installation)

These reusable components have been tested on the SAMRH71 and SAMV71 EK development boards. 

### Build Configurations
The underlying ZLib library relies upon dynamic memory allocation with a heap. Since the SAMRH71 and SAMV71 TASTE runtimes use RTEMS which does not support dynamic memory allocation, Zlib has been modified so that a number of buffers are statically allocated at compile time. These buffers are described below. When tuning memory allocated for ZLib use its also important to review the working memory configuration of ZLib (see below)

#### Data Input / Output Buffers
The TASTE components for compression / decompression include a buffer for holding the data to be processed, and the result of the data following the processing. Both of these buffers default to 4096 bytes in size. This memory allocation can be adjusated by defining the compile time constants ```DATACOMPRESSOR_INPUT_BUFFER_SIZE```, ```DATACOMPRESSOR_OUTPUT_BUFFER_SIZE```, ```DATAUNCOMPRESSOR_INPUT_BUFFER_SIZE```, ```DATAUNCOMPRESSOR_OUTPUT_BUFFER_SIZE```. Setting these constants will define the maximum size of the uncompressed and compressed data that the application will support.

#### Memory Allocated for the ZLib library
The memory made available for ZLib's use is defined in a heap, which defaults to 384 KB. This heap allocation is defined by the compile time constant ```STATIC_POOL_SIZE``` (the number is specified in bytes). By default ZLib will use around ~400 KB to perform its compression (decompression tends to be less costly).  If the compression fails, it may be that Zlib requires more memory, adjusting this compile time constant can be done to provide it further resource. See also Working Memory Used By Zlib

#### Working Memory Used By ZLib
ZLib is unaware of the limitations of the heap mentioned above, and instead its actual memory footprint is defined natively by the two internal parameters windowsBits and memLevel. By default these are 15 and 8 respectively

The actual memory footprint of ZLib can be computed roughly as follows: ```(1 << (windowBits + 2)) + (1 << (memLevel + 9))```
Thus for windowBits=15 and memLevel=8 (default values), the memory footprint is is 256KB; Reducing these to windowBits=14 and memLevel=7, the memory footprint drops to just 128 KB.

The values of the internal windowBits and memLevel parameters can be set via the compile time constants:
* windowBits: ```MAX_WBITS```
* memLevel: ```MAX_MEM_LEVEL```

For applications running on the SAMV71 - and required to perform their compression/decompression within internal RAM - it is recommended to use the following values:
* ```MAX_WBITS=14```
* ```MAX_MEM_LEVEL=7```

#### Setting Compiler Flags

To change the memory used by Zlib add the following to your Makefile (adjusting the figures as required):
````
export PARTITION_1_USER_CFLAGS=-DMAX_WBITS=14 -DMAX_MEM_LEVEL=7 -DSTATIC_POOL_SIZE=163840 
````

The above sets windowBits to 14, memLevel to 7, and STATIC_POOL_SIZE to 160 KB.

# Sample Application TASTE Model

The sample application periodically sends a message over a serial connection to indicate its status, while waiting for user input. User input is provided over the serial connection. Once the application receives a string followed by four ```#``` characters it will then commence to compress - and subseqeutly decompress - the provided string, reporting the results over the serial connection.

The sample application performs all its compression and decompression using only the internal SRAM of the processor. For serial the model makes use of the ```FLEXCOM1``` UART peripheral (SAMRH71) or the ```UART0``` UART peripheral (SAMV71).

## Installation & building
To run the sample application TASTE model
1. Clone this repository into a directory off your home directory on a Linux installation.
2. In a terminal window enter the directory where you have cloned the TASTE project and navigate to the ```test-project\lossless-comp-decomp``` folder.
3. Type "taste" to launch the TASTE IDE

![interface view of TASTE reusable components](https://github.com/KISPE-Space/TASTE-Reusable-Components/blob/main/screenshots/TASTE%20model_1.png)

In order to build the model, navigate to the Deployment View for the relevant target (i.e., double click on either ```samrh71.dv.xml``` or ```samv71.dv.xml```) and then click the build button (hammer icon, bottom left).

![deployment view of TASTE reusable components](https://github.com/KISPE-Space/TASTE-Reusable-Components/blob/main/screenshots/TASTE%20model_2.png)

## Running the test model
In order to run the test model: 
- Use a JTAG probe (e.g. SEGGER J-Link) and connect via GDB to the SAMRH71 EK development board. (Note that the Atmel ICD debugger is incompatible with running TASTE models over gdb sessions)
- Connect the relevant UART TX/TX pins through an FTDI adapter to a host computer using a USB connection. Serial configuration: Baud 115200, 8E1
- Start gdb, and load the ```work/binaries/partition_1``` binary. (Note that the partition_2 binary is merely a dummy interface sink and not intended for deployment anywhere).
- On the host computer, start minicom (or some other terminal emulator that supports both TX and RX), connecting it to the relevant USB serial port
- In gdb enter "continue" to run the application

You should now see the following output on the terminal:

![deployment view of TASTE reusable components](https://github.com/KISPE-Space/TASTE-Reusable-Components/blob/main/screenshots/TASTE%20model_3.png)

By streaming data to the terminal (either a file or by typing a message), you will observe the compression and decompression process. Note, the example program waits for ```####``` to be typed on the terminal to signify the end of the data to compress and then decompress.

