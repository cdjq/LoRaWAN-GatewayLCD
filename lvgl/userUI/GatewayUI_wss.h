#ifndef __GatewayUI_wss_H__
#define __GatewayUI_wss_H__

#include <iostream>
using namespace std;

int My_wssRequest_RTD(void);
int My_wssRequest_NFC(void);
void NFC_sendJsonData(const char *json);

int Get_NFC_updateStatus(void);

//其中涉及原子操作的NFC读写模式切换
void NFC_JsonDatAassembly(int FactoryResetValue);
void NFC_sendJsonData_Read(void);


#ifdef __cplusplus
extern "C"
{
#endif


#ifdef __cplusplus
}
#endif

#endif



