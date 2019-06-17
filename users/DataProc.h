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
    Reset = 0x21,           //阅读器复位
    GetFirmversion = 0x22,  //获取固件版本
    
    SetReaderID = 0x25,     //设置阅读器ID
    GetReaderID = 0x26,     //获取阅读器ID
    
    DelWhite = 0x71,        //删除白名单
    OpenWhite = 0x73,       //打开白名单
    CloseWhite = 0x74,      //关闭白名单
    AddWhite = 0x75,        //添加白名单
    GetWhite = 0x78,        //获取白名单
    
    Heart = 0x82,           //心跳包
    
    GetWhiteState = 0x94    //获取白名单状态
};

enum
{
    WriteNum = 0xA0,    //写号
    WriteNumState,      //写号结果上报
    ReadNum,            //读卡号
    UpdateFirmversion,  //下载升级固件
    UpdateFirm,         //升级固件开关
    UpdateFirmState,    //升级结果上报
    
};

#endif
