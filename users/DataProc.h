#ifndef _DATAPROC_H
#define _DATAPROC_H
#include "stm32f10x.h"
#include "nrf24L01.h"
#include "delay.h"
#include "printf.h"
#include "led.h"
#include "USART2.h"
#include "stmflash.h"


void DataProc_Nrf24l01(void);
void DataProc_Nrf24l01_2(void);
void DataProc_RecNrf24L01(void);
void Usart1_Process(void);

void DataProc_1356(void);
void DataProc_USART3(void);
void Data_Init(void);
void DataProc_USART1(void);

#define TestFlashAddr 0x80020000

enum
{
    Reset = 0x21,           //�Ķ�����λ
    GetFirmversion = 0x22,  //��ȡ�̼��汾
    
    SetReaderID = 0x25,     //�����Ķ���ID
    GetReaderID = 0x26,     //��ȡ�Ķ���ID
    
    DelWhite = 0x71,        //ɾ��������
    OpenWhite = 0x73,       //�򿪰�����
    CloseWhite = 0x74,      //�رհ�����
    AddWhite = 0x75,        //��Ӱ�����
    GetWhite = 0x78,        //��ȡ������
    
    Heart = 0x82,           //������
    
    GetWhiteState = 0x94    //��ȡ������״̬
};

enum
{
    WriteNum = 0xA0,    //д��
    WriteNumState,      //д�Ž���ϱ�
    ReadNum,            //������
    UpdateFirmversion,  //���������̼�
    UpdateFirm,         //�����̼�����
    UpdateFirmState,    //��������ϱ�
    
};

#endif
