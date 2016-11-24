/********************************** (C) COPYRIGHT *********************************
* File Name          : CH395.c
* Author             : WCH
* Version            : V1.1
* Date               : 2014/8/1
* Description        : CH395功能演示
**********************************************************************************/

/**********************************************************************************
CH395 TCP/IP 协议族接口
MSC51 演示程序，演示ICMP PING 功能，主动发送PING包以及回复PING包
上传。MCS51@24MHZ,KEIL 3.51
**********************************************************************************/

/* 头文件包含*/
#include <stc15f.h>
#include <stdio.h>
#include <string.h>
#include "CH395INC.H"
#include "CH395.H"
#include "CH395PING.H"
#include "Ch395UART.C"
#include "CH395CMD.C"
#include "INC_ALL.H"
/********************************************************************************/
/*
CH395_OP_INTERFACE_MODE可以为1-5
1：硬件总线并口连接方式
2：软件模拟并口连接方式
3: 硬件SPI连接方式
4: 软件模拟SPI方式
5: 硬件异步串口连接方式
*/

/* 包含命令文件 */


/**********************************************************************************/
/* CH395相关定义 */
const  UINT8 CH395MACAddr[6]  = {0x02,0x03,0x04,0x05,0x06,0x07};       /* CH395MAC地址 */
const  UINT8 CH395IPAddr[4]   = {192,168,1,251};                        /* CH395IP地址 */
const  UINT8 CH395GWIPAddr[4] = {192,168,1,1};                         /* CH395网关 */
const  UINT8 CH395IPMask[4]   = {255,255,255,0};                       /* CH395子网掩码 */
const  UINT8 DestIPAddr[4]    = {192,168,1,252};                       /* 目的IP */
const  UINT8 DestIPAddr_o[4]    = {192,168,0,252};                       /* 目的IP */
const  UINT8  IPRawProto      = 1;                                     /* IP包协议类型 */
UINT8 DisDestIP_BUF[4];
/*******************************************************************************/
/* 常用变量定义 */
UINT8 xdata MyBuffer[512];                                            /* 数据缓冲区 */
struct _SOCK_INF xdata SockInf;                                       /* 保存Socket信息 */
struct _CH395_SYS xdata CH395Inf;                                     /* 保存CH395信息 */

/**********************************************************************************
* Function Name  : mStopIfError
* Description    : 调试使用，显示错误代码，并停机
* Input          : iError 错误代码
* Output         : None
* Return         : None
**********************************************************************************/
void mStopIfError(UINT8 iError)
{
    if (iError == CMD_ERR_SUCCESS) return;                           /* 操作成功 */
    printf("Error: %02X\n", (UINT16)iError);                         /* 显示错误 */
 /*   while ( 1 ) 
    {
        mDelaymS(200);
        mDelaymS(200);
    }	*/
}
//#define stop(Dat)  mStopIfError(Dat)
/**********************************************************************************
* Function Name  : InitCH395InfParam
* Description    : 初始化CH395Inf参数
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void InitCH395InfParam(void)
{
    memset(&CH395Inf,0,sizeof(CH395Inf));                            /* 将CH395Inf全部清零*/
    memcpy(CH395Inf.IPAddr,CH395IPAddr,sizeof(CH395IPAddr));         /* 将IP地址写入CH395Inf中 */
    memcpy(CH395Inf.GWIPAddr,CH395GWIPAddr,sizeof(CH395GWIPAddr));   /* 将网关IP地址写入CH395Inf中 */
    memcpy(CH395Inf.MASKAddr,CH395IPMask,sizeof(CH395IPMask));       /* 将子网掩码写入CH395Inf中 */
}

/**********************************************************************************
* Function Name  : InitSocketParam
* Description    : 初始化socket
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void InitSocketParam(char *DestIPaddress)
{
    memset(&SockInf,0,sizeof(SockInf));                              /* 将SockInf[0]全部清零*/
    memcpy(SockInf.IPAddr,DestIPaddress,4);            /* 将目的IP地址写入 */
    SockInf.ProtoType = PROTO_TYPE_IP_RAW;                           /* IP RAW模式 */
    SockInf.IPRAWProtoType = IPRawProto;
	memset(DisDestIP_BUF,0,4);                            /* 将CH395Inf全部清零*/
    memcpy(DisDestIP_BUF,DestIPaddress,4);         /* 将IP地址写入CH395Inf中 */       
}

