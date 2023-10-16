# ReadMe
## About this repository
This program is an demo program about how to code an log function using UART in DMA Mode.
My test board is Nucleo-H723ZG and I use the VCP port(UART3) as the target port. The arch of H7 series is too complicated.   

When you use H7, please take care about its domains(D1/D2/D3) and its different RAM (AXI RAM, ITCM RAM, DTCM RAM, AXT and ITCM shareable RAM, SRAM1/SRAM2[/SRAM3], SRAM4 and backup RAM). When you don't use MDAM, please note the RAM area accessible by DMA1, DMA2.

This project provides two IDE programs, IAR and KEIL MDK. The array area (SRAM1) is specially designated in the program. Please pay attention to modifying this part of the code when transplanting it to other series of MCUs.