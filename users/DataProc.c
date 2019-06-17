#include "DataProc.h"
#include <string.h>

//SPIType	SPI_Using = SPI_PORT1;	//选择一个SPI接口

extern void IWDG_Feed(void);

u8 CheckData_1356[10] = {0xD1,0xD1,0xE0,0xD0,0x00,0x00,0x00,0x00,0xD2,0xD2}; //13.56M接收数据
u8 GetNum_1356[5] = {0x00,0x00,0x00,0x00,0x00}; //13.56M缓存

u8 uartSendData[100]; //最大包100个字节
u8 uartRecData[100]; //最大包100个字节

u8 uartSendRdNumData[100];
u8 uartSendWrNumOkData[100];

u16 count;
u8 flag_1ms;

u8 Flag_update;

struct Protocol_send_data   //通讯协议 发送
{
    u8 HeadFix;
    u8 Control;
    u8 Magic[4];
    u8 SequenceID[4];
    u8 Command;
    u8 Tag;
    u8 Length;
    u8 Value[6];            //最大值是6
    u8 FootCRC[2];          //CRC 暂时固定为00 00
    u8 FootFix;             //固定为0xFB
};

struct Protocol_rec_data   //通讯协议 接收
{
    u8 HeadFix;
    u8 Control;
    u8 Magic[4];
    u8 SequenceID[4];
    u8 Command;
    u8 Tag;
    u8 Length;
    u8 Value[62];           //最大值是62
    u8 FootCRC[2];          //CRC 暂时固定为00 00
    u8 FootFix;             //固定为0xFB
};

struct ReadNum_send_data   //读号回复指令
{
    u8 HeadFix;
    u8 Control;
    u8 Magic[4];
    u8 SequenceID[4];
    u8 Command;
    u8 Tag1;
    u8 Length1;
    u8 Value1[6];            //最大值是6
    u8 Tag2;
    u8 Length2;
    u8 Value2[6];            //最大值是6
    u8 FootCRC[2];          //CRC 暂时固定为00 00
    u8 FootFix;             //固定为0xFB
};

struct WrNumOk_send_data   //写卡成功回复指令
{
    u8 HeadFix;
    u8 Control;
    u8 Magic[4];
    u8 SequenceID[4];
    u8 Command;
    u8 FootCRC[2];          //CRC 暂时固定为00 00
    u8 FootFix;             //固定为0xFB
};




#define ReceiveBuf ((struct Protocol_rec_data *)&uartRecData[0])
#define SendBuf ((struct Protocol_send_data *)&uartSendData[0])

#define WrNumOkBuf ((struct WrNumOk_send_data *)&uartSendWrNumOkData[0])
#define ReadNumBuf ((struct ReadNum_send_data *)&uartSendRdNumData[0])

#define	CndRspBufferSize  100

unsigned char CmdRspBuffer[CndRspBufferSize];
unsigned char CmdRspBufferWr_Rx = 0;
u8 idBuf[5];    //ID缓存空间
u8 newIdFlag;   //写2.4G ID标志
u8 recFlag;

unsigned char txBuf[32]="123456789123456789";
unsigned char rspFlag = 0;  //是否处理答题卡上电2.4G信息 标志
unsigned char wrNumFlag = 0;//写号是否完成标志

/****************************************************
功能: 校验和

方法: sum = ~(data0+...+datan)+1;
*****************************************************/
u8 CheckSum(u8 *buf,u8 len)
{
	u8 sum = 0;
	u8 i;
	for(i = 0;i < len; i++)
	{
		sum += buf[i];
	}
	sum = ~sum;
	sum += 1;
	return sum;
}

