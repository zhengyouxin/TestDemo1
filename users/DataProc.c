#include "DataProc.h"
#include <string.h>

//SPIType	SPI_Using = SPI_PORT1;	//ѡ��һ��SPI�ӿ�

extern void IWDG_Feed(void);

u8 CheckData_1356[10] = {0xD1,0xD1,0xE0,0xD0,0x00,0x00,0x00,0x00,0xD2,0xD2}; //13.56M��������
u8 GetNum_1356[5] = {0x00,0x00,0x00,0x00,0x00}; //13.56M����

u8 uartSendData[100]; //����100���ֽ�
u8 uartRecData[100]; //����100���ֽ�

u8 uartSendRdNumData[100];
u8 uartSendWrNumOkData[100];

u16 count;
u8 flag_1ms;

u8 Flag_update;

struct Protocol_send_data   //ͨѶЭ�� ����
{
    u8 HeadFix;
    u8 Control;
    u8 Magic[4];
    u8 SequenceID[4];
    u8 Command;
    u8 Tag;
    u8 Length;
    u8 Value[6];            //���ֵ��6
    u8 FootCRC[2];          //CRC ��ʱ�̶�Ϊ00 00
    u8 FootFix;             //�̶�Ϊ0xFB
};

struct Protocol_rec_data   //ͨѶЭ�� ����
{
    u8 HeadFix;
    u8 Control;
    u8 Magic[4];
    u8 SequenceID[4];
    u8 Command;
    u8 Tag;
    u8 Length;
    u8 Value[62];           //���ֵ��62
    u8 FootCRC[2];          //CRC ��ʱ�̶�Ϊ00 00
    u8 FootFix;             //�̶�Ϊ0xFB
};

struct ReadNum_send_data   //���Żظ�ָ��
{
    u8 HeadFix;
    u8 Control;
    u8 Magic[4];
    u8 SequenceID[4];
    u8 Command;
    u8 Tag1;
    u8 Length1;
    u8 Value1[6];            //���ֵ��6
    u8 Tag2;
    u8 Length2;
    u8 Value2[6];            //���ֵ��6
    u8 FootCRC[2];          //CRC ��ʱ�̶�Ϊ00 00
    u8 FootFix;             //�̶�Ϊ0xFB
};

struct WrNumOk_send_data   //д���ɹ��ظ�ָ��
{
    u8 HeadFix;
    u8 Control;
    u8 Magic[4];
    u8 SequenceID[4];
    u8 Command;
    u8 FootCRC[2];          //CRC ��ʱ�̶�Ϊ00 00
    u8 FootFix;             //�̶�Ϊ0xFB
};




#define ReceiveBuf ((struct Protocol_rec_data *)&uartRecData[0])
#define SendBuf ((struct Protocol_send_data *)&uartSendData[0])

#define WrNumOkBuf ((struct WrNumOk_send_data *)&uartSendWrNumOkData[0])
#define ReadNumBuf ((struct ReadNum_send_data *)&uartSendRdNumData[0])

#define	CndRspBufferSize  100

unsigned char CmdRspBuffer[CndRspBufferSize];
unsigned char CmdRspBufferWr_Rx = 0;
u8 idBuf[5];    //ID����ռ�
u8 newIdFlag;   //д2.4G ID��־
u8 recFlag;

unsigned char txBuf[32]="123456789123456789";
unsigned char rspFlag = 0;  //�Ƿ�����⿨�ϵ�2.4G��Ϣ ��־
unsigned char wrNumFlag = 0;//д���Ƿ���ɱ�־

