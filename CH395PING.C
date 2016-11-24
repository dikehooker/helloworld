/********************************** (C) COPYRIGHT *********************************
* File Name          : CH395PING.C
* Author             : WCH
* Version            : V1.1
* Date               : 2014/8/1
* Description        : CH395芯片PING相关函数
**********************************************************************************/
#include <stc15f.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "CH395INC.H"
#include "CH395CMD.H"
#include "CH395.H"
#include "CH395PING.H"
#include "INC_ALL.H"

UINT8  UNREACH_COUNT;
UINT8  TIMOUT_COUNT;
UINT8  SUCRECV_COUNT;
UINT8  CH395INF_BUF[20];
UINT8  SEND_BUF[100];
UINT8  IcmpCont;
UINT8  IcmpSeq;
UINT8  IcmpSuc;
UINT8  icmp_tmp;
UINT32 TimeCount;

/**********************************************************************************
* Function Name  : Intervalometer_4ms
* Description    : 定时器初始化
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void Intervalometer_4ms( )
{	
	AUXR &= 0xBF;		//定时器时钟12T模式
	TMOD &= 0x0F;		//设置定时器模式
	TL1 = 0x9A;		//设置定时初值
	TH1 = 0xF1;		//设置定时初值
	TF1 = 0;		//清除TF1标志
	TR1 = 1; 
}

/**********************************************************************************
* Function Name  : InitPing
* Description    : Ping初始化
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void InitParameter( void )
{
	UINT8 i=0;
    UNREACH_COUNT=0;
    TIMOUT_COUNT=0;
    SUCRECV_COUNT=0;
    IcmpCont=0;
    IcmpSeq=0;
    IcmpSuc=0;
    icmp_tmp=0;
    CH395GetIPInf( CH395INF_BUF ); 	 //获取IP数据 并保存到CH395INF_BUF中
	for(i=0;i<4;i++)
	{
		SendData_Uart4(CH395INF_BUF[i]);   //ch395 IP数据
	}
//	printf("CH395GetInf=%s\n\r",CH395INF_BUF);
}

/**********************************************************************************
* Function Name  : InitPing
* Description    : Ping初始化
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void InitPing( void )
{
  IcmpHeader head;
  UINT32 check_sum=0;
  UINT8 i;

  IcmpCont++;
  IcmpSeq += 10;
  head.i_type = ICMP_HEAD_TYPE;
  head.i_code = ICMP_HEAD_CODE;
  head.i_id   = ICMP_HEAD_ID;
  head.i_seq  = ICMP_HEAD_SEQ+IcmpSeq; 	//	100 +10
  memset(head.i_data,0,sizeof(head.i_data));
  for( i=0;i<ICMP_DATA_BYTES;i++ ){
    if(i<26) head.i_data[i] = i + 'a';
    else head.i_data[i] = i + 'a' - 26;
    if(i%2==0) check_sum += head.i_data[i]<<8;
    else check_sum += head.i_data[i]; 
  }
  check_sum += head.i_type<<8;
  check_sum += head.i_code;
  check_sum += head.i_id;
  check_sum += head.i_seq;
  head.i_cksum = check_sum>>16;
  head.i_cksum += check_sum&0xffff;
  head.i_cksum = 0xffff - head.i_cksum;
  memset(SEND_BUF,0,sizeof(SEND_BUF));
  memcpy(SEND_BUF,&head,sizeof(head));
}

/**********************************************************************************
* Function Name  : InitPing
* Description    : Ping初始化
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void Respond_Ping( UINT8 *pDat )
{
  IcmpHeader head;
  UINT32 check_sum=0;
  UINT8 i;

  head.i_type = ICMP_HEAD_REPLY;
  head.i_code = pDat[1];
  head.i_id   = pDat[4]<<8 + pDat[5];
  head.i_seq  = pDat[6]<<8 + pDat[7]; 
  check_sum += head.i_type<<8;
  check_sum += head.i_code;
  check_sum += head.i_id;
  check_sum += head.i_seq;
  for( i=0;i<32;i++ ){
    head.i_data[i] = pDat[i+8];
    if(i%2==0) check_sum += head.i_data[i]<<8;
    else check_sum += head.i_data[i]; 
  }
  head.i_cksum = check_sum>>16;
  head.i_cksum += check_sum&0xffff;
  head.i_cksum = 0xffff - head.i_cksum;
  memset(SEND_BUF,0,sizeof(SEND_BUF));
  memcpy(SEND_BUF,&head,sizeof(head));
}

/**********************************************************************************
* Function Name  : InitPing
* Description    : Ping初始化
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void CH395IcmpRecvData( UINT32 len, UINT8 *pDat )
{
  UINT16 tmp=0;

  icmp_tmp = IcmpSuc; 
  IcmpSuc = 3;		//
  if( len == 40 ){
    if( pDat[0] == ICMP_HEAD_REPLY ){
      if( pDat[1] == ICMP_HEAD_CODE ){
        tmp = pDat[4];
        tmp = tmp<<8; 
        tmp += pDat[5]; 	//tmp 是高字节是pDat[4]和低字节pDat[4]合并以后形成的16字节数据
        if( tmp == ICMP_HEAD_ID ){		   //ICMP_HEAD_ID 512
          tmp = pDat[6];
          tmp = (tmp<<8); 
          tmp += pDat[7] - IcmpSeq; 	   //tmp 是高字节是pDat[6]和低字节pDat[7]合并以后形成的16字节数据
          if( tmp == ICMP_HEAD_SEQ ){	 // ICMP_HEAD_SEQ 100
            IcmpSuc = 4;
          }
        }
      }
    }
    if( pDat[0] == ICMP_HEAD_TYPE ){
      if( pDat[1] == ICMP_HEAD_CODE ){
        Respond_Ping(pDat);
        IcmpSuc = 6;
      }
    }
  }
  else{
    if( pDat[0] == 3 ){
      if( pDat[1] == 1 ){
        IcmpSuc = 5;
      }
    }
  }
}
/**********************************************************************************
* Function Name  : CH395_PINGCmd
* Description    : 查询状态执行相应命令
* Input          : None
* Output         : None
* Return         : None
**********************************************************************************/
void CH395_PINGCmd( )                                          
{
  if( IcmpSuc<=ICMP_KEEP_NO ){
    switch(IcmpSuc){
      case ICMP_SOKE_CON:	    //	  ICMP_SOKE_CON 0 
        IcmpSuc = 1;
        CH395SendData(0,SEND_BUF,40);
		
      /* printf( "\nPinging %d.%d.%d.%d with %d bytes of data:\n\r\n",(UINT16)DestIPAddr[0],(UINT16)DestIPAddr[1],\
        (UINT16)DestIPAddr[2],(UINT16)DestIPAddr[3],(UINT16)ICMP_DATA_BYTES ); */ 
        TimeCount = 0;
        break;
      case ICMP_SEND_ERR:				//ICMP_SEND_ERR 1
          if(TimeCount>250){			   //等待1s时间 如果仍然没有收到中断 需要重新发送一次数据
          printf("send data fail!\n");
          CH395SendData(0,SEND_BUF,40);
          TimeCount = 0;
        }
        break;
      case ICMP_SEND_SUC:  				   //  ICMP_SEND_SUC 2
        if( TimeCount>2000 ){			//4*2000=8000ms
          printf("Request time out.\n");
          TIMOUT_COUNT++;
          if(IcmpCont < 4){		   //Icmp 是发起ping的次数
                    IcmpSuc = 1;
                    InitPing( );
                    CH395SendData(0,SEND_BUF,40);
                    TimeCount = 0;
              }
          else{
          printf("\r\nPing statistics for %d.%d.%d.%d:\n    Packets: Sent = 4,Received = %d,Lost = %d<%d%% loss>.\n\r\n",(UINT16)DestIPAddr[0],\
            (UINT16)DestIPAddr[1],(UINT16)DestIPAddr[2],(UINT16)DestIPAddr[3],(UINT16)(4-TIMOUT_COUNT),(UINT16)TIMOUT_COUNT,\
            (UINT16)(TIMOUT_COUNT*25));
            IcmpSuc = ICMP_KEEP_NO;
          }
        }
        break;
      case ICMP_RECV_ERR: 	   // ICMP_RECV_ERR	    	3
        printf("recevie data unknow.\n\r\n");
        IcmpSuc = ICMP_KEEP_NO;
        break;
      case ICMP_RECV_SUC:  		   //		 ICMP_RECV_SUC	    	4
        printf( "Reply from %d.%d.%d.%d: bytes=%d time<4ms\n",(UINT16)DisDestIP_BUF[0],(UINT16)DisDestIP_BUF[1],(UINT16)DisDestIP_BUF[2],\
        (UINT16)DisDestIP_BUF[3],(UINT16)ICMP_DATA_BYTES );
        SUCRECV_COUNT++;
        if(IcmpCont < 4){
          IcmpSuc = 1;
          InitPing( );
          TimeCount = 0;
          CH395SendData(0,SEND_BUF,40);
            }
        else{
          /*  printf("\r\nPing statistics for %d.%d.%d.%d:\n    Packets: Sent = 4,Received = %d,Lost = %d<%d%% loss>.\n\r\n",(UINT16)DestIPAddr[0],\
            (UINT16)DestIPAddr[1],(UINT16)DestIPAddr[2],(UINT16)DestIPAddr[3],(UINT16)SUCRECV_COUNT,(UINT16)(4-SUCRECV_COUNT),\
            (UINT16)((4-SUCRECV_COUNT)*25));*/
            IcmpSuc = ICMP_KEEP_NO;
        }
        break;
      case ICMP_UNRECH: 	   //  ICMP_UNRECH	            5
        printf("Reply from %d.%d.%d.%d: Destination host unreachable.\n",(UINT16)DestIPAddr[0],(UINT16)DestIPAddr[1],\
        (UINT16)DestIPAddr[2],(UINT16)DestIPAddr[3]);
        UNREACH_COUNT++;
        if(IcmpCont < 4){
                  IcmpSuc = 1;
                  InitPing( );
                  TimeCount = 0;
                  CH395SendData(0,SEND_BUF,40);
            }
        else{
            printf("\r\nPing statistics for %d.%d.%d.%d:\n    Packets: Sent = 4,Received = %d,Lost = %d<%d%% loss>.\n\r\n",(UINT16)DestIPAddr[0],\
            (UINT16)DestIPAddr[1],(UINT16)DestIPAddr[2],(UINT16)DestIPAddr[3],(UINT16)UNREACH_COUNT,(UINT16)(4-UNREACH_COUNT),\
            (UINT16)((4-UNREACH_COUNT)*25));
            IcmpSuc = ICMP_KEEP_NO;
        }
        break;
      case ICMP_REPLY: 		   //	 ICMP_REPLY	            6
        CH395SendData( 0,SEND_BUF,40 );
        break;
      case ICMP_REPLY_SUC: 			  //ICMP_REPLY_SUC	        7
        printf( "Reply ping.\n\r\n" );
        IcmpSuc = icmp_tmp;
        break;
      case ICMP_KEEP_NO:			 //ICMP_KEEP_NO	       10
      //  CH395CloseSocket(0);
        IcmpSuc = 0xff;
        break;
        default:
        break;
      }
  }
    if( TF1 ){
    TF1 = 0;
    TimeCount++;
    if(TimeCount>100000) TimeCount = 10000;
  }
}