//void AT_AddNewRxByte(u8 c)
//{
//	if (c != 0x0D)
//	{
//		if (c < 32) return;											// ignore control charcters			
//		if (CmdRspBufferWr_Rx >= (CndRspBufferSize - 1)) return;	       // buffer overflow - ignore character
//		CmdRspBuffer[CmdRspBufferWr_Rx++] = c;
//		CmdRspBuffer[CmdRspBufferWr_Rx] = 0;	
//		return;
//	}
//	//----------------------------------------------------
//	// ID=XXXXX
//	if((CmdRspBuffer[0] == 'I')&&(CmdRspBuffer[1] == 'D')&&(CmdRspBuffer[2] == '='))	
//	{
//	   memcpy(idBuf,&CmdRspBuffer[3],5);
//	   SendConsoleStr("id=");
//	   SendConsoleData(idBuf,5);
//	   SendConsoleStr("\n");
//	   newIdFlag = 1;
//	}
//	else if((CmdRspBuffer[0] == 'I')&&(CmdRspBuffer[1] == 'D')&&(CmdRspBuffer[2] == '?'))
//	{
//	  	SendConsoleStr("id=");
//	   	SendConsoleData(idBuf,5);
//		SendConsoleStr("\n");  		
//	}
//	memset(CmdRspBuffer,0,sizeof(CmdRspBuffer));
//	CmdRspBufferWr_Rx = 0;
//}


void Data_Init(void)    //包数据初始化
{
    SendBuf->HeadFix = 0xFA;        //发送缓冲区初始化
    SendBuf->Control = 0x10;
    SendBuf->Magic[0]= 0;
    SendBuf->Magic[1]= 0;
    SendBuf->Magic[2]= 0;
    SendBuf->Magic[3]= 0;
    SendBuf->SequenceID[0] = 0;
    SendBuf->SequenceID[1] = 0;
    SendBuf->SequenceID[2] = 0;
    SendBuf->SequenceID[3] = 0;
    SendBuf->FootCRC[0] = 0;
    SendBuf->FootCRC[1] = 0;
    SendBuf->FootFix = 0xFB;
    
    ReceiveBuf->HeadFix = 0xFA;         //接收缓冲区初始化
    ReceiveBuf->Control = 0x10;
    ReceiveBuf->Magic[0]= 0;
    ReceiveBuf->Magic[1]= 0;
    ReceiveBuf->Magic[2]= 0;
    ReceiveBuf->Magic[3]= 0;
    ReceiveBuf->SequenceID[0] = 0;
    ReceiveBuf->SequenceID[1] = 0;
    ReceiveBuf->SequenceID[2] = 0;
    ReceiveBuf->SequenceID[3] = 0;
    ReceiveBuf->FootCRC[0] = 0;
    ReceiveBuf->FootCRC[1] = 0;
    ReceiveBuf->FootFix = 0xFB;
    
    ReadNumBuf->HeadFix = 0xFA;         //接收缓冲区初始化
    ReadNumBuf->Control = 0x10;
    ReadNumBuf->Magic[0]= 0;
    ReadNumBuf->Magic[1]= 0;
    ReadNumBuf->Magic[2]= 0;
    ReadNumBuf->Magic[3]= 0;
    ReadNumBuf->SequenceID[0] = 0;
    ReadNumBuf->SequenceID[1] = 0;
    ReadNumBuf->SequenceID[2] = 0;
    ReadNumBuf->SequenceID[3] = 0;
    ReadNumBuf->FootCRC[0] = 0;
    ReadNumBuf->FootCRC[1] = 0;
    ReadNumBuf->FootFix = 0xFB;
    
    WrNumOkBuf->HeadFix = 0xFA;         //接收缓冲区初始化
    WrNumOkBuf->Control = 0x10;
    WrNumOkBuf->Magic[0]= 0;
    WrNumOkBuf->Magic[1]= 0;
    WrNumOkBuf->Magic[2]= 0;
    WrNumOkBuf->Magic[3]= 0;
    WrNumOkBuf->SequenceID[0] = 0;
    WrNumOkBuf->SequenceID[1] = 0;
    WrNumOkBuf->SequenceID[2] = 0;
    WrNumOkBuf->SequenceID[3] = 0;
    WrNumOkBuf->FootCRC[0] = 0;
    WrNumOkBuf->FootCRC[1] = 0;
    WrNumOkBuf->FootFix = 0xFB;
}