/**********************************************************************************
* Function Name  : CH395SocketInitOpen
* Description    : 配置CH395 socket 参数，初始化并打开socket
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void CH395SocketInitOpen(void)
{
    UINT8 i;
    /* socket 0为IP RAW模式 */
    CH395SetSocketDesIP(0,SockInf.IPAddr);                           /* 设置socket 0目标IP地址 */         
    CH395SetSocketProtType(0,SockInf.ProtoType);                     /* 设置socket 0协议类型 */
    CH395SetSocketIPRAWProto(0,SockInf.IPRAWProtoType);              /* 设置协议字段 */
    i = CH395OpenSocket(0);                                          /* 打开socket 0 */
    mStopIfError(i);  												 /* 检查是否成功 */
	printf("CH395SocketInitOpen(void),,,,,,i = CH395OpenSocket(0);\n\r");      //////For test                                  
}

/**********************************************************************************
* Function Name  : CH395SocketInterrupt
* Description    : CH395 socket 中断,在全局中断中被调用
* Input          : sockindex
* Output         : None
* Return         : None
**********************************************************************************/
void CH395SocketInterrupt(UINT8 sockindex)
{
   UINT8  sock_int_socket;
   UINT16 len,i;
   sock_int_socket = CH395GetSocketInt(sockindex);                   /* 获取socket 的中断状态 */
   if(sock_int_socket & SINT_STAT_SENBUF_FREE)                       /* 发送缓冲区空闲，可以继续写入要发送的数据 */
   {
   }
   if(sock_int_socket & SINT_STAT_SEND_OK)                           /* 发送完成中断 */
   {
     IcmpSuc++;
   }
   if(sock_int_socket & SINT_STAT_RECV)                              /* 接收中断 */
   {
     len = CH395GetRecvLength(sockindex);                            /* 获取当前缓冲区内数据长度 */
     if(len == 0)return;
     if(len > 512) len = 512;
     CH395GetRecvData(sockindex,len,MyBuffer);                       /* 读取数据 */
	 for(i=0;i<len;i++)
	 SendData_Uart3(MyBuffer[i]); //test 
     CH395IcmpRecvData( len,MyBuffer );
   }
   if(sock_int_socket & SINT_STAT_CONNECT)                           /* 连接中断，仅在TCP模式下有效*/
   {
   }
   if(sock_int_socket & SINT_STAT_DISCONNECT)                        /* 断开中断，仅在TCP模式下有效 */
   {
   }
   if(sock_int_socket & SINT_STAT_TIM_OUT)                           /* 超时中断，仅在TCP模式下有效 */
   {
   }
}