/****************************************************
����: У���

����: sum = ~(data0+...+datan)+1;
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


void Data_Init(void)    //�����ݳ�ʼ��
{
    SendBuf->HeadFix = 0xFA;        //���ͻ�������ʼ��
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
    
    ReceiveBuf->HeadFix = 0xFA;         //���ջ�������ʼ��
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
    
    ReadNumBuf->HeadFix = 0xFA;         //���ջ�������ʼ��
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
    
    WrNumOkBuf->HeadFix = 0xFA;         //���ջ�������ʼ��
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
//        USART1_REC_Status |= 0x8000;    //�յ�ID�������ʮ����10λID
//    }
//    
//    
//    if(USART1_REC_Status & 0x8000)
//    {
////        memcpy(idBuf,&USART1_RX_BUF[3],5);  //��5λʮ����ID��д��idBuf
//        idBuf[0]=0;
//        idBuf[1]=USART1_RX_BUF[4];
//        idBuf[2]=USART1_RX_BUF[5];
//        idBuf[3]=USART1_RX_BUF[6];
//        idBuf[4]=USART1_RX_BUF[7];  //�����յ���ID��д�뵽idBuf������
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
    
    if(recFlag==1)  //�ж�IC�Ƿ��Ѿ�д��
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
                recFlag = 2;    //��д2.4G����
//                newIdFlag = 0;  //��д2.4G����
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
        
        txBuf[7] = CheckSum(txBuf,7);   //д�ŵ�txBuf
		SPI1_CE =0;
        NRF24L01_Write_Reg(SPI_REV,FLUSH_TX,0xff);//���TX FIFO�Ĵ��� 
		NRF24L01_Write_Buf(SPI_REV,W_ACK_PAYLOAD,txBuf,8);   //д���ݵ�TX BUF 8���ֽڣ��Զ�����
		NRF24L01_RX_Mode(SPI_REV);      //����24L01Ϊ����ģʽ
        rspFlag = 1;
        count=0;
        IWDG_Feed();
        
    }
    

    
    if(rspFlag) //������⿨�ϵ緢�͵�����
    {
        IWDG_Feed();
        if(count<=2000) //1.5S��δ�յ�2.4G�ظ��źţ���rspFlagʧЧ
        {
            if(SPI1_IRQ == 0)   //24L01�յ����⿨�ϵ緢�͵�����
            {
                IWDG_Feed();
                temp = NRF24L01_RxPacket(SPI_REV,buffer);
//                SendConsoleByte(temp);

                if((temp&0x40)==0x40)   //���յ���������ʾ
                {
                    buffer[RX_PLOAD_WIDTH]='\0';
                    led0=~led0;

//                    SendConsoleData(buffer,RX_PLOAD_WIDTH); //

                    if((buffer[1]==idBuf[0])&& (buffer[2]==idBuf[1])&&(buffer[3]==idBuf[2])&&(buffer[4]==idBuf[3])&&(buffer[5]==idBuf[4]))    //�յ���д����
                    {
                        if((buffer[1]==GetNum_1356[0])&& (buffer[2]==GetNum_1356[1])&&(buffer[3]==GetNum_1356[2])&&(buffer[4]==GetNum_1356[3])&&(buffer[5]==GetNum_1356[4]))
                        {
                            if(rspFlag == 1)
                            {
                                rspFlag++;
                                txBuf[1] = 0x12;             ///0xfc + cmd(0x12) +id +crc   ----cmd=0x12 ȷ��
                                txBuf[7] = CheckSum(txBuf,7);   //д�ŵ�txBuf
                                NRF24L01_Write_Buf(SPI_REV,W_ACK_PAYLOAD,txBuf,8);   //д���ݵ�TX BUF 8���ֽڣ��Զ�����
                            }
                            else
                            {
                                rspFlag=0;  //���ID�Ѿ������£�rspFlag���㣬�������������
                                count = 0;  //��������
                                idBuf[0]=0;
                                WrNumOkBuf->Control = 0x10; //д�����
                                SendConsoleData(uartSendWrNumOkData,14);   //���ԣ����ظ�PC��
                                
                            }
                        }
                        
                        
                    }
                    else
                    {
                        SPI1_CE =0;
                        txBuf[1] = 0x11;
                        txBuf[7] = CheckSum(txBuf,7);   //д�ŵ�txBuf
                        NRF24L01_Write_Buf(SPI_REV,W_ACK_PAYLOAD,txBuf,8);   //д���ݵ�TX BUF  8���ֽ�
                        SPI1_CE =1;
                    }
                }
            }
//            if(rspFlag == 2)
//            {
//                
//            }
            
        }
        else if(count>2000)    //1.5s��ʱ�ظ�
        {
            IWDG_Feed();
            rspFlag=0;  //���ID�Ѿ������£�rspFlag���㣬�������������
            count = 0;  //��������
            
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
            
            WrNumOkBuf->Control = 0x30; //����
            SendConsoleData(uartSendWrNumOkData,14);   //���ԣ����ظ�PC��
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
        
        txBuf[7] = CheckSum(txBuf,7);   //д�ŵ�txBuf
		SPI1_CE =0;
        NRF24L01_Write_Reg(SPI_SEND,FLUSH_TX,0xff);//���TX FIFO�Ĵ��� 
		NRF24L01_Write_Buf(SPI_SEND,W_ACK_PAYLOAD,txBuf,8);   //д���ݵ�TX BUF 8���ֽڣ��Զ�����
		NRF24L01_RX_Mode(SPI_SEND);      //����24L01Ϊ����ģʽ
        rspFlag = 1;
        count=0;
        IWDG_Feed();
        
    }
    

    
    if(rspFlag) //������⿨�ϵ緢�͵�����
    {
        IWDG_Feed();
        if(count<=2000) //1.5S��δ�յ�2.4G�ظ��źţ���rspFlagʧЧ
        {
            if(SPI1_IRQ == 0)   //24L01�յ����⿨�ϵ緢�͵�����
            {
                IWDG_Feed();
                temp = NRF24L01_RxPacket(SPI_SEND,buffer);
//                SendConsoleByte(temp);

                if((temp&0x40)==0x40)   //���յ���������ʾ
                {
                    buffer[RX_PLOAD_WIDTH]='\0';
                    led0=~led0;

//                    SendConsoleData(buffer,RX_PLOAD_WIDTH); //

                    if((buffer[1]==idBuf[0])&& (buffer[2]==idBuf[1])&&(buffer[3]==idBuf[2])&&(buffer[4]==idBuf[3])&&(buffer[5]==idBuf[4]))    //�յ���д����
                    {
                        if((buffer[1]==GetNum_1356[0])&& (buffer[2]==GetNum_1356[1])&&(buffer[3]==GetNum_1356[2])&&(buffer[4]==GetNum_1356[3])&&(buffer[5]==GetNum_1356[4]))
                        {
                            if(rspFlag == 1)
                            {
                                rspFlag++;
                                txBuf[1] = 0x12;             ///0xfc + cmd(0x12) +id +crc   ----cmd=0x12 ȷ��
                                txBuf[7] = CheckSum(txBuf,7);   //д�ŵ�txBuf
                                NRF24L01_Write_Buf(SPI_SEND,W_ACK_PAYLOAD,txBuf,8);   //д���ݵ�TX BUF 8���ֽڣ��Զ�����
                            }
                            else
                            {
                                rspFlag=0;  //���ID�Ѿ������£�rspFlag���㣬�������������
                                count = 0;  //��������
                                idBuf[0]=0;
                                WrNumOkBuf->Control = 0x10; //д�����
                                SendConsoleData(uartSendWrNumOkData,14);   //���ԣ����ظ�PC��
                                
                            }
                        }
                        
                        
                    }
                    else
                    {
                        SPI1_CE =0;
                        txBuf[1] = 0x11;
                        txBuf[7] = CheckSum(txBuf,7);   //д�ŵ�txBuf
                        NRF24L01_Write_Buf(SPI_SEND,W_ACK_PAYLOAD,txBuf,8);   //д���ݵ�TX BUF  8���ֽ�
                        SPI1_CE =1;
                    }
                }
            }
//            if(rspFlag == 2)
//            {
//                
//            }
            
        }
        else if(count>2000)    //1.5s��ʱ�ظ�
        {
            IWDG_Feed();
            rspFlag=0;  //���ID�Ѿ������£�rspFlag���㣬�������������
            count = 0;  //��������
            
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
            
            WrNumOkBuf->Control = 0x30; //����
            SendConsoleData(uartSendWrNumOkData,14);   //���ԣ����ظ�PC��
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
            USART3_REC_Status |= 0x8000;    //����3��ɽ���һ֡����
        }
    }
    
    if(USART3_REC_Status & 0x8000)
    {        
        //д�������ֹ����һ��д�����Ӱ��
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
        GetNum_1356[4]=USART3_RX_BUF[7];  //�����յ���ID��д�뵽idBuf������
        
        recFlag = 1;      //����2.4G�����ź��ж��Ƿ��Ѿ�д��2.4G����
        
        led0 = ~led0;
        GPIO_SetBits(GPIOC,GPIO_Pin_9);   //�򿪺���ɨ��ǹ
        delay_ms(500);
        GPIO_ResetBits(GPIOC,GPIO_Pin_9);   //�رպ���ɨ��ǹ
        led0 = ~led0;
        
        IWDG_Feed();
//        newIdFlag = 1;  //д2.4G��
//        count = 0;      //��ʱ����������
        USART3_RX_CNT = 0;		   //
		USART3_REC_Status = 0;
    }
}


void DataProc_USART1(void)
{
    u16 reclen;
    
    IWDG_Feed();
    
    if(USART1_RX_CNT == 14 && USART1_RX_BUF[13]==0xFB)       //������һ��ָ���־
    {
        if(USART1_RX_BUF[0]==0xFA && USART1_RX_BUF[USART1_RX_CNT-1]==0xFB)//��ͷ��β���
        {
            USART1_REC_Status |= 0x8000;
        }
        else
        {
            USART1_RX_CNT = 0;
            USART1_REC_Status = 0;  //���ճ�����ȷ����ͷ��β���ûͨ��
        }
    }
    else if(USART1_RX_CNT == USART1_RX_BUF[12]+16)
    {
        if(USART1_RX_BUF[0]==0xFA && USART1_RX_BUF[USART1_RX_CNT-1]==0xFB)//��ͷ��β���
        {
            USART1_REC_Status |= 0x8000;
        }
        else
        {
            USART1_RX_CNT = 0;
            USART1_REC_Status = 0;  //���ճ�����ȷ����ͷ��β���ûͨ��
        }
    }
    else if(USART1_RX_CNT == 8) //���¹̼�
    {
        if(USART1_RX_BUF[0]=='u' && USART1_RX_BUF[1]=='p' && USART1_RX_BUF[2]=='d' && USART1_RX_BUF[3]=='a' && USART1_RX_BUF[4]=='t' && USART1_RX_BUF[5]=='e')//��ͷ��β���
        {
            Flag_update = 1;
            USART1_REC_Status = 0;     //��������
            USART1_RX_CNT = 0;
            USART1_TX_CNT = 0;
            NVIC_SystemReset(); //ϵͳ��λ
        }
    }
    else if(USART1_RX_CNT>100)    //���ճ��ȴ���
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
    

    if(USART1_REC_Status & 0x8000)                  //����һ֡�������
    {
        IWDG_Feed();
//        printf("Rec = %d\r\n",USART1_RX_CNT);
        ReceiveBuf->SequenceID[0] = USART1_RX_BUF[6];
        ReceiveBuf->SequenceID[1] = USART1_RX_BUF[7];
        ReceiveBuf->SequenceID[2] = USART1_RX_BUF[8];
        ReceiveBuf->SequenceID[3] = USART1_RX_BUF[9];
        
        ReceiveBuf->Command = USART1_RX_BUF[10];    //����
//        ReceiveBuf->Tag     = USART1_RX_BUF[11];    //Tag ��������
//        ReceiveBuf->Length  = USART1_RX_BUF[12];    //���ݳ���        
        
        SendBuf->SequenceID[0] = ReceiveBuf->SequenceID[0];
        SendBuf->SequenceID[1] = ReceiveBuf->SequenceID[1];
        SendBuf->SequenceID[2] = ReceiveBuf->SequenceID[2];
        SendBuf->SequenceID[3] = ReceiveBuf->SequenceID[3]; //SequenceID������ͬ��
        
        switch(ReceiveBuf->Command)
        {
            case Reset:                     //��λ
//                printf("Reset\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;
                
            case GetFirmversion:            //��ȡ�̼��汾
//                printf("GetFirmversion\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;
                
            case SetReaderID:            //�����Ķ���ID
//                printf("SetReaderID\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;
                
            case GetReaderID:            //��ȡ�Ķ���ID
//                printf("GetReaderID\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case DelWhite:            //ɾ��������
//                printf("DelWhite\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case OpenWhite:            //�򿪰�����
//                printf("OpenWhite\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case CloseWhite:            //�رհ�����
//                printf("CloseWhite\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case AddWhite:            //��Ӱ�����
//                printf("AddWhite\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case GetWhite:            //��ȡ������
//                printf("GetWhite\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case Heart:            //����
//                printf("Heart\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case GetWhiteState:            //��ȡ������״̬
//                printf("GetWhiteState\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case WriteNum:            //д�� A0
//                printf("Write 2.4G Num\n");
                //���ݴ���
                WrNumOkBuf->SequenceID[0] = ReceiveBuf->SequenceID[0];
                WrNumOkBuf->SequenceID[1] = ReceiveBuf->SequenceID[1];
                WrNumOkBuf->SequenceID[2] = ReceiveBuf->SequenceID[2];
                WrNumOkBuf->SequenceID[3] = ReceiveBuf->SequenceID[3]; //SequenceID������ͬ��
                
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
        
                    newIdFlag = 1;  //2.4Gд��
                    count = 0;      //��ʱ����������
                }
                else
                {
                     WrNumOkBuf->Control = 0x30; //����
                    SendConsoleData(uartSendWrNumOkData,14);   //���ԣ����ظ�PC��
       
                }
                    
                
                ReceiveBuf->Command = 0;    //����

                break;

            case WriteNumState:             //д�Ž���ϱ�
    //            printf("WriteNumState\n");
                //���ݴ���
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
//                        SendConsoleData(uartSendData,22);   //���ԣ����ظ�PC��
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
//                        SendConsoleData(uartSendData,22);   //���ԣ����ظ�PC��
//                        
//                    }    
//                }


                ReceiveBuf->Command = 0;    //����
                break;

            case ReadNum:            //���� A2

                    ReadNumBuf->SequenceID[0] = ReceiveBuf->SequenceID[0];
                    ReadNumBuf->SequenceID[1] = ReceiveBuf->SequenceID[1];
                    ReadNumBuf->SequenceID[2] = ReceiveBuf->SequenceID[2];
                    ReadNumBuf->SequenceID[3] = ReceiveBuf->SequenceID[3]; //SequenceID������ͬ��
                    
                    ReadNumBuf->Command  = ReceiveBuf->Command;
                    ReadNumBuf->Tag1      = 1;
                    ReadNumBuf->Length1   = 6;
                    ReadNumBuf->Value1[0] = 0;      //len=0 ��Ч
                    ReadNumBuf->Value1[1] = GetNum_1356[0];
                    ReadNumBuf->Value1[2] = GetNum_1356[1];
                    ReadNumBuf->Value1[3] = GetNum_1356[2];
                    ReadNumBuf->Value1[4] = GetNum_1356[3];
                    ReadNumBuf->Value1[5] = GetNum_1356[4];
                    
                    ReadNumBuf->Tag2      = 2;
                    ReadNumBuf->Length2   = 6;
                    ReadNumBuf->Value2[0] = 0;      //len=0 ��Ч
                    ReadNumBuf->Value2[1] = idBuf[0];
                    ReadNumBuf->Value2[2] = idBuf[1];
                    ReadNumBuf->Value2[3] = idBuf[2];
                    ReadNumBuf->Value2[4] = idBuf[3];
                    ReadNumBuf->Value2[5] = idBuf[4];
                    
                    SendConsoleData(uartSendRdNumData,30);   //���ԣ����ظ�PC��
                    //д�������ֹ����һ��д�����Ӱ��
//                    idBuf[1]=0;
//                    idBuf[2]=0;
//                    idBuf[3]=0;
//                    idBuf[4]=0;
//                    GetNum_1356[0]=0;
//                    GetNum_1356[1]=0;
//                    GetNum_1356[2]=0;
//                    GetNum_1356[3]=0;
//                    GetNum_1356[4]=0;
                
                ReceiveBuf->Command = 0;    //����
                break;

            case UpdateFirmversion:            //�����̼�
                printf("UpdateFirmversion\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case UpdateFirm:            //�����̼�����
                printf("UpdateFirm\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            case UpdateFirmState:            //�����̼�����ϱ�
                printf("UpdateFirmState\n");
                //���ݴ���
                
                ReceiveBuf->Command = 0;    //����
                break;

            default:
                break;
        }


        
        USART1_REC_Status = 0;                      //��������
        USART1_RX_CNT = 0;
        USART1_TX_CNT = 0;

    }
    
}

 //------------------------------------------------------------------
//��������void TIM3_IRQHandler(void)
//���������null
//���ز�����null
//˵������ʱ��3�жϷ���
//------------------------------------------------------------------
void TIM3_IRQHandler(void)
{ 
  if(TIM_GetITStatus(TIM3,TIM_IT_Update)!=RESET)        //�ж��Ƿ�Ϊ��ʱ��3����ж�
  {
	 TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //���жϱ��
	 count ++;
	 flag_1ms = 1;
  }
}