//void Usart1_Process(void)
//{
//    
//    if(USART1_RX_CNT==10 && USART1_RX_BUF[0]==0xd1 && USART1_RX_BUF[9]==0xd2)
//    {
//        USART1_REC_Status |= 0x8000;    //收到ID设置命令，十进制10位ID
//    }
//    
//    
//    if(USART1_REC_Status & 0x8000)
//    {
////        memcpy(idBuf,&USART1_RX_BUF[3],5);  //把5位十进制ID号写到idBuf
//        idBuf[0]=0;
//        idBuf[1]=USART1_RX_BUF[4];
//        idBuf[2]=USART1_RX_BUF[5];
//        idBuf[3]=USART1_RX_BUF[6];
//        idBuf[4]=USART1_RX_BUF[7];  //串口收到的ID号写入到idBuf缓冲区
//        
//        SendConsoleStr("id=");
//        SendConsoleData(idBuf,5);
//        SendConsoleStr("\n");
//        newIdFlag = 1;
//        
//        USART1_RX_CNT = 0;
//        USART1_REC_Status = 0;
//    }
//}

void DataProc_RecNrf24L01(void)
{
    u8 buffer[7];
    u8 temp;
    
    if(recFlag==1)  //判断IC是否已经写号
    {
        if(SPI2_IRQ == 0)
        {
            temp = NRF24L01_RxPacket(SPI_SEND,buffer);
            
//            SendConsoleData(buffer,RX_PLOAD_WIDTH);
            
            idBuf[0] = buffer[1];
            idBuf[1] = buffer[2];
            idBuf[2] = buffer[3];
            idBuf[3] = buffer[4];
            idBuf[4] = buffer[5];
            
            if(idBuf[0]==GetNum_1356[0] && idBuf[1]==GetNum_1356[1] && idBuf[2]==GetNum_1356[2] && idBuf[3]==GetNum_1356[3] && idBuf[4]==GetNum_1356[4])
            {
                recFlag = 2;    //已写2.4G卡号
//                newIdFlag = 0;  //不写2.4G卡号
            }
            else
            {
                recFlag = 0;
                idBuf[0] = 0;
                idBuf[1] = 0;
                idBuf[2] = 0;
                idBuf[3] = 0;
                idBuf[4] = 0;
            }
        }
    }
    
}