/**********************************************************************************
* Function Name  : CH395GlobalInterrupt
* Description    : CH395全局中断函数
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void CH395GlobalInterrupt(void)
{
   UINT16  init_status;
   UINT8  buf[10]; 
 
    init_status = CH395CMDGetGlobIntStatus_ALL();
    if(init_status & GINT_STAT_UNREACH)                              /* 不可达中断，读取不可达信息 */
    {
        printf("Init status : GINT_STAT_UNREACH\n");                 /* UDP模式下可能会产生不可达中断 */
        CH395CMDGetUnreachIPPT(buf);  
		printf("Unreach Ip : %s\n\r",buf);                              
    }
    if(init_status & GINT_STAT_IP_CONFLI)                            /* 产生IP冲突中断，建议重新修改CH395的 IP，并初始化CH395*/
    {
    printf("IP conflict\n");
    }
    if(init_status & GINT_STAT_PHY_CHANGE)                           /* 产生PHY改变中断*/
    {
        printf("Init status : GINT_STAT_PHY_CHANGE\n");
    }
    if(init_status & GINT_STAT_SOCK0)
    {
        CH395SocketInterrupt(0);                                     /* 处理socket 0中断*/
    }
    if(init_status & GINT_STAT_SOCK1)                                
    {
        CH395SocketInterrupt(1);                                     /* 处理socket 1中断*/
    }
    if(init_status & GINT_STAT_SOCK2)                                
    {
        CH395SocketInterrupt(2);                                     /* 处理socket 2中断*/
    }
    if(init_status & GINT_STAT_SOCK3)                                
    {
        CH395SocketInterrupt(3);                                     /* 处理socket 3中断*/
    }
    if(init_status & GINT_STAT_SOCK4)
    {
        CH395SocketInterrupt(4);                                     /* 处理socket 4中断*/
    }
    if(init_status & GINT_STAT_SOCK5)                                
    {
        CH395SocketInterrupt(5);                                     /* 处理socket 5中断*/
    }
    if(init_status & GINT_STAT_SOCK6)                                
    {
        CH395SocketInterrupt(6);                                     /* 处理socket 6中断*/
    }
    if(init_status & GINT_STAT_SOCK7)                                
    {
        CH395SocketInterrupt(7);                                     /* 处理socket 7中断*/
    }
}

/**********************************************************************************
* Function Name  : CH395Init
* Description    : 配置CH395的IP,GWIP,MAC等参数，并初始化
* Input          : None
* Output         : None
* Return         : 函数执行结果
**********************************************************************************/
UINT8  CH395Init(void)
{

    UINT8 i;
    
    i = CH395CMDCheckExist(0x65);                      
    if(i != 0x9a)return CH395_ERR_UNKNOW;                            /* 测试命令，如果无法通过返回0XFA */
                                                                     /* 返回0XFA一般为硬件错误或者读写时序不对 */

//    CH395CMDSetUartBaudRate(UART_WORK_BAUDRATE);                     // 设置波特率  默认波特率9600bps
    mDelaymS(1);
//    SetMCUBaudRate();												 //已设置完成
//    i = xReadCH395Data();                                            // 如果baud设置成功，CH395返回CMD_ERR_SUCCESS 
//   if(i == CMD_ERR_SUCCESS)printf("Set Success\n");				   // Baud 设置成功 打印出 set success
    CH395CMDSetIPAddr(CH395Inf.IPAddr);                              /* 设置CH395的IP地址 */
    CH395CMDSetGWIPAddr(CH395Inf.GWIPAddr);                          /* 设置网关地址 */
    CH395CMDSetMASKAddr(CH395Inf.MASKAddr);                          /* 设置子网掩码，默认为255.255.255.0*/   
    i = CH395CMDInitCH395();                                         /* 初始化CH395芯片 */
    return i;
}

/**********************************************************************************
* Function Name  : mInitSTDIO
* Description    : 串口初始化,仅调试使用
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/

/**********************************************************************************
* Function Name  : main
* Description    : main主函数
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void CH395PINGInit(void)
{
    UINT8 i,j=0; 
    mDelaymS(100);
    printf("CH395EVT Test Demo\n");
    InitCH395InfParam();                                             /* 初始化CH395相关变量 */
    i = CH395Init();                                                 /* 初始化CH395芯片 */
    mStopIfError(i);
	printf("CH395PINGInit(void),,,,,i = CH395Init();\n\r");			//For test
    while(1)
    {                                                                /* 等待以太网连接成功*/
       if(CH395CMDGetPHYStatus() == PHY_DISCONN)                     /* 查询CH395是否连接 */
       {
           mDelaymS(200);                                            /* 未连接则等待200MS后再次查询 */
		   if(j++>5)
		   {
		   		j=0;
				break;
		   }
       }
       else 
       {
           printf("CH395 Connect Ethernet\n");                       /* CH395芯片连接到以太网，此时会产生中断 */
           break;
       }
    }
    InitSocketParam(DestIPAddr_o);                                               /* 初始化socket相关变量 并确定目标IP */
    CH395SocketInitOpen();
    InitParameter();
    InitPing( );
    Intervalometer_4ms( );	  //初始化定时器1  用作ping反馈时间的计时
}