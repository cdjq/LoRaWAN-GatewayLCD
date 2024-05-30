#ifndef __GatewayUI_https_H__
#define __GatewayUI_https_H__

#include <iostream>
using namespace std;

#include <list>

void My_httpRequest_GetToken(void);
int cjsonParse_getTokenStatus(const char *response, std::string& TokenStatus);
std::string My_httpRequest_NodeRegistration();

void My_httpRequest_getNodeState(const char* devEUI);
void My_httpRequest_putNodeState(const char* devEUI ,bool isDisabled);
int cjsonParse_getNodeDevINFO(const char *response);
std::string send_httpRequest(const std::string url, int method, const std::string& postfields = "");

// 解析获取ETH网络信息
int cjsonParse_getEthINFO(const char *response, std::string& eth_IP, std::string& eth_Mask, std::string& eth_DHCP);
// 解析获取WIFI网络信息
int cjsonParse_getWifiINFO(const char *response, std::string& wifi_Mode, std::string& wifi_IP, std::string& wifi_Mask);
// 解析获取蜂窝网络信息
int cjsonParse_getCat1INFO(const char *response,  std::string& Cat1_IP, std::string& Cat1_SIM, std::string& Cat1_Mode, std::string& Cat1_SNR);

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef __cplusplus
}
#endif

#endif