void DataProc_Nrf24l01(void)
{
    u8 buffer[32];
    u8 temp;
    IWDG_Feed();
    
    if(newIdFlag==1)
    {
        newIdFlag = 0;
        txBuf[0] = 0xfc;
        txBuf[1] = 0x11;
        if(idBuf[0]==GetNum_1356[0] && idBuf[1]==GetNum_1356[1] && idBuf[2]==GetNum_1356[2] && idBuf[3]==GetNum_1356[3] && idBuf[4]==GetNum_1356[4])
        {
            txBuf[2] = idBuf[0];
            txBuf[3] = idBuf[1];
            txBuf[4] = idBuf[2];
            txBuf[5] = idBuf[3];
            txBuf[6] = idBuf[4];
        }
        
        txBuf[7] = CheckSum(txBuf,7);   //写号到txBuf
		SPI1_CE =0;
        NRF24L01_Write_Reg(SPI_REV,FLUSH_TX,0xff);//清除TX FIFO寄存器 
		NRF24L01_Write_Buf(SPI_REV,W_ACK_PAYLOAD,txBuf,8);   //写数据到TX BUF 8个字节，自动发送
		NRF24L01_RX_Mode(SPI_REV);      //设置24L01为接收模式
        rspFlag = 1;
        count=0;
        IWDG_Feed();
        
    }
    

    
    if(rspFlag) //处理答题卡上电发送的数据
    {
        IWDG_Feed();
        if(count<=2000) //1.5S内未收到2.4G回复信号，则rspFlag失效
        {
            if(SPI1_IRQ == 0)   //24L01收到答题卡上电发送的数据
            {
                IWDG_Feed();
                temp = NRF24L01_RxPacket(SPI_REV,buffer);
//                SendConsoleByte(temp);

                if((temp&0x40)==0x40)   //接收到的数据显示
                {
                    buffer[RX_PLOAD_WIDTH]='\0';
                    led0=~led0;

//                    SendConsoleData(buffer,RX_PLOAD_WIDTH); //

                    if((buffer[1]==idBuf[0])&& (buffer[2]==idBuf[1])&&(buffer[3]==idBuf[2])&&(buffer[4]==idBuf[3])&&(buffer[5]==idBuf[4]))    //收到改写后编号
                    {
                        if((buffer[1]==GetNum_1356[0])&& (buffer[2]==GetNum_1356[1])&&(buffer[3]==GetNum_1356[2])&&(buffer[4]==GetNum_1356[3])&&(buffer[5]==GetNum_1356[4]))
                        {
                            if(rspFlag == 1)
                            {
                                rspFlag++;
                                txBuf[1] = 0x12;             ///0xfc + cmd(0x12) +id +crc   ----cmd=0x12 确认
                                txBuf[7] = CheckSum(txBuf,7);   //写号到txBuf
                                NRF24L01_Write_Buf(SPI_REV,W_ACK_PAYLOAD,txBuf,8);   //写数据到TX BUF 8个字节，自动发送
                            }
                            else
                            {
                                rspFlag=0;  //如果ID已经是最新，rspFlag清零，不处理接收数据
                                count = 0;  //计数清零
                                idBuf[0]=0;
                                WrNumOkBuf->Control = 0x10; //写号完成
                                SendConsoleData(uartSendWrNumOkData,14);   //回显，返回给PC端
                                
                            }
                        }
                        
                        
                    }
                    else
                    {
                        SPI1_CE =0;
                        txBuf[1] = 0x11;
                        txBuf[7] = CheckSum(txBuf,7);   //写号到txBuf
                        NRF24L01_Write_Buf(SPI_REV,W_ACK_PAYLOAD,txBuf,8);   //写数据到TX BUF  8个字节
                        SPI1_CE =1;
                    }
                }
            }
//            if(rspFlag == 2)
//            {
//                
//            }
            
        }
        else if(count>2000)    //1.5s超时回复
        {
            IWDG_Feed();
            rspFlag=0;  //如果ID已经是最新，rspFlag清零，不处理接收数据
            count = 0;  //计数清零
            
            idBuf[0]=0;
            idBuf[1]=0;
            idBuf[2]=0;
            idBuf[3]=0;
            idBuf[4]=0;
            GetNum_1356[0]=0;
            GetNum_1356[1]=0;
            GetNum_1356[2]=0;
            GetNum_1356[3]=0;
            GetNum_1356[4]=0;
            
            WrNumOkBuf->Control = 0x30; //错误
            SendConsoleData(uartSendWrNumOkData,14);   //回显，返回给PC端
        }
        
    }
}

