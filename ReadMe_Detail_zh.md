# ReadMe with detail
## 关于本仓库
该程序是一个演示程序，介绍如何在 DMA 模式下使用 UART 编写日志功能。
我的测试板是Nucleo-H723ZG，我使用VCP端口（UART3）作为目标端口。 H7系列的系统框架（ARCH）太复杂了。

当您使用H7时，请注意它的域（D1/D2/D3）和不同的RAM（AXI RAM、ITCM RAM、DTCM RAM、AXT和ITCM共享RAM、SRAM1/SRAM2[/SRAM3]、SRAM4和备份RAM ）。 当您不使用 MDAM 时, 请注意 DMA1、DMA2 可访问的 RAM 区域。

本项目提供了IAR和KEIL MDK两个IDE的程序。程序中专门指定了数组的区域（SRAM1），这部分代码在移植到其它系列的MCU时注意修改。

##