void DataProc_Nrf24l01_2(void)
{
    u8 buffer[32];
    u8 temp;
    IWDG_Feed();
    
    if(newIdFlag==1)
    {
        newIdFlag = 0;
        txBuf[0] = 0xfc;
        txBuf[1] = 0x11;
        if(idBuf[0]==GetNum_1356[0] && idBuf[1]==GetNum_1356[1] && idBuf[2]==GetNum_1356[2] && idBuf[3]==GetNum_1356[3] && idBuf[4]==GetNum_1356[4])
        {
            txBuf[2] = idBuf[0];
            txBuf[3] = idBuf[1];
            txBuf[4] = idBuf[2];
            txBuf[5] = idBuf[3];
            txBuf[6] = idBuf[4];
        }
        
        txBuf[7] = CheckSum(txBuf,7);   //写号到txBuf
		SPI1_CE =0;
        NRF24L01_Write_Reg(SPI_SEND,FLUSH_TX,0xff);//清除TX FIFO寄存器 
		NRF24L01_Write_Buf(SPI_SEND,W_ACK_PAYLOAD,txBuf,8);   //写数据到TX BUF 8个字节，自动发送
		NRF24L01_RX_Mode(SPI_SEND);      //设置24L01为接收模式
        rspFlag = 1;
        count=0;
        IWDG_Feed();
        
    }
    

    
    if(rspFlag) //处理答题卡上电发送的数据
    {
        IWDG_Feed();
        if(count<=2000) //1.5S内未收到2.4G回复信号，则rspFlag失效
        {
            if(SPI1_IRQ == 0)   //24L01收到答题卡上电发送的数据
            {
                IWDG_Feed();
                temp = NRF24L01_RxPacket(SPI_SEND,buffer);
//                SendConsoleByte(temp);

                if((temp&0x40)==0x40)   //接收到的数据显示
                {
                    buffer[RX_PLOAD_WIDTH]='\0';
                    led0=~led0;

//                    SendConsoleData(buffer,RX_PLOAD_WIDTH); //

                    if((buffer[1]==idBuf[0])&& (buffer[2]==idBuf[1])&&(buffer[3]==idBuf[2])&&(buffer[4]==idBuf[3])&&(buffer[5]==idBuf[4]))    //收到改写后编号
                    {
                        if((buffer[1]==GetNum_1356[0])&& (buffer[2]==GetNum_1356[1])&&(buffer[3]==GetNum_1356[2])&&(buffer[4]==GetNum_1356[3])&&(buffer[5]==GetNum_1356[4]))
                        {
                            if(rspFlag == 1)
                            {
                                rspFlag++;
                                txBuf[1] = 0x12;             ///0xfc + cmd(0x12) +id +crc   ----cmd=0x12 确认
                                txBuf[7] = CheckSum(txBuf,7);   //写号到txBuf
                                NRF24L01_Write_Buf(SPI_SEND,W_ACK_PAYLOAD,txBuf,8);   //写数据到TX BUF 8个字节，自动发送
                            }
                            else
                            {
                                rspFlag=0;  //如果ID已经是最新，rspFlag清零，不处理接收数据
                                count = 0;  //计数清零
                                idBuf[0]=0;
                                WrNumOkBuf->Control = 0x10; //写号完成
                                SendConsoleData(uartSendWrNumOkData,14);   //回显，返回给PC端
                                
                            }
                        }
                        
                        
                    }
                    else
                    {
                        SPI1_CE =0;
                        txBuf[1] = 0x11;
                        txBuf[7] = CheckSum(txBuf,7);   //写号到txBuf
                        NRF24L01_Write_Buf(SPI_SEND,W_ACK_PAYLOAD,txBuf,8);   //写数据到TX BUF  8个字节
                        SPI1_CE =1;
                    }
                }
            }
//            if(rspFlag == 2)
//            {
//                
//            }
            
        }
        else if(count>2000)    //1.5s超时回复
        {
            IWDG_Feed();
            rspFlag=0;  //如果ID已经是最新，rspFlag清零，不处理接收数据
            count = 0;  //计数清零
            
            idBuf[0]=0;
            idBuf[1]=0;
            idBuf[2]=0;
            idBuf[3]=0;
            idBuf[4]=0;
            GetNum_1356[0]=0;
            GetNum_1356[1]=0;
            GetNum_1356[2]=0;
            GetNum_1356[3]=0;
            GetNum_1356[4]=0;
            
            WrNumOkBuf->Control = 0x30; //错误
            SendConsoleData(uartSendWrNumOkData,14);   //回显，返回给PC端
        }
        
    }
}

void DataProc_1356(void)
{    
    IWDG_Feed();
    if(USART3_RX_CNT==10)
    {
        if(USART3_RX_BUF[0]==CheckData_1356[0] && USART3_RX_BUF[1]==CheckData_1356[1]
        && USART3_RX_BUF[8]==CheckData_1356[8] && USART3_RX_BUF[9]==CheckData_1356[9])
        {
            USART3_REC_Status |= 0x8000;    //串口3完成接收一帧数据
        }
    }
    
    if(USART3_REC_Status & 0x8000)
    {        
        //写号清零防止对下一次写号造成影响
        delay_ms(1);
        idBuf[1]=0;
        idBuf[2]=0;
        idBuf[3]=0;
        idBuf[4]=0;
        GetNum_1356[0]=0;
        GetNum_1356[1]=0;
        GetNum_1356[2]=0;
        GetNum_1356[3]=0;
        GetNum_1356[4]=0;
        
//        SendConsoleData(USART3_RX_BUF,10);

        IWDG_Feed();
        GetNum_1356[0]=0;
        GetNum_1356[1]=USART3_RX_BUF[4];
        GetNum_1356[2]=USART3_RX_BUF[5];
        GetNum_1356[3]=USART3_RX_BUF[6];
        GetNum_1356[4]=USART3_RX_BUF[7];  //串口收到的ID号写入到idBuf缓冲区
        
        recFlag = 1;      //接收2.4G考勤信号判断是否已经写过2.4G卡号
        
        led0 = ~led0;
        GPIO_SetBits(GPIOC,GPIO_Pin_9);   //打开红外扫描枪
        delay_ms(500);
        GPIO_ResetBits(GPIOC,GPIO_Pin_9);   //关闭红外扫描枪
        led0 = ~led0;
        
        IWDG_Feed();
//        newIdFlag = 1;  //写2.4G号
//        count = 0;      //计时器计数清零
        USART3_RX_CNT = 0;		   //
		USART3_REC_Status = 0;
    }
}


void DataProc_USART1(void)
{
    u16 reclen;
    
    IWDG_Feed();
    
    if(USART1_RX_CNT == 14 && USART1_RX_BUF[13]==0xFB)       //接收完一条指令标志
    {
        if(USART1_RX_BUF[0]==0xFA && USART1_RX_BUF[USART1_RX_CNT-1]==0xFB)//包头包尾检测
        {
            USART1_REC_Status |= 0x8000;
        }
        else
        {
            USART1_RX_CNT = 0;
            USART1_REC_Status = 0;  //接收长度正确但包头包尾检测没通过
        }
    }
    else if(USART1_RX_CNT == USART1_RX_BUF[12]+16)
    {
        if(USART1_RX_BUF[0]==0xFA && USART1_RX_BUF[USART1_RX_CNT-1]==0xFB)//包头包尾检测
        {
            USART1_REC_Status |= 0x8000;
        }
        else
        {
            USART1_RX_CNT = 0;
            USART1_REC_Status = 0;  //接收长度正确但包头包尾检测没通过
        }
    }
    else if(USART1_RX_CNT == 8) //更新固件
    {
        if(USART1_RX_BUF[0]=='u' && USART1_RX_BUF[1]=='p' && USART1_RX_BUF[2]=='d' && USART1_RX_BUF[3]=='a' && USART1_RX_BUF[4]=='t' && USART1_RX_BUF[5]=='e')//包头包尾检测
        {
            Flag_update = 1;
            USART1_REC_Status = 0;     //参数清零
            USART1_RX_CNT = 0;
            USART1_TX_CNT = 0;
            NVIC_SystemReset(); //系统复位
        }
    }
    else if(USART1_RX_CNT>100)    //接收长度错误
    {
        if(Flag_update==0)
        {
            reclen = USART1_RX_CNT;
            delay_ms(10);
            if(reclen == USART1_RX_CNT)
            {
                USART1_RX_CNT = 0;
                USART1_REC_Status = 0;          
            }
        }
    }
    

    if(USART1_REC_Status & 0x8000)                  //接收一帧数据完成
    {
        IWDG_Feed();
//        printf("Rec = %d\r\n",USART1_RX_CNT);
        ReceiveBuf->SequenceID[0] = USART1_RX_BUF[6];
        ReceiveBuf->SequenceID[1] = USART1_RX_BUF[7];
        ReceiveBuf->SequenceID[2] = USART1_RX_BUF[8];
        ReceiveBuf->SequenceID[3] = USART1_RX_BUF[9];
        
        ReceiveBuf->Command = USART1_RX_BUF[10];    //命令
//        ReceiveBuf->Tag     = USART1_RX_BUF[11];    //Tag 命令类型
//        ReceiveBuf->Length  = USART1_RX_BUF[12];    //数据长度        
        
        SendBuf->SequenceID[0] = ReceiveBuf->SequenceID[0];
        SendBuf->SequenceID[1] = ReceiveBuf->SequenceID[1];
        SendBuf->SequenceID[2] = ReceiveBuf->SequenceID[2];
        SendBuf->SequenceID[3] = ReceiveBuf->SequenceID[3]; //SequenceID与请求方同步
        
        switch(ReceiveBuf->Command)
        {
            case Reset:                     //复位
//                printf("Reset\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;
                
            case GetFirmversion:            //获取固件版本
//                printf("GetFirmversion\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;
                
            case SetReaderID:            //设置阅读器ID
//                printf("SetReaderID\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;
                
            case GetReaderID:            //获取阅读器ID
//                printf("GetReaderID\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case DelWhite:            //删除白名单
//                printf("DelWhite\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case OpenWhite:            //打开白名单
//                printf("OpenWhite\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case CloseWhite:            //关闭白名单
//                printf("CloseWhite\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case AddWhite:            //添加白名单
//                printf("AddWhite\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case GetWhite:            //获取白名单
//                printf("GetWhite\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case Heart:            //心跳
//                printf("Heart\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case GetWhiteState:            //获取白名单状态
//                printf("GetWhiteState\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case WriteNum:            //写号 A0
//                printf("Write 2.4G Num\n");
                //数据处理
                WrNumOkBuf->SequenceID[0] = ReceiveBuf->SequenceID[0];
                WrNumOkBuf->SequenceID[1] = ReceiveBuf->SequenceID[1];
                WrNumOkBuf->SequenceID[2] = ReceiveBuf->SequenceID[2];
                WrNumOkBuf->SequenceID[3] = ReceiveBuf->SequenceID[3]; //SequenceID与请求方同步
                
                WrNumOkBuf->Command = ReceiveBuf->Command;
                
                ReceiveBuf->Value[0] = USART1_RX_BUF[13];
                ReceiveBuf->Value[1] = USART1_RX_BUF[14];
                ReceiveBuf->Value[2] = USART1_RX_BUF[15];
                ReceiveBuf->Value[3] = USART1_RX_BUF[16];
                ReceiveBuf->Value[4] = USART1_RX_BUF[17];
                
                if(USART1_RX_BUF[13]==GetNum_1356[0] && USART1_RX_BUF[14]==GetNum_1356[1] && USART1_RX_BUF[15]==GetNum_1356[2] && USART1_RX_BUF[16]==GetNum_1356[3] && USART1_RX_BUF[17]==GetNum_1356[4])
                {
                    idBuf[0] = USART1_RX_BUF[13];
                    idBuf[1] = USART1_RX_BUF[14];
                    idBuf[2] = USART1_RX_BUF[15];
                    idBuf[3] = USART1_RX_BUF[16];
                    idBuf[4] = USART1_RX_BUF[17];
        
                    newIdFlag = 1;  //2.4G写号
                    count = 0;      //计时器计数清零
                }
                else
                {
                     WrNumOkBuf->Control = 0x30; //错误
                    SendConsoleData(uartSendWrNumOkData,14);   //回显，返回给PC端
       
                }
                    
                
                ReceiveBuf->Command = 0;    //清零

                break;

            case WriteNumState:             //写号结果上报
    //            printf("WriteNumState\n");
                //数据处理
//                if(ReceiveBuf->Tag == 0x01)
//                {
//                    if(wrNumFlag == 1)
//                    {
//                        SendBuf->Command  = 0xA1;
//                        SendBuf->Tag      = 1;
//                        SendBuf->Length   = 6;
//                        
//                        SendBuf->Value[0] = idBuf[0];
//                        SendBuf->Value[1] = idBuf[1];
//                        SendBuf->Value[2] = idBuf[2];
//                        SendBuf->Value[3] = idBuf[3];
//                        SendBuf->Value[4] = idBuf[4];
//                        SendBuf->Value[5] = 1;
//                        
//                        SendConsoleData(uartSendData,22);   //回显，返回给PC端
//                        
//                    }
//                    else
//                    {
//                        SendBuf->Command  = 0xA1;
//                        SendBuf->Tag      = 1;
//                        SendBuf->Length   = 6;
//                        
//                        SendBuf->Value[0] = idBuf[0];
//                        SendBuf->Value[1] = idBuf[1];
//                        SendBuf->Value[2] = idBuf[2];
//                        SendBuf->Value[3] = idBuf[3];
//                        SendBuf->Value[4] = idBuf[4];
//                        SendBuf->Value[5] = 0;
//                        
//                        SendConsoleData(uartSendData,22);   //回显，返回给PC端
//                        
//                    }    
//                }


                ReceiveBuf->Command = 0;    //清零
                break;

            case ReadNum:            //读号 A2

                    ReadNumBuf->SequenceID[0] = ReceiveBuf->SequenceID[0];
                    ReadNumBuf->SequenceID[1] = ReceiveBuf->SequenceID[1];
                    ReadNumBuf->SequenceID[2] = ReceiveBuf->SequenceID[2];
                    ReadNumBuf->SequenceID[3] = ReceiveBuf->SequenceID[3]; //SequenceID与请求方同步
                    
                    ReadNumBuf->Command  = ReceiveBuf->Command;
                    ReadNumBuf->Tag1      = 1;
                    ReadNumBuf->Length1   = 6;
                    ReadNumBuf->Value1[0] = 0;      //len=0 无效
                    ReadNumBuf->Value1[1] = GetNum_1356[0];
                    ReadNumBuf->Value1[2] = GetNum_1356[1];
                    ReadNumBuf->Value1[3] = GetNum_1356[2];
                    ReadNumBuf->Value1[4] = GetNum_1356[3];
                    ReadNumBuf->Value1[5] = GetNum_1356[4];
                    
                    ReadNumBuf->Tag2      = 2;
                    ReadNumBuf->Length2   = 6;
                    ReadNumBuf->Value2[0] = 0;      //len=0 无效
                    ReadNumBuf->Value2[1] = idBuf[0];
                    ReadNumBuf->Value2[2] = idBuf[1];
                    ReadNumBuf->Value2[3] = idBuf[2];
                    ReadNumBuf->Value2[4] = idBuf[3];
                    ReadNumBuf->Value2[5] = idBuf[4];
                    
                    SendConsoleData(uartSendRdNumData,30);   //回显，返回给PC端
                    //写号清零防止对下一次写号造成影响
//                    idBuf[1]=0;
//                    idBuf[2]=0;
//                    idBuf[3]=0;
//                    idBuf[4]=0;
//                    GetNum_1356[0]=0;
//                    GetNum_1356[1]=0;
//                    GetNum_1356[2]=0;
//                    GetNum_1356[3]=0;
//                    GetNum_1356[4]=0;
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case UpdateFirmversion:            //升级固件
                printf("UpdateFirmversion\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case UpdateFirm:            //升级固件开关
                printf("UpdateFirm\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            case UpdateFirmState:            //升级固件结果上报
                printf("UpdateFirmState\n");
                //数据处理
                
                ReceiveBuf->Command = 0;    //清零
                break;

            default:
                break;
        }


        
        USART1_REC_Status = 0;                      //参数清零
        USART1_RX_CNT = 0;
        USART1_TX_CNT = 0;

    }
    
}

 //------------------------------------------------------------------
//函数名：void TIM3_IRQHandler(void)
//输入参数：null
//返回参数：null
//说明：定时器3中断服务
//------------------------------------------------------------------
void TIM3_IRQHandler(void)
{ 
  if(TIM_GetITStatus(TIM3,TIM_IT_Update)!=RESET)        //判断是否为定时器3溢出中断
  {
	 TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清中断标记
	 count ++;
	 flag_1ms = 1;
  }
}
