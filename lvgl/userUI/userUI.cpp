
#include "userUI.h"
#include "GatewayUI_DS.h"
#include "GatewayUI_https.h"
#include "GatewayUI_wss.h"

#include <iostream>
#include <regex>
#include <string>

#include <cmath>
#include "log.h"

#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <cstdio>
#include <mosquitto.h>

#include "stacktrace.h"

#include <chrono>
#include <ctime>
#include <iomanip>


#include <tuple>

// 返回图标
LV_IMG_DECLARE(Return_png);

// 注册成功
LV_IMG_DECLARE(RegisterTrue);
LV_IMG_DECLARE(English_RegisterTrue_PNG);
// 注册失败
LV_IMG_DECLARE(RegisterFlase);
LV_IMG_DECLARE(English_RegisterFlase_PNG);
// 注册失败（设备已存在）
LV_IMG_DECLARE(RegisterFlase_IsExist);
LV_IMG_DECLARE(English_RegisterFlase_PNG);
// 等待NFC靠近（更新、恢复出厂设置）
LV_IMG_DECLARE(Updating);
LV_IMG_DECLARE(English_Configuring_PNG);

// 更新成功
LV_IMG_DECLARE(UpdateSucceed_PNG);
LV_IMG_DECLARE(English_UpdateTrue_PNG);
// 更新失败
LV_IMG_DECLARE(updateFlase);
LV_IMG_DECLARE(English_UpdateFalse_PNG);


// 是否恢复出厂设置的界面
LV_IMG_DECLARE(FactoryReset_PNG);
LV_IMG_DECLARE(English_Confirm_PNG);
// 恢复出厂设置成功
LV_IMG_DECLARE(FactoryReset_True);
LV_IMG_DECLARE(English_ResetTrue_PNG);
// 恢复出厂设置失败
LV_IMG_DECLARE(FactoryReset_Flase);
LV_IMG_DECLARE(English_ResetFalse_PNG);

// 等待超时（更新、恢复出厂设置）
LV_IMG_DECLARE(WartingTimeout);
LV_IMG_DECLARE(English_TimeOut_PNG);


#define Signal_RSSI_A -30
#define Signal_RSSI_B -60
#define Signal_RSSI_C -90


#define JoinRequest "  "
#define JoinAccept "   "
#define ConfirmedDataUp "    "
#define ConfirmedDataDown "     "
#define UnconfirmedDataUp "      "
#define UnconfirmedDataDown "       "

// 定义节点信息表格行数和列数
#define NodeTable_RowsNum 4  //4行
#define NodeTable_ColsNum 5  //5列


#define RealTimeData_MaxNum 3  //实时数据流显示的最大行数


#define Node_left_rowNum 6
#define Node_right_rowNum 5
#define Node_Line_spacing 42

#define NFC_left_rowNum 4
#define NFC_right_rowNum 5
#define NFC_left_LineSpacing 45
#define NFC_right_LineSpacing 40

/**********************************************************************************
 *        数据段/BSS段——全局变量声明
 *（注意：函数中的static静态变量也存放在这个区域）
 * 1.普通全局变量
 * 2.LVGL对象的全局变量
 **********************************************************************************/
int  NodeTable_currentPage_SG = 1;   // 当前节点信息表格页数
int NodesTotal_SG = 0;               //注册节点总数


const char *NFC_ConfigInfo_leftKey[]={"名称", "型号", "DevEUI", "AppEUI"};
const char *NFC_ConfigInfo_rightKey[]={"LoRaWAN版本", "入网方式", "设备类型", "上报周期", "启用状态"};
const char *Node_ConfigInfo_leftKey[]={"设备名称", "激活状态", "备注", "DevEUI", "AppEUI", "电量"};
const char *Node_ConfigInfo_rightKey[]={"LoRaWAN版本", "设备类型", "入网方式", "活跃状态", "信号质量"};
const char *Network_LabelInfo[] = {"以太网", "WIFI", "蜂窝网络", "RNDIS网络"};



/**********************************************************************************
 *      LVGL对象的全局变量
 *      static将这些变量的作用域局限在userUI.c中，但生命周期与全局变量相同
 **********************************************************************************/
// 1、主窗口中
static lv_obj_t *tabview;   
static lv_obj_t *tab1;      // 选项卡1：节点信息-----------语种切换------------
static lv_obj_t *tab2;      // 选项卡3：网络信息-----------语种切换------------
static lv_obj_t *tab3;      // 选项卡2：Node-RED-----------语种切换------------

// 2、节点信息Tab中
static lv_obj_t* NodeDev_table;             // 节点信息表格控件-----------语种切换------------
// lv_obj_t *prev_btn                       // 上下翻页按钮(无需全局声明)
// lv_obj_t *next_btn
static lv_obj_t *label_prev;                // 上下翻页按钮上的标签-----------语种切换------------
static lv_obj_t *label_next;                      
static lv_obj_t *label_PageNum;             // 页码标签
static lv_obj_t *label_key_NodeNumber;      // 节点数量标签:key-----------语种切换------------
static lv_obj_t *label_value_NodeNumber;    // 节点数量标签:value
static lv_obj_t *label_key_4Gsignal;       // 4G信号标签:key-----------语种切换------------
static lv_obj_t *label_value_4Gsignal;     // 4G信号标签:value
static lv_obj_t *label_key_WIFI;            // WIFI连接标签:key-----------语种切换------------
static lv_obj_t *label_value_WIFI;          // WIFI连接标签:value

// 3、RTD弹窗
static lv_obj_t *RTD_Window;
static lv_obj_t *label_title_RTD;
static lv_obj_t *RealTimeData_table;  //实时数据流的表格控件

// 4、Node弹窗
static lv_obj_t *Node_window;
static lv_obj_t *label_title_NodePupUp;
static lv_obj_t *label_NodePopUp_left_KEY[Node_left_rowNum];
static lv_obj_t *label_NodePopUp_left_Value[Node_left_rowNum];
static lv_obj_t *label_NodePopUp_right_KEY[Node_right_rowNum];
static lv_obj_t *label_NodePopUp_right_Value[Node_right_rowNum];
static lv_obj_t *label_NodePopUp_JoinStatus_KEY;        //设置入网状态的标签
static lv_obj_t *ddlist_nodeState;

// 5、NFC弹窗
static lv_obj_t *NFC_window;
static lv_obj_t *label_title_NFC;
static lv_obj_t *label_NFCPopUp_left_KEY[NFC_left_rowNum];
static lv_obj_t *label_NFCPopUp_right_KEY[NFC_right_rowNum];
static lv_obj_t *label_NFCPopUp_left_Value[4];
static lv_obj_t *label_NFCPopUp_right_Value[2];
static lv_obj_t *NFC_ddlist[3];
static lv_obj_t *NFC_btn[3];
static lv_obj_t *label_NFC_btn[3];

// 6、网络信息Tab中
static lv_obj_t *label_InfoDes[4];         //网络信息标签描述-----------语种切换------------
static lv_obj_t *label_IP_key[4];          //IP_Key
static lv_obj_t *label_IP_value[4];        //IP_Value               
static lv_obj_t *label_Mask_key[4];        //子网掩码_Key-----------语种切换------------
static lv_obj_t *label_Mask_value[4];      //子网掩码_Value        
static lv_obj_t *label_Eth_DHCP_key;        //以太网的DHCP_key-----------语种切换------------
static lv_obj_t *label_Eth_DHCP_value;      //以太网的DHCP_Value
static lv_obj_t *label_Wifi_Mode_key;        //WIFI的模式_key
static lv_obj_t *label_Wifi_Mode_value;      //WIFI的模式_Value
static lv_obj_t *label_Wifi_signal_key;        //WIFI的信号质量_key-----------语种切换------------
static lv_obj_t *label_Wifi_signal_value;      //WIFI的信号质量_Value
static lv_obj_t *label_Cellular_SNR_key;        //蜂窝网络的网络状态_key-----------语种切换------------
static lv_obj_t *label_Cellular_SNR_value;      //蜂窝网络的网络状态_Value
static lv_obj_t *label_Cellular_sim_key;        //蜂窝网络的SIM卡_key-----------语种切换------------
static lv_obj_t *label_Cellular_sim_value;      //蜂窝网络的SIM卡_Value
static lv_obj_t *label_Cellular_ModsStatus_key;        //蜂窝网络的模组状态_key-----------语种切换------------
static lv_obj_t *label_Cellular_ModsStatus_value;      //蜂窝网络的模组状态_Value
static lv_obj_t *label_RNDIS_status_key;        //RNDIS网络状态_key-----------语种切换------------
static lv_obj_t *label_RNDIS_status_value;      //RNDIS网络网络状态_Value

// 7、Node-RED中
static lv_obj_t *ddlist_LanguageSwitch;

// 8、注册节点PupUp中
lv_obj_t *obj_base1; //NFC register PopUp
static lv_obj_t *img_RegisterTrue;
static lv_obj_t *img_RegisterFlase;
static lv_obj_t *img_RegisterFlase_IsExist;

// 9、更新节点PupUp中
lv_obj_t *obj_base2; //NFC update PopUp
lv_timer_t *timer;// 创建一个定时器对象——>用于判断节点更新是否超时
static lv_obj_t *img_updateTrue;
static lv_obj_t *img_updateFlase;
static lv_obj_t *img_updateing;
static lv_obj_t *img_WartingTimeout;

// 10、恢复出厂设置PupUp中
lv_obj_t *obj_base3;
// static lv_obj_t *img_updateing2;
static lv_obj_t *img_FactoryReset;
static lv_obj_t *img_FactoryReset_True;
static lv_obj_t *img_FactoryReset_Flase;


//-----------NodeDev主界面相关函数-------------
void MFunc_updateMainPage();
static void NodeDev_prevPage_event_handler( lv_event_t *event);
static void NodeDev_nextPage_event_handler(lv_event_t *event);
void Clear_NodeDev_table(int rows, int cols);
void Create_NodeDev_table();
void lora_MainWindow(void);
static void NodeDev_table_event_cb(lv_event_t *e);
//------------网络信息相关函数----------------
void lora_Network_TabUI(void);
//------------Node弹窗相关函数----------------
static void Event_Hidden_Node_Window(lv_event_t *event);
void Write_NodePopUP(const char* devName);
void lora_NodeWindow(void);
//-------------RTD弹窗相关函数----------------
static void RTDwin_Returnbtn_event_cb(lv_event_t* event);
void Clear_RTD_Table(void);
void Write_RTDPopUP(const char*devName);
void Create_RTD_Table(void);
void lora_RTDWindow(void);
//-------------NFC弹窗相关函数----------------
static void update_TimeoutJudgment(void);
static void lv_NodeupdateWin_close_event_cb(lv_event_t *event);
static void Event_Hidden_NFC_Window(lv_event_t *event);
static void dropdown0_event_cb(lv_event_t *event);
static void dropdown1_event_cb(lv_event_t *event);
static void network_event_cb(lv_event_t *e);
static void updateNode_event_cb(lv_event_t *e);
static void FactoryReset_event_cb(lv_event_t *e);
void lora_NFCWindow(void);


//能够引发段错误的函数
void foo();

std::string MF_execCommand(const std::string& cmd);
std::string MF_getIpAddress(const std::string& ifconfigOutput);
std::string MF_getSubnetMask(const std::string& ifconfigOutput);

void wirte_NetworkTabUI(lv_obj_t **label_ipAddress_Value, lv_obj_t **label_subnetMask_Value, const std::string& ipAddress, const std::string& subnetMask);
// 对选择的语种进行重新显示
void Replace_labelDisplay(int Languages);
// 创建自定义标签控件（KEY-Value）
void MF_create_customLabel_Init(lv_obj_t **label_key, lv_obj_t **label_value, lv_obj_t *parent, 
                                lv_coord_t key_width, lv_coord_t value_width, lv_coord_t height,
                                lv_coord_t x_offset, lv_coord_t y_offset,
                                const char *key_initText, const char *value_initText);




/**********************************************************************************
 *        与时间戳相关的函数
 **********************************************************************************/


std::string formatTimestamp(long long timestamp) {
    // 转换为std::chrono::time_point类型
    std::chrono::system_clock::time_point timePoint{ std::chrono::seconds(timestamp) };

    // 转换为std::tm结构体
    std::time_t time = std::chrono::system_clock::to_time_t(timePoint);
    std::tm* tm = std::localtime(&time);

    // 格式化为年月日时分秒的字符串
    std::ostringstream oss;
    oss << std::put_time(tm, "%H:%M:%S");
    return oss.str();
}


// 该函数的功能为：输入一个时间戳，会跟当前系统时间戳做差值并输出时间差
std::string printTimeDifference(time_t pastTimestamp) {

    time_t nowTimestamp = time(nullptr); // 获取当前时间的时间戳

    // 计算差值
    long long diff = nowTimestamp - pastTimestamp;
    // std::cout << "nowTimestamp: "<< nowTimestamp << ", pastTimestamp: " << pastTimestamp <<", diff: " << diff << std::endl;

    // 计算年、月、日、小时、分钟和秒数
    int years = diff / (365 * 24 * 60 * 60);
    int months = diff / (30 * 24 * 60 * 60);
    int days = diff / (24 * 60 * 60);
    int hours = diff / (60 * 60);
    int minutes = diff / 60;
    int seconds = diff;


    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();


    // 根据时间差进行分类输出
    if (years > 0) {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            return std::to_string(years) + "年前";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && years > 1) {
            return std::to_string(years) + " years ago";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            return std::to_string(years) + " year ago";
        }
    }
    else if (months > 0) {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            return std::to_string(months) + "个月前";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && months > 1) {
            return std::to_string(months) + " months ago";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            return std::to_string(months) + " month ago";
        }
    }
    else if (days > 0) {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            return std::to_string(days) + "天前";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && days > 1) {
            return std::to_string(days) + " days ago";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            return std::to_string(days) + " day ago";
        }
    }
    else if (hours > 0) {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            return std::to_string(hours) + "小时前";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && hours > 1) {
            return std::to_string(days) + " hours ago";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            return std::to_string(days) + " hour ago";
        }
    }
    else if (minutes > 0) {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            return std::to_string(minutes) + "分钟前";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && minutes > 1) {
            return std::to_string(days) + " minutes ago";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            return std::to_string(days) + " minute ago";
        }
    }
    else if (seconds >= 0) {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            return "刚刚";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            return "just now";
        }
    }
    else {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            return "未来时间";
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            return "Time in the future";
        }
    }
}





/**********************************************************************************
 *        NodeDev主界面相关函数
 **********************************************************************************/
//NodeDev界面————更新当前页面(页数并没有变化)√
void MFunc_updateMainPage()
{
    std::cout << "userUI.cpp——>324——>线程2——————进入: 更新NodeTableUI数据!" << std::endl;
    //单例模式确保类只有一个实例化对象
    LoraNodeDeviceClass *LoraNode_ClassOBJ = LoraNodeDeviceClass::getInstance();
    
    NodesTotal_SG = LoraNode_ClassOBJ->get_ListLength();


    if(NodesTotal_SG != 0){
        if(NodeTable_currentPage_SG > (int)ceil(NodesTotal_SG / (double)(NodeTable_RowsNum-1))){
            NodeTable_currentPage_SG = (int)ceil(NodesTotal_SG / (double)(NodeTable_RowsNum-1));
        }
        lv_label_set_text_fmt(label_PageNum,"#0066ff %d# / %d",NodeTable_currentPage_SG, (int)ceil(NodesTotal_SG / (double)(NodeTable_RowsNum-1)));
    }else{
        lv_label_set_text_fmt(label_PageNum,"#0066ff %d# / %d",NodeTable_currentPage_SG, 1);
    }

    Clear_NodeDev_table(3,5);
    lv_label_set_text_fmt(label_value_NodeNumber,"%d",NodesTotal_SG);

    
    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();


    int row = 1;
    const int MaxPerPage = NodeTable_RowsNum - 1;
    std::list<LORA_NODEDEVICE_INFO>::const_iterator item = LoraNode_ClassOBJ->getNodeDeviceList().begin();
    int start_index=(NodeTable_RowsNum-1)*(NodeTable_currentPage_SG-1);
    int end_index = std::min((start_index + MaxPerPage), NodesTotal_SG);
        std::advance(item, start_index); // 移动迭代器到指定元素位置
        for (int i = start_index; i < end_index; ++i) {
            lv_table_set_cell_value(NodeDev_table, row, 0, item->ANode_Info.devName);
            if(item->dataNumbers == 0) {
                lv_table_set_cell_value_fmt(NodeDev_table, row, 1, item->ANode_Info.lastOnlineTime);
            } else {
                std::string activeTimeStr = printTimeDifference(item->timeTamp);
                // std::cout << "activeTimeStr:" << activeTimeStr <<std::endl;
                // std::cout << "activeTimeStr.c_str():" << activeTimeStr.c_str() <<std::endl;
                lv_table_set_cell_value_fmt(NodeDev_table, row, 1, activeTimeStr.c_str());
            }
            lv_table_set_cell_value_fmt(NodeDev_table, row, 2, to_string(item->dataNumbers).c_str());
            if (item->RSSI < 0 && item->RSSI >= Signal_RSSI_A) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                    lv_table_set_cell_value(NodeDev_table, row, 3, "优");
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                    lv_table_set_cell_value(NodeDev_table, row, 3, "A");
                }
            } else if (item->RSSI < Signal_RSSI_A && item->RSSI > Signal_RSSI_B) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                    lv_table_set_cell_value(NodeDev_table, row, 3, "良");
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                    lv_table_set_cell_value(NodeDev_table, row, 3, "B");
                }
            } else if (item->RSSI < Signal_RSSI_B && item->RSSI > Signal_RSSI_C) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                    lv_table_set_cell_value(NodeDev_table, row, 3, "中");
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                    lv_table_set_cell_value(NodeDev_table, row, 3, "C");
                }
            } else if (item->RSSI < Signal_RSSI_C ) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                    lv_table_set_cell_value(NodeDev_table, row, 3, "差");
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                    lv_table_set_cell_value(NodeDev_table, row, 3, "D");
                }
            } else {
                lv_table_set_cell_value(NodeDev_table, row, 3, "");
            }
            lv_table_set_cell_value(NodeDev_table, row, 4, item->ANode_Info.Remark);
            for(int j = 0; j<NodeTable_ColsNum; j++){
                //对表格中的文本进行裁剪——>文本会自动换行，但是不会自动增加行的高度
                lv_table_add_cell_ctrl(NodeDev_table, row, j, LV_TABLE_CELL_CTRL_TEXT_CROP);
            }
            lv_obj_invalidate(NodeDev_table);//自动刷新表格 ，不加这句就需要手动点击表格刷新
            ++item;
            ++row;
        }
    std::cout << "userUI.cpp——>324——>线程2——————退出: NodeTableUI数据已更新!" << std::endl;
}

//NodeDev界面————上一页按钮回调函数√
static void NodeDev_prevPage_event_handler( lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    if (code == LV_EVENT_CLICKED) {
        NodeTable_currentPage_SG--;
        if(NodeTable_currentPage_SG<=0){
            NodeTable_currentPage_SG = 1;
            return;
        }
        MFunc_updateMainPage();
    }
}

//NodeDev界面————下一页按钮回调函数√
static void NodeDev_nextPage_event_handler(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    if (code == LV_EVENT_CLICKED) {
        NodeTable_currentPage_SG++;
        if(NodeTable_currentPage_SG*(NodeTable_RowsNum-1) >= NodesTotal_SG+(NodeTable_RowsNum-1)){
            NodeTable_currentPage_SG--;
            return;
        }
        MFunc_updateMainPage();
    }
}


// Tab3——重新加载多语言文件
static void reloadLanguage_event_handler(lv_event_t *event) {
    std::cout << "userUI.cpp——>404——>进入了reloadLanguage事件处理！" << std::endl;
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    if (code == LV_EVENT_CLICKED) {
        //单例模式确保类只有一个实例化对象
        languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();
        languageSwitch_ClassOBJ->parse_JsonFile();
        // 重新加载完后默认为中文
        Replace_labelDisplay(languageSwitch_ClassOBJ->get_CurrentLanguageType());
    }
    std::cout << "userUI.cpp——>404——>退出了reloadLanguage事件处理！" << std::endl;
}



void Get_NetworkInfo(void) {
    // 1、http请求获取——>以太网信息
    std::string getEthInfo_url = "https://127.0.0.1:8080/api/network/ethernet";
    std::string response_EthINFO = send_httpRequest(getEthInfo_url, Method_get);
    std::string eth_IP = "";
    std::string eth_Mask = "";
    std::string eth_DHCP = "";
    cjsonParse_getEthINFO(response_EthINFO.c_str(), eth_IP, eth_Mask, eth_DHCP);
    wirte_NetworkTabUI(&label_IP_value[0], &label_Mask_value[0], eth_IP, eth_Mask);
    lv_label_set_text(label_Eth_DHCP_value, eth_DHCP.c_str());


    // 2、http请求获取——>wifi信息
    std::string getWifiInfo_url = "https://127.0.0.1:8080/api/network/wifi";
    std::string response_WifiINFO = send_httpRequest(getWifiInfo_url, Method_get);
    std::string wifi_Mode = "";
    std::string wifi_IP = "";
    std::string wifi_Mask = "";
    cjsonParse_getWifiINFO(response_WifiINFO.c_str(), wifi_Mode, wifi_IP, wifi_Mask);                       
    wirte_NetworkTabUI(&label_IP_value[1], &label_Mask_value[1], wifi_IP, wifi_Mask);
    lv_label_set_text(label_Wifi_Mode_value, wifi_Mode.c_str());


    // 3、http请求获取——>蜂窝网络信息 
    std::string getCat1Info_url = "https://127.0.0.1:8080/api/network/cat1";
    std::string response_Cat1INFO = send_httpRequest(getCat1Info_url, Method_get);
    std::string Cat1_IP = "";
    std::string Cat1_SIM = "";
    std::string Cat1_Mode = "";
    std::string Cat1_SNR = "";
    cjsonParse_getCat1INFO(response_Cat1INFO.c_str(), Cat1_IP, Cat1_SIM, Cat1_Mode, Cat1_SNR);       
    lv_label_set_text(label_IP_value[2], Cat1_IP.c_str());
    lv_label_set_text(label_Cellular_sim_value, Cat1_SIM.c_str());
    lv_label_set_text(label_Cellular_ModsStatus_value, Cat1_Mode.c_str());
    lv_label_set_text(label_Cellular_SNR_value, Cat1_SNR.c_str());


    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();
    // 4、RNDIS网络获取其IP和子网掩码（如果获取不到说明RNDIS未打开）
    std::string ifconfigOutput = MF_execCommand("ifconfig br0");
    std::string ipAddress = MF_getIpAddress(ifconfigOutput);
    std::string subnetMask = MF_getSubnetMask(ifconfigOutput);
    wirte_NetworkTabUI(&label_IP_value[3], &label_Mask_value[3], ipAddress, subnetMask);
    if (ipAddress != "") {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
            lv_label_set_text(label_RNDIS_status_value, "启用");
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
            lv_label_set_text(label_RNDIS_status_value, "enable");
        }
    } else {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
            lv_label_set_text(label_RNDIS_status_value, "禁用");
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
            lv_label_set_text(label_RNDIS_status_value, "disable");
        }
    }

    std::cout << "__________________Cat1_SNR = " << Cat1_SNR << std::endl;
    if(Cat1_SNR != "") {
        lv_label_set_text_fmt(label_value_4Gsignal,"%s",Cat1_SNR.c_str());
    }
    std::cout << "__________________wifi_Mode = " << wifi_Mode << " ___wifi_Mode = " << wifi_IP << std::endl;
    if(wifi_Mode == "STA" && wifi_IP != "") {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
            lv_label_set_text(label_value_WIFI,"已连接");
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
            lv_label_set_text(label_value_WIFI,"connected");
        }
    } else {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
            lv_label_set_text(label_value_WIFI,"未连接");
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
            lv_label_set_text(label_value_WIFI,"disconnect");
        }
        
    }
}

static void Network_TabUI_event_cb(lv_event_t *event) {
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    std::cout << "userUI.cpp——>419——>进入了Network TabUI 事件处理！" << std::endl;
    if (code == LV_EVENT_CLICKED) {
        Get_NetworkInfo();
    }
    std::cout << "userUI.cpp——>419——>退出了Network TabUI 事件处理！" << std::endl;
}

// 对选择的语种进行重新显示
void Replace_labelDisplay(int Languages) {
    
    // 1、单例模式确保类只有一个实例化对象
    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();

    languageSwitch_ClassOBJ->set_CurrentLanguageType(Languages);

    // 2、tabview中Tab标签
    lv_tabview_rename_tab(tabview, 0, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo",Languages).c_str());
    lv_tabview_rename_tab(tabview, 1, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo",Languages).c_str());
    lv_tabview_rename_tab(tabview, 2, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab3_NodeREDInfo",Languages).c_str());

    // 3、Tab1中的固定标签（节点信息）
    // 3.1 表格控件中的字段
    lv_table_set_cell_value(NodeDev_table, 0, 0, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.devName",Languages).c_str());
    lv_table_set_cell_value(NodeDev_table, 0, 1, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.activeState",Languages).c_str());     
    lv_table_set_cell_value(NodeDev_table, 0, 2, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.Transmit",Languages).c_str());
    lv_table_set_cell_value(NodeDev_table, 0, 3, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.Signal",Languages).c_str());
    lv_table_set_cell_value(NodeDev_table, 0, 4, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.Remark",Languages).c_str());
    // 3.2 按钮、标签
    lv_label_set_text(label_prev, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.prevPage",Languages).c_str());
    lv_label_set_text(label_next, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.nextPage",Languages).c_str());
    lv_label_set_text(label_key_NodeNumber, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.registrationNumber",Languages).c_str());
    lv_label_set_text(label_key_4Gsignal, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.4Gsignal",Languages).c_str());
    lv_label_set_text(label_key_WIFI, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab1_DevInfo.WIFI",Languages).c_str());
    
    // 4、Tab1中的RTD弹窗
    lv_table_set_cell_value(RealTimeData_table, 0, 0, languageSwitch_ClassOBJ->get_languageLabelInfo("RTDPopUp.packageType",Languages).c_str());
    lv_table_set_cell_value(RealTimeData_table, 0, 1, languageSwitch_ClassOBJ->get_languageLabelInfo("RTDPopUp.Time",Languages).c_str());  

    // 5、Tab1中的Node弹窗
    lv_label_set_text(label_title_NodePupUp, languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.nodeDetails",Languages).c_str());
    lv_label_set_text(label_NodePopUp_left_KEY[0], languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.devName",Languages).c_str());
    lv_label_set_text(label_NodePopUp_left_KEY[1], languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.Activated",Languages).c_str());
    lv_label_set_text(label_NodePopUp_left_KEY[2], languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.Remark",Languages).c_str());
    lv_label_set_text(label_NodePopUp_left_KEY[5], languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.Battery",Languages).c_str());
    lv_label_set_text(label_NodePopUp_right_KEY[0], languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.LorawanVersion",Languages).c_str());
    lv_label_set_text(label_NodePopUp_right_KEY[1], languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.LoraClass",Languages).c_str());
    lv_label_set_text(label_NodePopUp_right_KEY[2], languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.accessMode",Languages).c_str());
    lv_label_set_text(label_NodePopUp_right_KEY[3], languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.activeState",Languages).c_str());
    lv_label_set_text(label_NodePopUp_right_KEY[4], languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.signalQuality",Languages).c_str());
    lv_label_set_text(label_NodePopUp_JoinStatus_KEY, languageSwitch_ClassOBJ->get_languageLabelInfo("NodePopUp.SetNetworkStatus",Languages).c_str());

    // 6、NFC弹窗
    lv_label_set_text(label_NFCPopUp_left_KEY[0], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.Name",Languages).c_str());
    lv_label_set_text(label_NFCPopUp_left_KEY[1], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.DevModel",Languages).c_str());
    lv_label_set_text(label_NFCPopUp_right_KEY[0], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.LorawanVersion",Languages).c_str());
    lv_label_set_text(label_NFCPopUp_right_KEY[1], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.accessMode",Languages).c_str());
    lv_label_set_text(label_NFCPopUp_right_KEY[2], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.LoraClass",Languages).c_str());
    lv_label_set_text(label_NFCPopUp_right_KEY[3], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.uploadInterval",Languages).c_str());
    lv_label_set_text(label_NFCPopUp_right_KEY[4], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.EnabledState",Languages).c_str());
    lv_label_set_text(label_NFC_btn[0], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.RegisterNode",Languages).c_str());
    lv_label_set_text(label_NFC_btn[1], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.UpdateNode",Languages).c_str());
    lv_label_set_text(label_NFC_btn[2], languageSwitch_ClassOBJ->get_languageLabelInfo("NFCPopUp.FactoryReset",Languages).c_str());

    // 7、Tab2中的固定标签（网络信息）
    // 7.1 网络标签描述
    lv_label_set_text(label_InfoDes[0], languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.Ethernet",Languages).c_str());
    lv_label_set_text(label_InfoDes[2], languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.CellularNetwork",Languages).c_str());
    lv_label_set_text(label_InfoDes[3], languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.RNDIS",Languages).c_str());
    

    // 7.2 各网络标签中属性信息标签
    lv_label_set_text(label_Mask_key[0], languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.SubnetMask",Languages).c_str());  // 子网掩码
    lv_label_set_text(label_Mask_key[1], languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.SubnetMask",Languages).c_str());
    lv_label_set_text(label_Mask_key[3], languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.SubnetMask",Languages).c_str());

    lv_label_set_text(label_Wifi_Mode_key, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.WIFIMode",Languages).c_str());
    lv_label_set_text(label_Wifi_signal_key, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.WIFIsignal",Languages).c_str());
    lv_label_set_text(label_Cellular_sim_key, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.CellularSIM",Languages).c_str());
    lv_label_set_text(label_Cellular_ModsStatus_key, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.CellularModsStatus",Languages).c_str());
    lv_label_set_text(label_Cellular_SNR_key, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.CellularNetworkState",Languages).c_str());
    lv_label_set_text(label_RNDIS_status_key, languageSwitch_ClassOBJ->get_languageLabelInfo("Tab2_NetworkInfo.RNDISstate",Languages).c_str());

    // 8、更换图片中涉及的语种
    if(Languages == CHINESE){
        // 注册
        lv_img_set_src(img_RegisterTrue, &RegisterTrue);  //设置图片源
        lv_img_set_src(img_RegisterFlase_IsExist, &RegisterFlase_IsExist); 
        // 更新
        lv_img_set_src(img_updateTrue, &UpdateSucceed_PNG); 
        lv_img_set_src(img_updateFlase, &updateFlase); 
        lv_img_set_src(img_updateing, &Updating);  
        lv_img_set_src(img_WartingTimeout, &WartingTimeout); 
        // 恢复出厂设置
        lv_img_set_src(img_FactoryReset, &FactoryReset_PNG);
            lv_obj_align_to(img_FactoryReset, obj_base3, LV_ALIGN_TOP_MID, 0, 25);
        lv_img_set_src(img_FactoryReset_True, &FactoryReset_True);
        lv_img_set_src(img_FactoryReset_Flase, &FactoryReset_Flase);
        // 其他
    }else if(Languages == ENGLISH){
        // 注册
        lv_img_set_src(img_RegisterTrue, &English_RegisterTrue_PNG);  
        lv_img_set_src(img_RegisterFlase_IsExist, &English_RegisterFlase_PNG);  
        // 更新
        lv_img_set_src(img_updateTrue, &English_UpdateTrue_PNG); 
        lv_img_set_src(img_updateFlase, &English_UpdateFalse_PNG); 
        lv_img_set_src(img_updateing, &English_Configuring_PNG);  //设置图片源
        lv_img_set_src(img_WartingTimeout, &English_TimeOut_PNG); 
        // 恢复出厂设置
        lv_img_set_src(img_FactoryReset, &English_Confirm_PNG);
            lv_obj_align_to(img_FactoryReset, obj_base3, LV_ALIGN_TOP_MID, -15, 0);
        lv_img_set_src(img_FactoryReset_True, &English_ResetTrue_PNG);
        lv_img_set_src(img_FactoryReset_Flase, &English_ResetFalse_PNG);

        // 其他
    }
}


// 语种切换功能的设计
static void ddlist_LanguageSwitch_event_cb(lv_event_t *event) {
    std::cout << "userUI.cpp——>552——>进入了LanguageSwitch事件处理！" << std::endl;
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    if (code == LV_EVENT_VALUE_CHANGED) {
        char selected_option[16];
        lv_dropdown_get_selected_str(ddlist_LanguageSwitch, selected_option, 16);

        if (strcmp(selected_option, "汉语") == 0) {
            std::cout << "显示为汉语" << std::endl;
            Replace_labelDisplay(CHINESE);
            {
                std::string getNodeInfo_url = "https://127.0.0.1:8080/api/devices?applicationID=0&limit=200&offset=0";
                std::lock_guard<std::mutex> lock(mutex_NodeTabel);
                modified_Node = true;
                // http请求：获取节点设备信息
                std::string response_NodeINFO = send_httpRequest(getNodeInfo_url, Method_get);
                cjsonParse_getNodeDevINFO(response_NodeINFO.c_str());
                std::cout << "Node表格————更新表格信号————发出" << std::endl;
                cv_NodeTabel.notify_one();  
            }
        } else if (strcmp(selected_option, "日语") == 0) {
            std::cout << "显示为日语" << std::endl;
            // Replace_labelDisplay(JAPANESE);
        } else if (strcmp(selected_option, "韩语") == 0) {
            std::cout << "显示为韩语" << std::endl;
            // Replace_labelDisplay(KOREAN);
        } else if (strcmp(selected_option, "英语") == 0) {
            std::cout << "显示为英语" << std::endl;
            Replace_labelDisplay(ENGLISH);
            {
                std::string getNodeInfo_url = "https://127.0.0.1:8080/api/devices?applicationID=0&limit=200&offset=0";
                std::lock_guard<std::mutex> lock(mutex_NodeTabel);
                modified_Node = true;
                // http请求：获取节点设备信息
                std::string response_NodeINFO = send_httpRequest(getNodeInfo_url, Method_get);
                cjsonParse_getNodeDevINFO(response_NodeINFO.c_str());
                std::cout << "Node表格————更新表格信号————发出" << std::endl;
                cv_NodeTabel.notify_one();  
            }
        }
    }
    std::cout << "userUI.cpp——>552——>退出了LanguageSwitch事件处理！" << std::endl;
}



//NodeDev表格————清空表格
void Clear_NodeDev_table(int rows, int cols)
{
    //清空节点设备信息表格的函数(参数1：行数  参数2：列数  参数3：要清除的表格)
    for (int row = 1; row < rows+1; row++) {
        for (int col = 0; col < cols; col++) {
            lv_table_set_cell_value(NodeDev_table, row, col, " ");
        }
    }
    lv_obj_invalidate(NodeDev_table);//自动刷新表格 ，不加这句就需要手动点击表格刷新
}

//NodeDev表格————创建空表
void Create_NodeDev_table()
{

    //设置表对象的行列数
    lv_table_set_row_cnt(NodeDev_table, NodeTable_RowsNum);
    lv_table_set_col_cnt(NodeDev_table, NodeTable_ColsNum);
    //设置列宽(每个选项的宽度)
    lv_table_set_col_width(NodeDev_table, 0, 150);
    lv_table_set_col_width(NodeDev_table, 1, 85);
    lv_table_set_col_width(NodeDev_table, 2, 65);
    lv_table_set_col_width(NodeDev_table, 3, 60);
    lv_table_set_col_width(NodeDev_table, 4, 80);
    //设置单元格数据
    lv_table_set_cell_value(NodeDev_table, 0, 0, "设备名称");
    lv_table_set_cell_value(NodeDev_table, 0, 1, "活跃状态");     //第0行第1列设置为"status"字符串
    lv_table_set_cell_value(NodeDev_table, 0, 2, "传输");
    lv_table_set_cell_value(NodeDev_table, 0, 3, "信号");
    lv_table_set_cell_value(NodeDev_table, 0, 4, "备注");
    for(int i = 0; i<5; i++){
        //对表格中的文本进行裁剪——>文本会自动换行，但是不会自动增加行的高度
        lv_table_add_cell_ctrl(NodeDev_table, 0, i, LV_TABLE_CELL_CTRL_TEXT_CROP);
    }
    Clear_NodeDev_table(3,5);
}



static void scroll_begin_event(lv_event_t * e)
{
    /*Disable the scroll animations. Triggered when a tab button is clicked */
    if(lv_event_get_code(e) == LV_EVENT_SCROLL_BEGIN) {
        lv_anim_t * a = (lv_anim_t *)lv_event_get_param(e);
        if(a)  a->time = 0;
    }
}


//UI————主界面的搭建函数（不填入数据的图形界面）√
void lora_MainWindow(void)
{
    //1、搭建主界面UI——整体搭建在选项卡中（节点设备信息、Node-RED）
    tabview = lv_tabview_create(lv_scr_act(), LV_DIR_TOP, 30);    //创建选项卡并获取选项卡内容主体部分
    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tabview);  //获取选项卡部件的选项卡按钮
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0xb8ddfa), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_border_color(tab_btns, lv_color_hex(0xe36b29), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x000000), 0);  //本地样式修改未选中时的文字颜色——黑色
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0xe36b29), LV_PART_ITEMS | LV_STATE_CHECKED);   //本地样式修改选中时的文字颜色
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0xb8ddfa), 0);

    // lv_obj_clear_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(tabview, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(lv_tabview_get_content(tabview), LV_OBJ_FLAG_SCROLLABLE);


    // lv_tabview_set_act(tabview, 0, LV_ANIM_OFF);
    lv_obj_add_event_cb(lv_tabview_get_content(tabview), scroll_begin_event, LV_EVENT_SCROLL_BEGIN, NULL);

    //2、将主界面分为节点设备信息 和 Node-RED两部分 和网络信息
    tab1 = lv_tabview_add_tab(tabview, "节点信息");  //添加选项
    tab2 = lv_tabview_add_tab(tabview, "网络信息");
    tab3 = lv_tabview_add_tab(tabview, "Node-RED");
    lv_obj_clear_flag(tab2, LV_OBJ_FLAG_SCROLLABLE);  //取消表格的下滑滚动条
    lv_obj_clear_flag(tab3, LV_OBJ_FLAG_SCROLLABLE);  //取消表格的下滑滚动条


    lv_obj_add_event_cb(tab_btns, Network_TabUI_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *LanguageSwitch_label = lv_label_create(tab3);
    lv_label_set_text(LanguageSwitch_label,"语种选择: ");
    lv_obj_center(LanguageSwitch_label);
    lv_obj_align_to(LanguageSwitch_label, tab3, LV_ALIGN_TOP_LEFT,0,10);

    //创建一个下拉列表
    ddlist_LanguageSwitch = lv_dropdown_create(tab3);
    //设置下拉列表对象的宽高
    lv_obj_set_height(ddlist_LanguageSwitch, 40);
    lv_obj_set_width(ddlist_LanguageSwitch, 100);
    lv_obj_align_to(ddlist_LanguageSwitch, tab3, LV_ALIGN_TOP_LEFT,70,0);
    //在下拉列表对象上添加一个字符标志
    lv_dropdown_set_symbol(ddlist_LanguageSwitch, LV_SYMBOL_DOWN);

    //在下拉列表中添加选项
    lv_dropdown_set_options(ddlist_LanguageSwitch, "汉语\n日语\n韩语\n英语");
    // lv_obj_set_style_text_font(ddlist_LanguageSwitch, &lv_font_montserrat_12,0);   // 使用字库中不同字号的字体

    // 设置选中选项后的回调函数
    lv_obj_add_event_cb(ddlist_LanguageSwitch, ddlist_LanguageSwitch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // 重新加载多语种.json文件
    lv_obj_t *reloadLanguage_btn = lv_btn_create(tab3);
    lv_obj_set_size(reloadLanguage_btn,120,40);
    lv_obj_align(reloadLanguage_btn,LV_ALIGN_TOP_LEFT,70,80);
    lv_obj_t *label_reloadLanguage = lv_label_create(reloadLanguage_btn);
    lv_label_set_text(label_reloadLanguage,"重新加载多语言文件");
    lv_obj_center(label_reloadLanguage);
    lv_obj_add_event_cb(reloadLanguage_btn, reloadLanguage_event_handler, LV_EVENT_CLICKED, NULL);  


    //绘制网络信息UI界面
    lora_Network_TabUI();
    



    //3、在节点设备信息选项卡下创建——节点信息表格控件
    NodeDev_table = lv_table_create(tab1);    //在tab1上放置表格table1
    lv_obj_align_to(NodeDev_table, tab1, LV_ALIGN_TOP_LEFT, 0, 0);
    Create_NodeDev_table();   //创建节点信息表格（空表）

    //4、创建上下翻页的按钮以及页码——以及相关功能实现
    lv_obj_t *prev_btn = lv_btn_create(tab1);
    lv_obj_t *next_btn = lv_btn_create(tab1);
    lv_obj_set_size(prev_btn,80,35);
    lv_obj_set_size(next_btn,80,35);
    lv_obj_align(prev_btn,LV_ALIGN_BOTTOM_LEFT,50,-30);
    lv_obj_align(next_btn,LV_ALIGN_BOTTOM_RIGHT,-50,-30);
    label_prev = lv_label_create(prev_btn);
    label_next = lv_label_create(next_btn);
    lv_label_set_text(label_prev,"上 页");
    lv_label_set_text(label_next,"下 页");
    lv_obj_center(label_prev);
    lv_obj_center(label_next);


    //-------------------------------------------------
    label_PageNum = lv_label_create(tab1);
    lv_obj_set_size(label_PageNum,50,25);
    lv_obj_align(label_PageNum,LV_ALIGN_BOTTOM_MID,0,-30);
    lv_label_set_recolor( label_PageNum, true ); 						/* 开启重新着色功能 */

    lv_obj_add_event_cb(prev_btn, NodeDev_prevPage_event_handler, LV_EVENT_CLICKED, NULL);  //给表1添加上翻页面事件(写事件回调函数)
    lv_obj_add_event_cb(next_btn, NodeDev_nextPage_event_handler, LV_EVENT_CLICKED, NULL);  //给表1添加下翻页面事件(写事件回调函数)

    //5、创建三个标签用于表示——入网节点数、4G信号质量、WIFI
    
    
    MF_create_customLabel_Init(&label_key_NodeNumber, &label_value_NodeNumber, tab1, 80, 20, 22, 0, 235, "入网节点数：", "0");
    MF_create_customLabel_Init(&label_key_4Gsignal, &label_value_4Gsignal, tab1, 80, 20, 22, 165, 235, "4G信号质量：", "");
    MF_create_customLabel_Init(&label_key_WIFI, &label_value_WIFI, tab1, 40, 45, 22, 315, 235, "WIFI：", "未连接");

    MFunc_updateMainPage();//首次更新表格——>全是初始化的信息
}

//NodeDev表格————表中按钮事件触发（第1、3列）
static void NodeDev_table_event_cb(lv_event_t *e)
{
    std::cout << "userUI.cpp——>717——>进入: 触发NodeDev表中单元格事件" << std::endl;
    lv_obj_t *obj = lv_event_get_target(e);
    uint16_t  col;
    uint16_t  row;
    lv_table_get_selected_cell(obj, &row, &col);    //获得选定的单元格
    if (row != 0 && col == 0){   //表示行数不为0，列数为0列的单元格执行后，弹出一个新的窗口
        const char * cellValue_devName = lv_table_get_cell_value(obj, row, col);
        if(strcmp(cellValue_devName, " ")){
            Write_NodePopUP(cellValue_devName);
            lv_obj_add_flag(tabview, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(Node_window, LV_OBJ_FLAG_HIDDEN);
        }
    }else if(row != 0 && col == 2){
        const char * cellValue_devName = lv_table_get_cell_value(obj, row, 0);
        if(strcmp(cellValue_devName, " ")){
            strcpy(RTDTabel_option, cellValue_devName);
            Write_RTDPopUP(RTDTabel_option);
            // 给主窗口tabview的添加隐藏属性，清除RTD_Window的隐藏属性
            lv_obj_add_flag(tabview, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(RTD_Window, LV_OBJ_FLAG_HIDDEN);
        }
    }
    std::cout << "userUI.cpp——>717——>退出: 触发NodeDev表中单元格事件" << std::endl;
}


/**********************************************************************************
 *        网络信息弹窗相关函数
 **********************************************************************************/

// 通过管道向外部进程获取信息的函数
std::string MF_execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// 用正则表达式捕获IP的函数
std::string MF_getIpAddress(const std::string& ifconfigOutput) {
    std::regex regexPattern(R"(inet ([\d.]+))");
    std::smatch match;
    if (std::regex_search(ifconfigOutput, match, regexPattern)) {
        return match.str(1);
    }
    return "";
}

// 用正则表达式捕获子网掩码的函数
std::string MF_getSubnetMask(const std::string& ifconfigOutput) {
    std::regex regexPattern(R"(netmask ([\d.]+))");
    std::smatch match;
    if (std::regex_search(ifconfigOutput, match, regexPattern)) {
        return match.str(1);
    }
    return "";
}


void wirte_NetworkTabUI(lv_obj_t **label_ipAddress_Value, lv_obj_t **label_subnetMask_Value, const std::string& ipAddress, const std::string& subnetMask) {

    lv_label_set_text(*label_ipAddress_Value, ipAddress.c_str());
    lv_label_set_text(*label_subnetMask_Value, subnetMask.c_str());

    // std::cout << "每隔5s向网络信息窗口中填写数据" << std::endl;

}

// 创建KEY-Value标签
void MF_create_customLabel_Init(lv_obj_t **label_key, lv_obj_t **label_value, lv_obj_t *parent, 
                                lv_coord_t key_width, lv_coord_t value_width, lv_coord_t height, 
                                lv_coord_t x_offset, lv_coord_t y_offset, const char *key_initText, const char *value_initText)
{
    *label_key = lv_label_create(parent);
    lv_obj_set_width(*label_key, key_width);
    lv_obj_set_height(*label_key, height);
    lv_obj_align_to(*label_key, parent, LV_ALIGN_TOP_LEFT, x_offset, y_offset);
    lv_label_set_text(*label_key, key_initText);
    lv_obj_set_style_text_align(*label_key, LV_TEXT_ALIGN_LEFT, 0);
    // lv_obj_set_style_bg_color(*label_key, lv_color_hex(0x50b8fe), LV_STATE_DEFAULT);  // 标签key的颜色
    // lv_obj_set_style_bg_opa(*label_key, 255, LV_STATE_DEFAULT);

    *label_value = lv_label_create(parent);
    lv_obj_set_width(*label_value, value_width);
    lv_obj_set_height(*label_value, height);
    lv_obj_align_to(*label_value, parent, LV_ALIGN_TOP_LEFT, x_offset + key_width, y_offset);
    lv_label_set_text(*label_value, value_initText);
    lv_obj_set_style_text_align(*label_value, LV_TEXT_ALIGN_LEFT, 0);
    // lv_obj_set_style_bg_color(*label_value, lv_color_hex(0x6a9955), LV_STATE_DEFAULT);  // 标签value的颜色
    // lv_obj_set_style_bg_opa(*label_value, 255, LV_STATE_DEFAULT);

}



//UI————搭建网络信息UI界面
void lora_Network_TabUI(void) {

    //[0]Ethernet以太网 [1]WIFI [2]Cellular蜂窝网络 [3]RNDIS网络
    lv_obj_t *obj_networkInfoArea[4];   //各网络信息显示的区域
    

    for(int i = 0; i<4; i++){
        obj_networkInfoArea[i] = lv_obj_create(tab2);
        lv_obj_set_size( obj_networkInfoArea[i],460,70);
        lv_obj_set_style_radius(obj_networkInfoArea[i], 5, LV_PART_MAIN);//设置圆角的半径属性（下拉列表框对象变为圆角）
        lv_obj_align(obj_networkInfoArea[i],LV_ALIGN_TOP_LEFT,-5,-5+i*67);
        lv_obj_clear_flag(obj_networkInfoArea[i], LV_OBJ_FLAG_SCROLLABLE);  //取消对象的下滑滚动条
        lv_obj_set_style_border_color(obj_networkInfoArea[i],lv_color_hex(0xff0000),LV_STATE_DEFAULT); //边框颜色

        // 各类网络标签
        label_InfoDes[i] =  lv_label_create(obj_networkInfoArea[i]);
        lv_obj_set_width(label_InfoDes[i], 90);
        lv_obj_set_height(label_InfoDes[i], 25);
        lv_obj_set_style_radius(label_InfoDes[i], 10, LV_PART_MAIN);//设置圆角的半径属性（下拉列表框对象变为圆角）
        lv_obj_align_to(label_InfoDes[i], obj_networkInfoArea[i], LV_ALIGN_TOP_LEFT, -7, -8);
        lv_label_set_text(label_InfoDes[i], Network_LabelInfo[i]);
        lv_obj_set_style_text_align(label_InfoDes[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_bg_color(label_InfoDes[i], lv_color_hex(0x50b8fe), LV_STATE_DEFAULT); // 设置屏幕背景颜色为浅蓝色
        lv_obj_set_style_bg_opa(label_InfoDes[i],255,LV_STATE_DEFAULT);

        // IP标签的key与value
        MF_create_customLabel_Init(&label_IP_key[i], &label_IP_value[i], obj_networkInfoArea[i], 35, 100, 20, 110, -5, "IP:", "");
        // Mask子网掩码标签的key与value
        if (i != 2) {
            MF_create_customLabel_Init(&label_Mask_key[i], &label_Mask_value[i], obj_networkInfoArea[i], 75, 100, 20, 260, -5, "子网掩码:", "");
        }
    }

    // 一行三条数据 -5 110 260

    // 以太网DHCP的key和value
    MF_create_customLabel_Init(&label_Eth_DHCP_key, &label_Eth_DHCP_value, obj_networkInfoArea[0], 40, 60, 20, 110, 22, "DHCP:", "");

    // WIFI模式的key和value
    MF_create_customLabel_Init(&label_Wifi_Mode_key, &label_Wifi_Mode_value, obj_networkInfoArea[1], 40, 60, 20, 110, 22, "模式:", "");
    // WIFI信号质量的key和value
    MF_create_customLabel_Init(&label_Wifi_signal_key, &label_Wifi_signal_value, obj_networkInfoArea[1], 70, 60, 20, 260, 22, "信号质量:", "");

    // 蜂窝网络SIM卡key和value
    // MF_create_customLabel_Init(&label_Cellular_sim_key, &label_Cellular_sim_value, obj_networkInfoArea[2], 55, 55, 20, -5, 22, "SIM卡:", "");
    MF_create_customLabel_Init(&label_Cellular_sim_key, &label_Cellular_sim_value, obj_networkInfoArea[2], 75, 100, 20, 260, -5, "SIM卡:", "");
    // 蜂窝网络模组状态的key和value
    MF_create_customLabel_Init(&label_Cellular_ModsStatus_key, &label_Cellular_ModsStatus_value, obj_networkInfoArea[2], 75, 45, 20, 110, 22, "模组状态:", "");
    // 蜂窝网络网络状态的key和value
    MF_create_customLabel_Init(&label_Cellular_SNR_key, &label_Cellular_SNR_value, obj_networkInfoArea[2], 85, 60, 20, 260, 22, "信号质量:", "");
    // RNDIS网络状态的key和value
    MF_create_customLabel_Init(&label_RNDIS_status_key, &label_RNDIS_status_value, obj_networkInfoArea[3], 40, 60, 20, 110, 22, "状态:", "");

}





/**********************************************************************************
 *        Node弹窗相关函数
 **********************************************************************************/
//隐藏NodeDev窗口、显示主窗口
static void Event_Hidden_Node_Window(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    if (code == LV_EVENT_RELEASED) {
        // 给窗口2的容器添加隐藏属性，清除窗口1的隐藏属性
        lv_obj_add_flag(Node_window, LV_OBJ_FLAG_HIDDEN);
        // lv_obj_add_flag(NFC_window, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(tabview, LV_OBJ_FLAG_HIDDEN);
    }
}

//NFC弹窗下拉列表获取下拉选项的回调函数
static void dropdown_nodeState_event_cb(lv_event_t *event) {
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    if (code == LV_EVENT_VALUE_CHANGED) {
        char selected_option[16];
        lv_dropdown_get_selected_str(ddlist_nodeState, selected_option, 16);

        char devEUI[24];
        strcpy(devEUI,read_NodeState());

        My_httpRequest_getNodeState(devEUI);

        // 根据选中的选项来更新结构体中的值
        if (strcmp(selected_option, "enable") == 0) {
            std::cout << " read_NodeState() = " << devEUI << std::endl;
            My_httpRequest_putNodeState(devEUI, false);
            std::cout << "节点状态————启用" << std::endl;
        } else if (strcmp(selected_option, "disable") == 0) {
            std::cout << " read_NodeState() = " << devEUI << std::endl;
            My_httpRequest_putNodeState(devEUI, true);
            std::cout << "节点状态————禁用" << std::endl;
        }
    }
}

//Node弹窗————填写数据
void Write_NodePopUP(const char*devName)
{
    //单例模式确保类只有一个实例化对象
    LoraNodeDeviceClass *LoraNode_ClassOBJ = LoraNodeDeviceClass::getInstance();
    LORA_NODEDEVICE_INFO* pNode = LoraNode_ClassOBJ->getNodeDeviceByDevName(devName);

    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();

    if (pNode != nullptr) {
        //节点设备详情界面 左边的value值
        lv_label_set_text_fmt(label_NodePopUp_left_Value[0], "%s", pNode->ANode_Info.devName);
        if(strcmp(pNode->ANode_Info.lastOnlineTime, "未通信") || strcmp(pNode->ANode_Info.lastOnlineTime, "Not communicated")) {
            if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                lv_label_set_text_fmt(label_NodePopUp_left_Value[1], "已激活");
            } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                lv_label_set_text_fmt(label_NodePopUp_left_Value[1], "Activated");
            } 
        }else{
            if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                lv_label_set_text_fmt(label_NodePopUp_left_Value[1], "未激活");
            } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                lv_label_set_text_fmt(label_NodePopUp_left_Value[1], "Not activated");
            }
        }
        lv_label_set_text_fmt(label_NodePopUp_left_Value[2], "%s", pNode->ANode_Info.Remark);
        lv_label_set_text_fmt(label_NodePopUp_left_Value[3], "%s", pNode->ANode_Info.devEUI);
        lv_label_set_text_fmt(label_NodePopUp_left_Value[4], "%s", pNode->ANode_Info.appEUI);
        lv_label_set_text_fmt(label_NodePopUp_left_Value[5], "%s %%",to_string(pNode->ANode_Info.Battery).c_str());
        //节点设备详情界面 右边的value值
        lv_label_set_text_fmt(label_NodePopUp_right_Value[0], "V%s", pNode->ANode_Info.MacVersion);
        lv_label_set_text_fmt(label_NodePopUp_right_Value[1], "Class-%s", pNode->ANode_Info.loraClass);
        lv_label_set_text_fmt(label_NodePopUp_right_Value[2], "%s", pNode->ANode_Info.joinType);
        if(pNode->dataNumbers == 0) {
            lv_label_set_text_fmt(label_NodePopUp_right_Value[3], "%s", pNode->ANode_Info.lastOnlineTime);
        } else {
            std::string activeTimeStr = printTimeDifference(pNode->timeTamp);
            lv_label_set_text_fmt(label_NodePopUp_right_Value[3], "%s", activeTimeStr.c_str());
        }
 
          if (pNode->RSSI < 0 && pNode->RSSI >= Signal_RSSI_A) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                    lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "优");
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                    lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "A");
                }
            } else if (pNode->RSSI < Signal_RSSI_A && pNode->RSSI > Signal_RSSI_B) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                    lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "良");
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                    lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "B");
                }
            } else if (pNode->RSSI < Signal_RSSI_B && pNode->RSSI > Signal_RSSI_C) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                    lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "中");
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                    lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "C");
                }
            } else if (pNode->RSSI < Signal_RSSI_C ) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                    lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "差");
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                    lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "D");
                }
            } else {
                lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "");
            }
        // lv_label_set_text_fmt(label_NodePopUp_right_Value[4], "良好");
    }







    write_NodeState(pNode->ANode_Info.devEUI);
}

//UI————搭建Node弹窗UI界面
void lora_NodeWindow(void){
    Node_window = lv_win_create(lv_scr_act(), 30);    //在当前屏幕上创建一个新的窗口
    lv_obj_t *header  = lv_win_get_header(Node_window);         //获取窗口的头部对象
    lv_obj_t *content = lv_win_get_content(Node_window);        //获取窗口的内容对象
    //------------------------------------------
    label_title_NodePupUp = lv_label_create(header);
    lv_obj_set_width(label_title_NodePupUp, 420);
    lv_label_set_text_fmt(label_title_NodePupUp,"节点详情");
    lv_obj_t *btn_close_NodeWin = lv_win_add_btn(Node_window, " ", 40);         //创建按钮
    //给按钮添加事件
    lv_obj_add_event_cb(btn_close_NodeWin, Event_Hidden_Node_Window, LV_EVENT_RELEASED, NULL);  //给关闭按钮添加事件

    //给标题和内容添加样式
    lv_obj_set_style_bg_color(content, lv_color_hex(0xB8DDFA), 0);
    lv_obj_clear_flag(Node_window, LV_OBJ_FLAG_SCROLLABLE); //禁用窗口的滚动条
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE); //禁用窗口中内容的滚动条

    //选项卡下的表格Table1
    lv_obj_t *obj1 = lv_obj_create(content);
    lv_obj_t *obj2 = lv_obj_create(content);
    lv_obj_set_size(obj1,245,250);
    lv_obj_set_size(obj2,205,210);
    lv_obj_set_style_radius(obj1, 15, LV_PART_MAIN);//设置圆角的半径属性（下拉列表框对象变为圆角）
    lv_obj_set_style_radius(obj2, 15, LV_PART_MAIN);
    lv_obj_align(obj1,LV_ALIGN_TOP_LEFT,-5,-5);
    lv_obj_align(obj2,LV_ALIGN_TOP_RIGHT,5,-5);
    lv_obj_clear_flag(obj1, LV_OBJ_FLAG_SCROLLABLE);  //取消对象的下滑滚动条
    lv_obj_clear_flag(obj2, LV_OBJ_FLAG_SCROLLABLE);  //取消对象的下滑滚动条
    lv_obj_set_style_border_color(obj1,lv_color_hex(0xff0000),LV_STATE_DEFAULT); //边框颜色
    lv_obj_set_style_border_color(obj2,lv_color_hex(0xff0000),LV_STATE_DEFAULT); //边框颜色

//左边的文本列表
    MF_create_customLabel_Init(&label_NodePopUp_left_KEY[0], &label_NodePopUp_left_Value[0], obj1, 70, 150, 27, 0, Node_Line_spacing*0-10, Node_ConfigInfo_leftKey[0], "");
    for (int i = 1; i < Node_left_rowNum; i++)
    {
        // 创建Node弹窗左边的属性条目
        MF_create_customLabel_Init(&label_NodePopUp_left_KEY[i], &label_NodePopUp_left_Value[i], obj1, 70, 150, 15, 0, Node_Line_spacing*i, Node_ConfigInfo_leftKey[i], "");

    }
    
    lv_obj_t *line_left[Node_left_rowNum];
    for (int i = 0; i < Node_left_rowNum; i++)
    {
        line_left[i] = lv_line_create(obj1);
    }
    static lv_point_t line_points1[] = {{0,Node_Line_spacing*0+17}, {210,Node_Line_spacing*0+17}};
    static lv_point_t line_points2[] = {{0,Node_Line_spacing*1+15}, {210,Node_Line_spacing*1+15}};
    static lv_point_t line_points3[] = {{0,Node_Line_spacing*2+15}, {210,Node_Line_spacing*2+15}};
    static lv_point_t line_points4[] = {{0,Node_Line_spacing*3+15}, {210,Node_Line_spacing*3+15}};
    static lv_point_t line_points5[] = {{0,Node_Line_spacing*4+15}, {210,Node_Line_spacing*4+15}};
    static lv_point_t line_points6[] = {{0,Node_Line_spacing*5+15}, {210,Node_Line_spacing*5+15}};
    lv_line_set_points(line_left[0], line_points1, 2);
    lv_line_set_points(line_left[1], line_points2, 2);
    lv_line_set_points(line_left[2], line_points3, 2);
    lv_line_set_points(line_left[3], line_points4, 2);
    lv_line_set_points(line_left[4], line_points5, 2);
    lv_line_set_points(line_left[5], line_points6, 2);

//右边的文本列表
    for (int i = 0; i < Node_right_rowNum; i++)
    {
        // 创建Node弹窗右边的属性条目
        MF_create_customLabel_Init(&label_NodePopUp_right_KEY[i], &label_NodePopUp_right_Value[i], obj2, 100, 100, 15, 0, Node_Line_spacing*i, Node_ConfigInfo_rightKey[i], "");

    }



    lv_obj_t *line_right[Node_right_rowNum];
    for (int i = 0; i < Node_right_rowNum; i++)
    {
        line_right[i] = lv_line_create(obj2);
    }
    static lv_point_t line_points1_right[] = {{0,Node_Line_spacing*0+20-5}, {170,Node_Line_spacing*0+20-5}};
    static lv_point_t line_points2_right[] = {{0,Node_Line_spacing*1+20-5}, {170,Node_Line_spacing*1+20-5}};
    static lv_point_t line_points3_right[] = {{0,Node_Line_spacing*2+20-5}, {170,Node_Line_spacing*2+20-5}};
    static lv_point_t line_points4_right[] = {{0,Node_Line_spacing*3+20-5}, {170,Node_Line_spacing*3+20-5}};
    static lv_point_t line_points5_right[] = {{0,Node_Line_spacing*4+20-5}, {170,Node_Line_spacing*4+20-5}};
    lv_line_set_points(line_right[0], line_points1_right, 2);
    lv_line_set_points(line_right[1], line_points2_right, 2);
    lv_line_set_points(line_right[2], line_points3_right, 2);
    lv_line_set_points(line_right[3], line_points4_right, 2);
    lv_line_set_points(line_right[4], line_points5_right, 2);

    label_NodePopUp_JoinStatus_KEY = lv_label_create(content);
    lv_obj_set_width(label_NodePopUp_JoinStatus_KEY, 100);
    lv_obj_set_height(label_NodePopUp_JoinStatus_KEY, 15);
    lv_obj_align_to(label_NodePopUp_JoinStatus_KEY, content, LV_ALIGN_BOTTOM_RIGHT, -85, -25);
    lv_label_set_text(label_NodePopUp_JoinStatus_KEY, "设置节点状态");


    //创建一个下拉列表
    ddlist_nodeState = lv_dropdown_create(content);
    //设置下拉列表对象的宽高
    lv_obj_set_height(ddlist_nodeState, 45);
    lv_obj_set_width(ddlist_nodeState, 90);
    lv_obj_align_to(ddlist_nodeState, content, LV_ALIGN_BOTTOM_RIGHT,-5,-5);
    //在下拉列表对象上添加一个字符标志
    lv_dropdown_set_symbol(ddlist_nodeState, LV_SYMBOL_DOWN);

    //在下拉列表中添加选项
    lv_dropdown_set_options(ddlist_nodeState, "enable\ndisable");
    lv_obj_set_style_text_font(ddlist_nodeState, &lv_font_montserrat_12,0);   // 使用字库中不同字号的字体

    // 设置选中选项后的回调函数
    lv_obj_add_event_cb(ddlist_nodeState, dropdown_nodeState_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

}



/**********************************************************************************
 *        RTD弹窗相关函数
 **********************************************************************************/
//RTD弹窗————返回按钮功能实现
static void RTDwin_Returnbtn_event_cb(lv_event_t* event)
{
    lv_event_code_t code = lv_event_get_code(event);
    if (code == LV_EVENT_CLICKED)
    {
        // 清空表格，并将字符数组RTDTabel_option全部赋值为空
        Clear_RTD_Table();
        memset(RTDTabel_option, '\0', sizeof(RTDTabel_option)); 
        // 隐藏RTD弹窗，显示主界面
        lv_obj_add_flag(RTD_Window, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(tabview, LV_OBJ_FLAG_HIDDEN);
    }
}

//RTD表格————清除表格
void Clear_RTD_Table(void)
{
    for(int index = 0; index<6; index++){
        for(int i = 0; i<6; i++){
        //对表格中的文本进行裁剪——>文本会自动换行，但是不会自动增加行的高度
        lv_table_add_cell_ctrl(RealTimeData_table, index, i, LV_TABLE_CELL_CTRL_TEXT_CROP);
        }
        if(index != 0){
            lv_table_set_cell_value_fmt(RealTimeData_table, index, 0, " ");
            lv_table_set_cell_value_fmt(RealTimeData_table, index, 1, " ");
            lv_table_set_cell_value_fmt(RealTimeData_table, index, 2, " ");
            lv_table_set_cell_value_fmt(RealTimeData_table, index, 3, " ");
            lv_table_set_cell_value_fmt(RealTimeData_table, index, 4, " ");
            lv_table_set_cell_value_fmt(RealTimeData_table, index, 5, " ");

            lv_obj_invalidate(RealTimeData_table);//自动刷新表格 ，不加这句就需要手动点击表格刷新
        }
    }
}


void Write_RTDPopUP(const char*devName)
{
    std::cout << "userUI.cpp——>1101——>线程4——————进入: 写RTDTable!" << std::endl;
    //单例模式确保类只有一个实例化对象
    LoraNodeDeviceClass *LoraNode_ClassOBJ = LoraNodeDeviceClass::getInstance();
    LORA_NODEDEVICE_INFO* pNode = LoraNode_ClassOBJ->getNodeDeviceByDevName(devName);
    if (pNode != nullptr) {

        lv_label_set_text_fmt(label_title_RTD,"%s-%s-%s",pNode->ANode_Info.devName, pNode->ANode_Info.devEUI, pNode->ANode_Info.Remark);

        //获取RTD数组中的元素个数
        int count = pNode->dataNumbers; //只有实时数据流来了自加数据个数
        int row = count;        //行编号
        if(row > 5){
            row = 5;
        }
        int index = 1;
        while (row != 0)	
        {
            for(int i = 0; i<6; i++){
                //对表格中的文本进行裁剪——>文本会自动换行，但是不会自动增加行的高度
                lv_table_add_cell_ctrl(RealTimeData_table, row, i, LV_TABLE_CELL_CTRL_TEXT_CROP);
            }
            row--;
            if(!strcmp(pNode->RTDArray_Info[row].packetType, "JoinR")){
                lv_table_set_cell_value(RealTimeData_table, index, 0, JoinRequest);
            }else if(!strcmp(pNode->RTDArray_Info[row].packetType, "JoinA")){
                lv_table_set_cell_value(RealTimeData_table, index, 0, JoinAccept);
            }else if(!strcmp(pNode->RTDArray_Info[row].packetType, "CUp")){
                lv_table_set_cell_value(RealTimeData_table, index, 0, ConfirmedDataUp);
            }else if(!strcmp(pNode->RTDArray_Info[row].packetType, "CDown")){
                lv_table_set_cell_value(RealTimeData_table, index, 0, ConfirmedDataDown);
            }else if(!strcmp(pNode->RTDArray_Info[row].packetType, "UncUp")){
                lv_table_set_cell_value(RealTimeData_table, index, 0, UnconfirmedDataUp);
            }else if(!strcmp(pNode->RTDArray_Info[row].packetType, "UncDown")){
                lv_table_set_cell_value(RealTimeData_table, index, 0, UnconfirmedDataDown);
            }else{
                lv_table_set_cell_value(RealTimeData_table, index, 0, "未知");
            }

            lv_table_set_cell_value_fmt(RealTimeData_table, index, 1, "%s", pNode->RTDArray_Info[row].Time);
            lv_table_set_cell_value_fmt(RealTimeData_table, index, 2, "%s", pNode->RTDArray_Info[row].devEUI);
            lv_table_set_cell_value_fmt(RealTimeData_table, index, 3, "%s", pNode->RTDArray_Info[row].SNR);
            lv_table_set_cell_value_fmt(RealTimeData_table, index, 4, "%s", pNode->RTDArray_Info[row].RSSI);
            lv_obj_invalidate(RealTimeData_table);//自动刷新表格 ，不加这句就需要手动点击表格刷新
            index++; 
        }
    }
    std::cout << "userUI.cpp——>1101——>线程4——————退出: 写RTDTableUI数据完毕!" << std::endl;
}


//RTD表格————创建空表
void Create_RTD_Table(void)
{
    int col_num        = 5;   //列数

    //设置表对象的行列数
    lv_table_set_row_cnt(RealTimeData_table, 5);
    lv_table_set_col_cnt(RealTimeData_table, col_num);
    //设置列宽(每个选项的宽度)
    lv_table_set_col_width(RealTimeData_table, 0, 80);
    lv_table_set_col_width(RealTimeData_table, 1, 100);
    lv_table_set_col_width(RealTimeData_table, 2, 160);
    lv_table_set_col_width(RealTimeData_table, 3, 65);
    lv_table_set_col_width(RealTimeData_table, 4, 70);

    for(int i = 0; i<6; i++){
        //对表格中的文本进行裁剪——>文本会自动换行，但是不会自动增加行的高度
        lv_table_add_cell_ctrl(RealTimeData_table, 0, i, LV_TABLE_CELL_CTRL_TEXT_CROP);
    }

    //设置单元格数据
    lv_table_set_cell_value(RealTimeData_table, 0, 0, "包类型");
    lv_table_set_cell_value(RealTimeData_table, 0, 1, "时间");     //第0行第1列设置为"status"字符串
    lv_table_set_cell_value(RealTimeData_table, 0, 2, "Deveui");
    lv_table_set_cell_value(RealTimeData_table, 0, 3, "SNR");
    lv_table_set_cell_value(RealTimeData_table, 0, 4, "RSSI");

    lv_obj_invalidate(RealTimeData_table);//自动刷新表格 ，不加这句就需要手动点击表格刷新
    //设置整张表对象的高度
    lv_obj_set_height(RealTimeData_table, 275);
}



//UI————搭建RTD弹窗UI界面
void lora_RTDWindow(void)
{
    // 1、创建实时数据流窗口
    RTD_Window = lv_win_create(lv_scr_act(), 40);    //在当前屏幕上创建一个新的窗口
    lv_obj_t *header  = lv_win_get_header(RTD_Window);         //获取窗口的头部对象
    lv_obj_set_style_bg_color(header, lv_color_hex(0xb8ddfa), LV_PART_MAIN);
    
    //2、在窗口的头部对象上创建按钮与标签
    lv_obj_t *btn_Return = lv_btn_create(header);
    lv_obj_t *img_return_PNG = lv_img_create(btn_Return);   //创建一个图片部件
    lv_img_set_src(img_return_PNG, &Return_png);  //设置图片源
    lv_img_set_zoom(img_return_PNG,64);
    lv_obj_center(img_return_PNG);
    lv_obj_set_size(btn_Return, 40, 40);
    lv_obj_add_event_cb(btn_Return, RTDwin_Returnbtn_event_cb, LV_EVENT_ALL, NULL); // 给对象添加CLICK事件和事件处理回调函数
    lv_obj_set_style_bg_color(btn_Return, lv_color_hex(0xffffff), 0);
    //------------------------------------------------
    label_title_RTD = lv_label_create(header);
    lv_obj_center(label_title_RTD);
    lv_label_set_text(label_title_RTD,"Name-DevEUI-RTD");

    //3、在窗口的主体上创建表格控件
    RealTimeData_table = lv_table_create(RTD_Window);    //在tab2上放置表格table2
    lv_obj_clear_flag(RealTimeData_table, LV_OBJ_FLAG_SCROLLABLE);  //取消表格的下滑滚动条
    Create_RTD_Table();    //创建实时数据流表格（空表，数据由另外的函数填写）
    lv_obj_align_to(RealTimeData_table, RTD_Window, LV_ALIGN_TOP_MID, 0, -5);
}



/**********************************************************************************
 *        NFC弹窗相关函数
 **********************************************************************************/
// 恢复出厂设置弹窗————>点击确认按钮————>弹出更新弹出的逻辑（只是更换了图片源而已）
static void lv_FactoryReset_Sure_event_cb(lv_event_t *event)
{
    std::cout << "userUI.cpp——>1222——>进入: 确认恢复出厂设置按钮的事件处理" << std::endl;
    lv_obj_add_flag(obj_base3, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
    lv_obj_clear_flag(obj_base2, LV_OBJ_FLAG_HIDDEN);

    lv_obj_add_flag(img_WartingTimeout, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
    lv_obj_add_flag(img_updateFlase, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
    lv_obj_add_flag(img_updateTrue, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
    lv_obj_add_flag(img_FactoryReset_True, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
    lv_obj_add_flag(img_FactoryReset_Flase, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

    lv_obj_clear_flag(img_updateing, LV_OBJ_FLAG_HIDDEN);//清空隐藏标志位=显示图片


    NFC_JsonDatAassembly(FactoryResetValue_True);

    // 创建一个线程，执行定时器逻辑
    std::thread timerThread(update_TimeoutJudgment);
    // 等待定时器线程执行完毕
    timerThread.detach();
    std::cout << "userUI.cpp——>1222——>退出: 确认恢复出厂设置按钮的事件处理" << std::endl;
}

// 恢复出厂设置弹窗————>点击关闭窗口/取消按钮————>关闭该弹窗
static void lv_FactoryResetWin_close_event_cb(lv_event_t *event)
{
    lv_obj_add_flag(obj_base3, LV_OBJ_FLAG_HIDDEN);
    //启用按钮
    lv_obj_clear_state(NFC_btn[0], LV_STATE_DISABLED);
    lv_obj_clear_state(NFC_btn[1], LV_STATE_DISABLED);
    lv_obj_clear_state(NFC_btn[2], LV_STATE_DISABLED);
}

// 
static void lv_NodeupdateWin_close_event_cb(lv_event_t *event)
{
    NFC_sendJsonData_Read();
    lv_obj_add_flag(obj_base2, LV_OBJ_FLAG_HIDDEN);
    //启用按钮
    lv_obj_clear_state(NFC_btn[0], LV_STATE_DISABLED);
    lv_obj_clear_state(NFC_btn[1], LV_STATE_DISABLED);
    lv_obj_clear_state(NFC_btn[2], LV_STATE_DISABLED);

    std::unique_lock<std::mutex> lock(mutex_NFC_Nodeupdate);
    signal_NFC_Nodeupdate = NFC_subWIN_Close; // 生成更新状态的信号
    cv_NFC_Nodeupdate.notify_all(); // 通知所有等待的线程
}

static void lv_NodeRegisterWin_close_event_cb(lv_event_t *event)
{
    lv_obj_add_flag(obj_base1, LV_OBJ_FLAG_HIDDEN);
    //启用按钮
    lv_obj_clear_state(NFC_btn[0], LV_STATE_DISABLED);
    lv_obj_clear_state(NFC_btn[1], LV_STATE_DISABLED);
    lv_obj_clear_state(NFC_btn[2], LV_STATE_DISABLED);
}

// 隐藏NFC窗口、显示主窗口
static void Event_Hidden_NFC_Window(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    if (code == LV_EVENT_RELEASED) {
        NFC_sendJsonData_Read();
        // 给窗口2的容器添加隐藏属性，清除窗口1的隐藏属性
        lv_obj_add_flag(obj_base2, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(NFC_window, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(tabview, LV_OBJ_FLAG_HIDDEN);
    }
}



// NFC弹窗下拉列表获取下拉选项的回调函数
static void dropdown0_event_cb(lv_event_t *event) {
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    if (code == LV_EVENT_VALUE_CHANGED) {
        char selected_option[16];
        lv_dropdown_get_selected_str(NFC_ddlist[0], selected_option, 16);

        // printf("%s\n",selected_option);

        // //花括号是一个局部变量空间，作用域结束之后锁解除
        // {
        //     std::lock_guard<std::mutex> lock(mutex_NFC_updatebtnStatus);
        //     std::cout << "发送——>恢复更新节点按钮可点击状态的信号" << std::endl;
        //     signal_NFC_updatebtnStatus = true;
        //     cv_NFC_updatebtnStatus.notify_one();
        // }


        // 根据选中的选项来更新结构体中的值
        if (strcmp(selected_option, "CLASS-A") == 0) {
            write_NFCInfo_loraClass("A");
        } else if (strcmp(selected_option, "CLASS-B") == 0) {
            write_NFCInfo_loraClass("B");
        } else if (strcmp(selected_option, "CLASS-C") == 0) {
            write_NFCInfo_loraClass("C");
        }
    }
}

static void dropdown1_event_cb(lv_event_t *event) {
    lv_event_code_t code = lv_event_get_code(event);    //获取事件类型
    if (code == LV_EVENT_VALUE_CHANGED) {
        char selected_option[16];
        lv_dropdown_get_selected_str(NFC_ddlist[1], selected_option, 16);

        // //花括号是一个局部变量空间，作用域结束之后锁解除
        // {
        //     std::lock_guard<std::mutex> lock(mutex_NFC_updatebtnStatus);
        //     std::cout << "发送——>恢复更新节点按钮可点击状态的信号" << std::endl;
        //     signal_NFC_updatebtnStatus = true;
        //     cv_NFC_updatebtnStatus.notify_one();
        // }

        // printf("%s\n",selected_option);
        // 根据选中的选项来更新结构体中的值
        if (strcmp(selected_option, "1min") == 0) {
            write_NFCInfo_uploadInterval(60000);
        } else if (strcmp(selected_option, "5min") == 0) {
            write_NFCInfo_uploadInterval(300000);
        } else if (strcmp(selected_option, "10min") == 0) {
            write_NFCInfo_uploadInterval(600000);
        }
    }
}


// 节点注册的弹窗
void registerNode_PopUp(void){
    obj_base1 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj_base1,400,200);
    lv_obj_center(obj_base1);
    lv_obj_set_style_radius(obj_base1, 25, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj_base1, lv_color_hex(0xffffff),0);
    lv_obj_set_style_border_color(obj_base1,lv_color_hex(0xa5a5a5),LV_STATE_DEFAULT); //边框颜色
    lv_obj_set_style_border_width(obj_base1,2,LV_STATE_DEFAULT); //边框宽度
    lv_obj_set_style_border_opa(obj_base1,50,LV_STATE_DEFAULT); //边框透明度

    lv_obj_t *btnClose = lv_btn_create(obj_base1);
    lv_obj_set_size(btnClose,20,20);
    lv_obj_align(btnClose,LV_ALIGN_TOP_RIGHT,-5,0);
    lv_obj_add_event_cb(btnClose, lv_NodeRegisterWin_close_event_cb, LV_EVENT_RELEASED, obj_base1);  //给关闭按钮添加事件

    img_RegisterTrue = lv_img_create(obj_base1);   //创建一个图片部件
    lv_img_set_src(img_RegisterTrue, &RegisterTrue);  //设置图片源
    lv_img_set_zoom(img_RegisterTrue,192);
    lv_obj_center(img_RegisterTrue);
    lv_obj_add_flag(img_RegisterTrue, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

    img_RegisterFlase = lv_img_create(obj_base1);   //创建一个图片部件
    lv_img_set_src(img_RegisterFlase, &English_RegisterFlase_PNG);  //设置图片源
    // lv_img_set_src(img_RegisterFlase, &RegisterFlase);  //设置图片源
    lv_img_set_zoom(img_RegisterFlase,192);
    lv_obj_center(img_RegisterFlase);
    lv_obj_add_flag(img_RegisterFlase, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

    img_RegisterFlase_IsExist = lv_img_create(obj_base1);   //创建一个图片部件
    lv_img_set_src(img_RegisterFlase_IsExist, &RegisterFlase_IsExist);  //设置图片源
    lv_img_set_zoom(img_RegisterFlase_IsExist,168);
    lv_obj_center(img_RegisterFlase_IsExist);
    lv_obj_add_flag(img_RegisterFlase_IsExist, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
}

// NFC弹窗————注册节点
static void network_event_cb(lv_event_t *e)
{
    std::cout << "userUI.cpp——>1388——>进入：注册节点事件处理" << std::endl;
    lv_event_code_t code = lv_event_get_code(e);    //获取事件类型
    if (code == LV_EVENT_CLICKED) {

        lv_obj_add_flag(img_RegisterTrue, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
        lv_obj_add_flag(img_RegisterFlase, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
        lv_obj_add_flag(img_RegisterFlase_IsExist, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
        lv_obj_clear_flag(obj_base1, LV_OBJ_FLAG_HIDDEN);

        //禁用按钮
        lv_obj_add_state(NFC_btn[0], LV_STATE_DISABLED);
        lv_obj_add_state(NFC_btn[1], LV_STATE_DISABLED);
        lv_obj_add_state(NFC_btn[2], LV_STATE_DISABLED);

        std::string Join_status = My_httpRequest_NodeRegistration();   //在这里面已经知道了 节点入网是否成功
        // std::cout << "反出函数的Join_status = " << Join_status << std::endl;
        if(Join_status == "Join_OK"){
            lv_obj_clear_flag(img_RegisterTrue, LV_OBJ_FLAG_HIDDEN);//清空隐藏标志位=显示图片
            std::cout << " 节点注册————注册成功" << std::endl;
            NFC_Register_sendSignal();  //3s后关闭节点注册的窗口
        }else if(Join_status == "Join_isExist"){
            std::cout << " 节点注册————注册失败（该设备已被注册）" << std::endl;
            lv_obj_clear_flag(img_RegisterFlase_IsExist, LV_OBJ_FLAG_HIDDEN);//清空隐藏标志位=显示图片
            NFC_Register_sendSignal();
        }else if(Join_status == "Join_argError"){
            std::cout << " 节点注册————注册失败（注册参数错误）" << std::endl;
            lv_obj_clear_flag(img_RegisterFlase, LV_OBJ_FLAG_HIDDEN);//清空隐藏标志位=显示图片
            NFC_Register_sendSignal();
        }else{
            std::cout << " 节点注册————注册失败（不明原因）" << std::endl;
            lv_obj_clear_flag(img_RegisterFlase, LV_OBJ_FLAG_HIDDEN);//清空隐藏标志位=显示图片
            NFC_Register_sendSignal();
        }
    }
    std::cout << "userUI.cpp——>1388——>退出：注册节点事件处理" << std::endl;
}



// 节点更新的弹窗
void updateNode_PopUp(void){
    obj_base2 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj_base2,400,200);
    lv_obj_center(obj_base2);
    lv_obj_set_style_radius(obj_base2, 25, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj_base2, lv_color_hex(0xffffff),0);
    lv_obj_set_style_border_color(obj_base2,lv_color_hex(0xa5a5a5),LV_STATE_DEFAULT); //边框颜色
    lv_obj_set_style_border_width(obj_base2,2,LV_STATE_DEFAULT); //边框宽度
    lv_obj_set_style_border_opa(obj_base2,50,LV_STATE_DEFAULT); //边框透明度

    lv_obj_t *btnClose = lv_btn_create(obj_base2);
    lv_obj_set_size(btnClose,20,20);
    lv_obj_align(btnClose,LV_ALIGN_TOP_RIGHT,-5,0);
    lv_obj_add_event_cb(btnClose, lv_NodeupdateWin_close_event_cb, LV_EVENT_RELEASED, obj_base2);  //给关闭按钮添加事件

    img_updateTrue = lv_img_create(obj_base2);   //创建一个图片部件
    lv_img_set_src(img_updateTrue, &UpdateSucceed_PNG);  //设置图片源
    lv_img_set_zoom(img_updateTrue,192);
    lv_obj_center(img_updateTrue);
    lv_obj_add_flag(img_updateTrue, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

    img_updateFlase = lv_img_create(obj_base2);   //创建一个图片部件
    lv_img_set_src(img_updateFlase, &updateFlase);  //设置图片源
    lv_img_set_zoom(img_updateFlase,192);
    lv_obj_center(img_updateFlase);
    lv_obj_add_flag(img_updateFlase, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

    img_WartingTimeout = lv_img_create(obj_base2);   //创建一个图片部件
    lv_img_set_src(img_WartingTimeout, &WartingTimeout);  //设置图片源
    lv_img_set_zoom(img_WartingTimeout,256);
    lv_obj_center(img_WartingTimeout);
    lv_obj_add_flag(img_WartingTimeout, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

    img_updateing = lv_img_create(obj_base2);   //创建一个图片部件
    lv_img_set_src(img_updateing, &Updating);  //设置图片源
    lv_img_set_zoom(img_updateing,168);
    lv_obj_center(img_updateing);
    // lv_obj_add_flag(img_updateing, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

    // lv_obj_clear_flag(ui_Image5, LV_OBJ_FLAG_HIDDEN);//清空隐藏标志位=显示图片
}





//定时器线程回调函数，发送节点更新超时信息
static void update_TimeoutJudgment(void)
{
    std::cout << "userUI.cpp——>1477——>进入: 节点更新的定时器事件" << std::endl;
    // 执行超时操作
    std::unique_lock<std::mutex> lock(mutex_NFC_Nodeupdate);
    // 会在等待过程中，最多等待10秒的时间，如果在10秒内条件满足，那么等待会提前结束；如果超过10秒仍未满足条件，那么等待会自动结束
    cv_NFC_Nodeupdate2.wait_for(lock, std::chrono::seconds(10));    
    // std::cout << "____________________________定时器signal_NFC_Nodeupdate = " << signal_NFC_Nodeupdate << std::endl;
    if(signal_NFC_Nodeupdate == NFC_update_TimeOut || signal_NFC_Nodeupdate == 0){
        NFC_sendJsonData_Read();
        std::cout << "NFC————更新超时————切换至读模式" << std::endl;

        lv_obj_add_flag(img_updateing, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(img_updateFlase, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(img_updateTrue, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(img_FactoryReset_Flase, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(img_FactoryReset_True, LV_OBJ_FLAG_HIDDEN);// 置位隐藏标志位=隐藏图片

        lv_obj_clear_flag(img_WartingTimeout, LV_OBJ_FLAG_HIDDEN);// 清空隐藏标志位=显示图片

        signal_NFC_Nodeupdate = NFC_update_TimeOut;
        NFC_Update_sendSignal();    //3s后关闭节点更新的窗口
    }else{
        // std::cout << "signal_NFC_Nodeupdate " << signal_NFC_Nodeupdate << std::endl;
        signal_NFC_Nodeupdate = NFC_update_TimeOut;
    }
    // std::cout << "定时器线程执行完毕, 内存资源已被释放！" << std::endl;
    std::cout << "userUI.cpp——>1477——>退出: 节点更新的定时器事件" << std::endl;
}




//NFC弹窗————更新节点
static void updateNode_event_cb(lv_event_t *e)
{
    std::cout << "userUI.cpp——>1511——>进入：更新节点事件处理" << std::endl;
    lv_event_code_t code = lv_event_get_code(e);    //获取事件类型
    if (code == LV_EVENT_CLICKED) {

        lv_obj_add_flag(img_WartingTimeout, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
        lv_obj_add_flag(img_updateFlase, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
        lv_obj_add_flag(img_updateTrue, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
        lv_obj_add_flag(img_FactoryReset_Flase, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片
        lv_obj_add_flag(img_FactoryReset_True, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

        lv_obj_clear_flag(img_updateing, LV_OBJ_FLAG_HIDDEN);//清空隐藏标志位=显示图片
        lv_obj_clear_flag(obj_base2, LV_OBJ_FLAG_HIDDEN);


        //禁用按钮
        lv_obj_add_state(NFC_btn[0], LV_STATE_DISABLED);
        lv_obj_add_state(NFC_btn[1], LV_STATE_DISABLED);
        lv_obj_add_state(NFC_btn[2], LV_STATE_DISABLED);

        NFC_JsonDatAassembly(FactoryResetValue_Flase); //通过wss发送更新节点信息

        // 创建一个线程，执行定时器逻辑
        std::thread timerThread(update_TimeoutJudgment);
        // 等待定时器线程执行完毕(线程执行完毕后自动释放内存资源)
        timerThread.detach();
    }
    std::cout << "userUI.cpp——>1511——>退出：更新节点事件处理" << std::endl;
}



//节点恢复出厂设置的弹窗
void FactoryReset_PopUp(void){

    obj_base3 = lv_obj_create(lv_scr_act());
    lv_obj_set_size(obj_base3,400,200);
    lv_obj_center(obj_base3);
    lv_obj_set_style_radius(obj_base3, 25, LV_PART_MAIN);
    lv_obj_set_style_bg_color(obj_base3, lv_color_hex(0xffffff),0);
    lv_obj_set_style_border_color(obj_base3,lv_color_hex(0xa5a5a5),LV_STATE_DEFAULT); //边框颜色
    lv_obj_set_style_border_width(obj_base3,2,LV_STATE_DEFAULT); //边框宽度
    lv_obj_set_style_border_opa(obj_base3,50,LV_STATE_DEFAULT); //边框透明度

    lv_obj_t *btnClose = lv_btn_create(obj_base3);
    lv_obj_set_size(btnClose,20,20);
    lv_obj_align(btnClose,LV_ALIGN_TOP_RIGHT,-5,0);
    lv_obj_add_event_cb(btnClose, lv_FactoryResetWin_close_event_cb, LV_EVENT_RELEASED, obj_base3);  //给关闭按钮添加事件

    img_FactoryReset = lv_img_create(obj_base3);   //创建一个图片部件
    lv_img_set_src(img_FactoryReset, &FactoryReset_PNG);  //设置图片源
    lv_img_set_zoom(img_FactoryReset,168);
    // lv_obj_center(FactoryReset);
    lv_obj_align_to(img_FactoryReset, obj_base3, LV_ALIGN_TOP_MID, 0, 25);


    lv_obj_t *btnCancel = lv_btn_create(obj_base3);
    lv_obj_align(btnCancel,LV_ALIGN_BOTTOM_LEFT,70,-15);
    lv_obj_set_size(btnCancel,100,50);
    lv_obj_set_style_bg_color(btnCancel, lv_color_hex(0xffffff),0);
    lv_obj_set_style_border_color(btnCancel,lv_color_hex(0x2094f0),LV_STATE_DEFAULT); //边框颜色
    lv_obj_set_style_border_width(btnCancel,2,LV_STATE_DEFAULT); //边框宽度
    lv_obj_add_event_cb(btnCancel, lv_FactoryResetWin_close_event_cb, LV_EVENT_RELEASED, obj_base3);  //给取消按钮添加事件
    lv_obj_t *label_Cancel = lv_label_create(btnCancel);
    lv_label_set_recolor( label_Cancel, true ); 						/* 开启重新着色功能 */
    lv_label_set_text( label_Cancel, "#2094f0 Cancel#");			/* 单独设置颜色 */
    lv_obj_center(label_Cancel);

    lv_obj_t *btnOK = lv_btn_create(obj_base3);
    lv_obj_align(btnOK,LV_ALIGN_BOTTOM_RIGHT,-70,-15);
    lv_obj_set_size(btnOK,100,50);
    lv_obj_add_event_cb(btnOK, lv_FactoryReset_Sure_event_cb, LV_EVENT_RELEASED, obj_base3);  //给取消按钮添加事件
    lv_obj_t *label_OK = lv_label_create(btnOK);
    lv_label_set_text(label_OK,"OK");
    // lv_obj_set_style_text_font(label_OK, &lv_font_montserrat_16,0);   // 使用字库中不同字号的字体
    lv_obj_center(label_OK);



    img_FactoryReset_True = lv_img_create(obj_base2);   //创建一个图片部件
    lv_img_set_src(img_FactoryReset_True, &FactoryReset_True);  //设置图片源
    lv_img_set_zoom(img_FactoryReset_True,192);
    lv_obj_center(img_FactoryReset_True);
    lv_obj_add_flag(img_FactoryReset_True, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

    img_FactoryReset_Flase = lv_img_create(obj_base2);   //创建一个图片部件
    lv_img_set_src(img_FactoryReset_Flase, &FactoryReset_Flase);  //设置图片源
    lv_img_set_zoom(img_FactoryReset_Flase,198);
    lv_obj_center(img_FactoryReset_Flase);
    lv_obj_add_flag(img_FactoryReset_Flase, LV_OBJ_FLAG_HIDDEN);//置位隐藏标志位=隐藏图片

}



//NFC弹窗————恢复出厂设置
static void FactoryReset_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);    //获取事件类型
    if (code == LV_EVENT_CLICKED) {

        lv_obj_clear_flag(obj_base3, LV_OBJ_FLAG_HIDDEN);
        //禁用按钮
        lv_obj_add_state(NFC_btn[0], LV_STATE_DISABLED);
        lv_obj_add_state(NFC_btn[1], LV_STATE_DISABLED);
        lv_obj_add_state(NFC_btn[2], LV_STATE_DISABLED);

    }
}


//给NFC弹窗写数据
void Write_NFCPopUP()
{
    std::cout << "userUI.cpp——>1624——>进入: 填写NFC弹窗数据" << std::endl;
    // 禁用按钮
    // lv_btn_set_state(NFC_btn, LV_BTN_STYLE_INA );
    // lv_btn_set_state(NFC_btn, LV_BTN_STATE_DISABLED);

    LORA_NFC_INFO  NFC_Info = read_NFCInfo();
    
    // std::cout << NFC_Info.altitude << std::endl;
    // std::cout << NFC_Info.confirm << std::endl;
    // std::cout << NFC_Info.devAddr << std::endl;
    // std::cout << NFC_Info.name << std::endl;
    // std::cout << NFC_Info.devEUI << std::endl;

    lv_label_set_text_fmt(label_title_NFC,"%s-%s",NFC_Info.name, NFC_Info.devEUI);
    lv_label_set_text_fmt(label_NFCPopUp_left_Value[0], "%s", NFC_Info.name);
    lv_label_set_text_fmt(label_NFCPopUp_left_Value[1], "%s", NFC_Info.product);
    lv_label_set_text_fmt(label_NFCPopUp_left_Value[2], "%s", NFC_Info.devEUI);
    lv_label_set_text_fmt(label_NFCPopUp_left_Value[3], "%s", NFC_Info.joinEUI);


    lv_label_set_text_fmt(label_NFCPopUp_right_Value[0], "%s", NFC_Info.macVersion);
    lv_label_set_text_fmt(label_NFCPopUp_right_Value[1], "%s", NFC_Info.joinType);



    //在下拉列表中添加选项
    lv_dropdown_set_options(NFC_ddlist[0], "CLASS-A\nCLASS-B\nCLASS-C\n...");
    lv_dropdown_set_options(NFC_ddlist[1], "1min\n5min\n10min\n...");
    lv_dropdown_set_options(NFC_ddlist[2], "ON\nOFF\n...");

    // 设置选中选项后的回调函数
    lv_obj_add_event_cb(NFC_ddlist[0], dropdown0_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(NFC_ddlist[1], dropdown1_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    // lv_obj_add_event_cb(NFC_ddlist[2], dropdown2_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    //修改选项字符串的颜色
    // lv_obj_set_style_text_color(dropdown, lv_color_hex(0xFF0000), LV_PART_MAIN);


    if(!strncmp(NFC_Info.loraClass, "A", 1)){
        lv_dropdown_set_selected(NFC_ddlist[0], 0);
    }else if(!strncmp(NFC_Info.loraClass, "B", 1)){
        lv_dropdown_set_selected(NFC_ddlist[0], 1);
    }else if(!strncmp(NFC_Info.loraClass, "C", 1)){
        lv_dropdown_set_selected(NFC_ddlist[0], 2);
    }

    // std::cout << "NFC_Info.uploadInterval = " << NFC_Info.uploadInterval << std::endl;
    if(NFC_Info.uploadInterval == 60000){
        lv_dropdown_set_selected(NFC_ddlist[1], 0);
    }else if(NFC_Info.uploadInterval == 300000){
        lv_dropdown_set_selected(NFC_ddlist[1], 1);
    }else if(NFC_Info.uploadInterval == 600000){
        lv_dropdown_set_selected(NFC_ddlist[1], 2);
    }else{
        lv_dropdown_set_selected(NFC_ddlist[1], 3);
    }
    std::cout << "userUI.cpp——>1624——>退出: 填写NFC弹窗数据" << std::endl;
}



//UI————搭建NFC弹窗UI界面
void lora_NFCWindow(void) {
    //1、设计NFC弹窗的头部控件（标签+按钮）
    NFC_window = lv_win_create(lv_scr_act(), 30);    //在当前屏幕上创建一个新的窗口
    lv_obj_t *header  = lv_win_get_header(NFC_window);         //获取窗口的头部对象
    lv_obj_t *content = lv_win_get_content(NFC_window);        //获取窗口的内容对象
    //------------------------------------------
    label_title_NFC = lv_label_create(header);
    lv_obj_set_width(label_title_NFC, 420);
    lv_label_set_text_fmt(label_title_NFC,"Name-DevEUI-NFC");
    lv_obj_t *btn_close_NFCWin = lv_win_add_btn(NFC_window, " ", 30);         //创建按钮

    //给标题和内容添加样式
    lv_obj_set_style_bg_color(content, lv_color_hex(0xB8DDFA), 0);
    lv_obj_clear_flag(NFC_window, LV_OBJ_FLAG_SCROLLABLE); //禁用窗口的滚动条
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE); //禁用窗口中内容的滚动条

    //选项卡下的表格Table1
    lv_obj_t *obj1 = lv_obj_create(content);
    lv_obj_t *obj2 = lv_obj_create(content);
    lv_obj_set_size(obj1,240,210);
    lv_obj_set_size(obj2,210,210);
    lv_obj_set_style_radius(obj1, 15, LV_PART_MAIN);//设置圆角的半径属性（下拉列表框对象变为圆角）
    lv_obj_set_style_radius(obj2, 15, LV_PART_MAIN);
    lv_obj_align(obj1,LV_ALIGN_TOP_LEFT,-5,-5);
    lv_obj_align(obj2,LV_ALIGN_TOP_RIGHT,5,-5);
    lv_obj_clear_flag(obj1, LV_OBJ_FLAG_SCROLLABLE);  //取消对象的下滑滚动条
    lv_obj_clear_flag(obj2, LV_OBJ_FLAG_SCROLLABLE);  //取消对象的下滑滚动条
    lv_obj_set_style_border_color(obj1,lv_color_hex(0xff0000),LV_STATE_DEFAULT); //边框颜色
    lv_obj_set_style_border_color(obj2,lv_color_hex(0xff0000),LV_STATE_DEFAULT); //边框颜色

//NFC弹窗左边的文本列表
     for (int i = 0; i < NFC_left_rowNum; i++)
    {
        // 创建Node弹窗右边的属性条目
        MF_create_customLabel_Init(&label_NFCPopUp_left_KEY[i], &label_NFCPopUp_left_Value[i], obj1, 70, 140, 15, 0, NFC_left_LineSpacing*i, NFC_ConfigInfo_leftKey[i], "");

    }

    static lv_point_t line_points1[] = {{0,NFC_left_LineSpacing*0+20-5}, {210,NFC_left_LineSpacing*0+20-5}};
    static lv_point_t line_points2[] = {{0,NFC_left_LineSpacing*1+20-5}, {210,NFC_left_LineSpacing*1+20-5}};
    static lv_point_t line_points3[] = {{0,NFC_left_LineSpacing*2+20-5}, {210,NFC_left_LineSpacing*2+20-5}};
    static lv_point_t line_points4[] = {{0,NFC_left_LineSpacing*3+20-5}, {210,NFC_left_LineSpacing*3+20-5}};
    lv_obj_t *line_left[NFC_left_rowNum];
    for (int i = 0; i < NFC_left_rowNum; i++)
    {
        line_left[i] = lv_line_create(obj1);
    }
    lv_line_set_points(line_left[0], line_points1, 2);
    lv_line_set_points(line_left[1], line_points2, 2);
    lv_line_set_points(line_left[2], line_points3, 2);
    lv_line_set_points(line_left[3], line_points4, 2);

//NFC弹窗右边的文本列表
    for (int i = 0; i < NFC_right_rowNum; i++)
    {
        label_NFCPopUp_right_KEY[i] = lv_label_create(obj2);    //将标签创建在窗口的内容部分上
        lv_obj_set_width(label_NFCPopUp_right_KEY[i], 100);
        lv_obj_set_height(label_NFCPopUp_right_KEY[i], 15);
        lv_obj_align_to(label_NFCPopUp_right_KEY[i], obj2, LV_ALIGN_TOP_LEFT, 0, NFC_right_LineSpacing*i+1);
        lv_label_set_text(label_NFCPopUp_right_KEY[i], NFC_ConfigInfo_rightKey[i]);
    }

    lv_obj_t *line_right[NFC_right_rowNum];
    for (int i = 0; i < NFC_right_rowNum; i++)
    {
        line_right[i] = lv_line_create(obj2);
    }
    static lv_point_t line_points1_right[] = {{0,NFC_right_LineSpacing*0+20-5}, {180,NFC_right_LineSpacing*0+20-5}};
    static lv_point_t line_points2_right[] = {{0,NFC_right_LineSpacing*1+20-5}, {180,NFC_right_LineSpacing*1+20-5}};
    static lv_point_t line_points3_right[] = {{0,NFC_right_LineSpacing*2+20-5}, {180,NFC_right_LineSpacing*2+20-5}};
    static lv_point_t line_points4_right[] = {{0,NFC_right_LineSpacing*3+20-5}, {180,NFC_right_LineSpacing*3+20-5}};
    static lv_point_t line_points5_right[] = {{0,NFC_right_LineSpacing*4+20-5}, {180,NFC_right_LineSpacing*4+20-5}};
    lv_line_set_points(line_right[0], line_points1_right, 2);
    lv_line_set_points(line_right[1], line_points2_right, 2);
    lv_line_set_points(line_right[2], line_points3_right, 2);
    lv_line_set_points(line_right[3], line_points4_right, 2);
    lv_line_set_points(line_right[4], line_points5_right, 2);



    for (int i = 0; i < 5; i++)
    {
        if(i < 2){
            label_NFCPopUp_right_Value[i] = lv_label_create(obj2);
            lv_obj_set_height(label_NFCPopUp_right_Value[i], 30);
            lv_obj_set_width(label_NFCPopUp_right_Value[i], 90);
            lv_obj_align_to(label_NFCPopUp_right_Value[i], obj2, LV_ALIGN_TOP_LEFT, 105, NFC_right_LineSpacing*i);
        }else{
            //创建一个下拉列表
            NFC_ddlist[i-2] = lv_dropdown_create(obj2);
            //设置下拉列表对象的宽高
            lv_obj_set_height(NFC_ddlist[i-2], 30);
            lv_obj_set_width(NFC_ddlist[i-2], 90);
            //设置文本对齐方式
            // lv_obj_set_style_text_align(NFC_ddlist[i], LV_TEXT_ALIGN_, LV_PART_MAIN);
            //在下拉列表对象上添加一个字符标志
            // lv_dropdown_set_symbol(NFC_ddlist[i], "▼");
            //在窗口内容上设置下拉列表对象的位置
            lv_obj_align_to(NFC_ddlist[i-2], obj2, LV_ALIGN_TOP_LEFT, 90, NFC_right_LineSpacing*i-15);
            //设置圆角的半径属性（下拉列表框对象变为圆角）
            // lv_obj_set_style_radius(NFC_ddlist[i], LV_PCT(1), LV_PART_MAIN);
            lv_obj_set_style_text_font(NFC_ddlist[i-2], &lv_font_montserrat_12,0);   // 使用字库中不同字号的字体
        }
    }


    //创建三个按钮、三个标签   （窗口内容上挂按钮 按钮上挂标签）
    for (int i = 0; i < 3; i++)
    {
        NFC_btn[i] = lv_btn_create(content);
        lv_obj_set_height(NFC_btn[i], 45);
        lv_obj_set_width(NFC_btn[i], 100);
        label_NFC_btn[i] = lv_label_create(NFC_btn[i]); //将标签放置在按钮上
        lv_obj_center(label_NFC_btn[i]);
    }
    lv_obj_align_to(NFC_btn[0], content, LV_ALIGN_BOTTOM_LEFT, 15, 0);
    lv_obj_align_to(NFC_btn[1], content, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_align_to(NFC_btn[2], content, LV_ALIGN_BOTTOM_RIGHT, -15, 0);

    lv_label_set_text(label_NFC_btn[0], "注册节点");
    lv_label_set_text(label_NFC_btn[1], "更新节点");
    lv_label_set_text(label_NFC_btn[2], "恢复出厂设置");

    //给按钮添加事件
    lv_obj_add_event_cb(btn_close_NFCWin, Event_Hidden_NFC_Window, LV_EVENT_RELEASED, NULL);  //给关闭按钮添加事件
    lv_obj_add_event_cb(NFC_btn[0], network_event_cb, LV_EVENT_CLICKED,NULL);//给按键1添加事件（事件回调函数）
    lv_obj_add_event_cb(NFC_btn[1], updateNode_event_cb, LV_EVENT_CLICKED,NULL);//给按键2添加事件（事件回调函数）
    lv_obj_add_event_cb(NFC_btn[2], FactoryReset_event_cb, LV_EVENT_CLICKED,NULL);//给按键3添加事件（事件回调函数）
}




/**********************************************************************************
 *        mqtt请求部分
 **********************************************************************************/
// 回调函数，当连接成功时会被调用
void on_connect(struct mosquitto *mosq, void *obj, int reason_code) {
    // 在此处订阅topic
    mosquitto_subscribe(mosq, nullptr, "postgresql/device/update/push", 0);
    mosquitto_subscribe(mosq, nullptr, "/system/event/event_eth0_change", 0);
    mosquitto_subscribe(mosq, nullptr, "/system/event/event_cellular_change", 0);
    mosquitto_subscribe(mosq, nullptr, "/system/event/event_wan_change", 0);
}


// 回调函数，当收到订阅消息时会被调用
void on_message(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    bool update = false;
    // 假设消息是一个字符串，我们检查是否包含"update :true"
    if(message->payloadlen) {
        printf("Received message: %s\n", (char *)message->payload);
        // std::string payload((char*)message->payload, message->payloadlen);
        // if (payload.find("update :true") != std::string::npos) {
            // update = true;
            // 处理更新逻辑
            printf("Update requested!\n");
            // Mqtt监测到节点设备发生变化后，才开始get节点设备信息
            {
                std::lock_guard<std::mutex> lock(mutex_Mqtt);
                modified_Mqtt = true;
                cv_Mqtt.notify_one();  
                std::cout << "数据库中节点信息发生变化————lcd get节点设备&更新网络信息信号————发出" << std::endl;
            }
        // }
    } else {
        printf("Received message with empty payload\n");
    }
}


// 回调函数，当连接丢失时会被调用
void on_disconnect(struct mosquitto *mosq, void *obj, int rc) {
    std::cout << "MQTT 意外 Disconnection, 订阅者未主动断开! 代理服务器-订阅者—>重新启用后依然能正常进行MQTT通信!" << std::endl;
}

int Mqtt_ListenPostgresqlUpdate(void) {
    const char *host = "localhost";
    int port = 1883;
    int keepalive = 60;

    // 初始化mosquitto库
    mosquitto_lib_init();

    // 创建新的mosquitto客户端实例
    struct mosquitto *mosq = mosquitto_new(nullptr, true, nullptr);
    if (!mosq) {
        fprintf(stderr, "Failed to create mosquitto instance\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    // 分配连接和消息回调
    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);

    // 尝试连接到MQTT代理（首次连接成功之后才会进入到以下消息循环）
    while (mosquitto_connect(mosq, host, port, keepalive)) {
        fprintf(stderr, "MQTT Unable to connect, retrying in 5 seconds...\n");
        std::this_thread::sleep_for(std::chrono::seconds(5));  // 退避策略
    }

    // 启动消息循环
    mosquitto_loop_start(mosq);

    std::cout << "MQTT连接代理服务器成功!" << std::endl;
 
    while(true) {
        getchar();
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    
    

    // 清理
    mosquitto_loop_stop(mosq, true);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();

    std::cout << "userUI.cpp——>1904——>MQTT相关资源已被清理" << std::endl;
    return 0;
}









/**********************************************************************************
 *        各种线程函数
 **********************************************************************************/
// 用于获取线程的唯一 ID 的函数
pid_t getThreadId() {
    return syscall(SYS_gettid);
}


//线程1————获取token/GatewayID/服务器的状态/token有效性/Node信息
void thread1()
{
    pid_t threadId = getThreadId();
    std::cout << "Thread1 with ID: " << threadId << std::endl;
    My_httpRequest_GetToken();
}


//线程2————线程1填写完Node信息发送信号给线程2用于更新Node信息表格
void thread2()
{
    bool once_temp = true;
    pid_t threadId = getThreadId();
    std::cout << "Thread2 with ID: " << threadId << std::endl;
    while(1){
        std::unique_lock<std::mutex> lock(mutex_NodeTabel);
        cv_NodeTabel.wait(lock, [](){ return modified_Node; });// 等待条件变量满足条件
        modified_Node = false;
        MFunc_updateMainPage();
        if(once_temp == true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            MFunc_updateMainPage();
            once_temp = false;
        }
        std::cout << "Node表格————更新表格信号————接收" << std::endl;
    }
}


//线程3————RTD建立wss长连接获取实时数据流
void thread3()
{
    pid_t threadId = getThreadId();
    std::cout << "Thread3 with ID: " << threadId << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));   //断开重连处理（完成）
    while(1){
        if(read_ServerStatus() == ServerStatus_True){   //只有当http成功连接上服务器并成功获取得到token后才开始建立wss长连接
            std::this_thread::sleep_for(std::chrono::seconds(2));
            break;
        }else{
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    My_wssRequest_RTD();
}


//线程4————线程3填写完Node中的RTD信息后发送信号给线程4用于更新RTD弹窗中的RTD表格
void thread4()
{
    pid_t threadId = getThreadId();
    std::cout << "Thread4 with ID: " << threadId << std::endl;
    while(1){
        std::unique_lock<std::mutex> lock(mutex_RTDTabel);
        cv_RTDTabel.wait(lock, [](){ return modified_RTD; });// 等待条件变量满足条件
        modified_RTD = false;
        MFunc_updateMainPage();
        // 判断字符数组是否为空（点击了单元格后，RTDTabel_option不应该为空，当关闭RTD弹窗后，应当设置为空）
        if (strlen(RTDTabel_option) != 0) {
            Write_RTDPopUP(RTDTabel_option);
            //清空参数RTDTabel_option放在了RTD弹窗关闭之后
        }
        std::cout << "RTD表格————更新表格信号————接收" << std::endl;
    }
}


void thread5()
{   
    pid_t threadId = getThreadId();
    std::cout << "Thread5 with ID: " << threadId << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));   //断开重连处理（完成）
    while(1){
        if(read_ServerStatus() == ServerStatus_True){   //只有当http成功连接上服务器并成功获取得到token后才开始建立wss长连接
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "线程5:NFC的wss长连接建立成功" << std::endl;
            break;
        }else{
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    My_wssRequest_NFC();
}


// 用于显示NFC弹窗信息
void thread6()
{
    pid_t threadId = getThreadId();
    std::cout << "Thread6 with ID: " << threadId << std::endl;
    while(1){
        std::unique_lock<std::mutex> lock(mutex_NFC);
        cv_NFC.wait(lock, [](){ return isNFCModified; });// 等待条件变量满足条件
        isNFCModified = false;

        // 给窗口tabview的添加隐藏属性，清除窗口3的隐藏属性
        lv_obj_add_flag(tabview, LV_OBJ_FLAG_HIDDEN);        
        lv_obj_add_flag(Node_window, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(NFC_window, LV_OBJ_FLAG_HIDDEN);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        lv_obj_clear_flag(NFC_window, LV_OBJ_FLAG_HIDDEN);

        lock.unlock(); // 解锁互斥锁，以便其他线程可以同时访问 NFC
        Write_NFCPopUP();
        std::cout << "NFC弹窗————更新弹窗信号————接收" << std::endl;
    }
}



// 等待信号的值不为0且不为4时继续等待。当满足条件时，线程会被唤醒，并继续执行后续的代码。这段代码的目的是等待信号的到来，然后继续执行后续的操作

// 用于显示更新弹窗（更新成功、更新失败、恢复出厂设置成功、恢复出厂设置失败）
void thread7()
{
    pid_t threadId = getThreadId();
    std::cout << "Thread7 with ID: " << threadId << std::endl;
    while (true)
    {
        std::cout << "userUI.cpp——>2048——>更新/恢复弹窗(结果图片)" << std::endl;
        {
            std::unique_lock<std::mutex> lock(mutex_NFC_Nodeupdate);
            cv_NFC_Nodeupdate.wait(lock, []{ return signal_NFC_Nodeupdate != 0 && signal_NFC_Nodeupdate != 4;}); // 等待信号的到来(不是0和4的信号都会往下执行)
            std::cout << "更新/恢复的信号参数: signal_NFC_Nodeupdate = " << signal_NFC_Nodeupdate << std::endl;
            if (signal_NFC_Nodeupdate == NFC_update_True)
            {
                lv_obj_add_flag(img_WartingTimeout, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(img_updateFlase, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(img_updateing, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(img_updateTrue, LV_OBJ_FLAG_HIDDEN);
            }
            else if (signal_NFC_Nodeupdate == NFC_update_False)
            {
                lv_obj_add_flag(img_WartingTimeout, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(img_updateTrue, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(img_updateing, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(img_updateFlase, LV_OBJ_FLAG_HIDDEN);
            }
            else if (signal_NFC_Nodeupdate == NFC_FactoryReset_True)
            {
                lv_obj_add_flag(img_WartingTimeout, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(img_updateFlase, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(img_updateing, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(img_FactoryReset_True, LV_OBJ_FLAG_HIDDEN);
            }
            else if (signal_NFC_Nodeupdate == NFC_FactoryReset_False)
            {
                lv_obj_add_flag(img_WartingTimeout, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(img_updateTrue, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(img_updateing, LV_OBJ_FLAG_HIDDEN);
                lv_obj_clear_flag(img_FactoryReset_Flase, LV_OBJ_FLAG_HIDDEN);
            }
            signal_NFC_Nodeupdate = NFC_update_ISOK; // 重置信号————>通知计时器结束计时（10s内）
            cv_NFC_Nodeupdate2.notify_one(); // 通知等待的线程
        }
        NFC_sendJsonData_Read();
        std::cout << "NFC————写完————切换至读模式" << std::endl;
        NFC_Update_sendSignal();//3s后关闭节点更新的窗口
    }
}



//线程8：3s后自动关闭NFC弹窗
void thread8()
{
    pid_t threadId = getThreadId();
    std::cout << "Thread8 with ID: " << threadId << std::endl;
    while (true)
    {
        std::cout << "userUI.cpp——>2099——>3s后自动关闭NFC弹窗" << std::endl;
        int res = NFC_Event_ReceiveSignal();
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        // std::this_thread::sleep_for(std::chrono::seconds(1));
        if(res == 1){
            lv_obj_add_flag(obj_base1, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(NFC_window, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(tabview, LV_OBJ_FLAG_HIDDEN);//清空隐藏标志位=显示图片

            //启用按钮
            lv_obj_clear_state(NFC_btn[0], LV_STATE_DISABLED);
            lv_obj_clear_state(NFC_btn[1], LV_STATE_DISABLED);
            lv_obj_clear_state(NFC_btn[2], LV_STATE_DISABLED);

        }else if(res == 2){
            lv_obj_add_flag(obj_base2, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_flag(NFC_window, LV_OBJ_FLAG_HIDDEN);
            lv_obj_clear_flag(tabview, LV_OBJ_FLAG_HIDDEN);//清空隐藏标志位=显示图片
            //启用按钮
            lv_obj_clear_state(NFC_btn[0], LV_STATE_DISABLED);
            lv_obj_clear_state(NFC_btn[1], LV_STATE_DISABLED);
            lv_obj_clear_state(NFC_btn[2], LV_STATE_DISABLED);
        }
    }
}



//线程9：MQTT监听数据库更新（监测节点设备的增删改查操作）
void thread9()
{
    pid_t threadId = getThreadId();
    std::cout << "Thread9 with ID: " << threadId << std::endl;
    Mqtt_ListenPostgresqlUpdate();
}



//线程10：当mqtt接收到数据库update信号后（发送get设备信息https请求 + 更新网络信息）
void thread10()
{

    pid_t threadId = getThreadId();
    std::cout << "Thread10 with ID: " << threadId << std::endl;
    std::string getNodeInfo_url = "https://127.0.0.1:8080/api/devices?applicationID=0&limit=200&offset=0";
    while(1){
        std::cout << "userUI.cpp——>2145——>当mqtt接收到数据库update信号后（发送get设备信息https请求）" << std::endl;
        {
            std::unique_lock<std::mutex> lock(mutex_Mqtt);
            cv_Mqtt.wait(lock, [](){ return modified_Mqtt; });// 等待条件变量满足条件
            modified_Mqtt = false;
            std::cout << "数据库中节点信息发生变化————lcd get节点设备信号————接收" << std::endl;
            Get_NetworkInfo();
        }

                        
        {
            std::lock_guard<std::mutex> lock(mutex_NodeTabel);
            modified_Node = true;
            // http请求：获取节点设备信息
            std::string response_NodeINFO = send_httpRequest(getNodeInfo_url, Method_get);
            cjsonParse_getNodeDevINFO(response_NodeINFO.c_str());
            std::cout << "Node表格————更新表格信号————发出" << std::endl;
            cv_NodeTabel.notify_one();  
        }

        
    }
}



void thread11() {
    
    pid_t threadId = getThreadId();
    std::cout << "Thread11 with ID: " << threadId << std::endl;
    while (true) {
        std::cout << "每隔30s刷新一次NodeTable, 主要目的是为了更新活跃状态" << std::endl;
        MFunc_updateMainPage();
        // std::this_thread::sleep_for(std::chrono::minutes(1));
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}


// //轮询每隔5s刷新一次网络信息
// void thread12() {
    
//     pid_t threadId = getThreadId();
//     std::cout << "Thread11 with ID: " << threadId << std::endl;
//     while (true) {
//         std::cout << "每隔5s刷新一次网络信息" << std::endl;
//         Get_NetworkInfo();
//         std::this_thread::sleep_for(std::chrono::seconds(5));
//     }
// }


// void thread8()
// {
//     while (true)
//     {
//         std::unique_lock<std::mutex> lock(mutex_NFC_updatebtnStatus);
//         cv_NFC_updatebtnStatus.wait(lock, [](){ return signal_NFC_updatebtnStatus; });// 等待条件变量满足条件
//         signal_NFC_updatebtnStatus = false;
//         // 启用按钮
//         // lv_btn_set_state(NFC_btn, LV_BTN_STATE_DISABLED);
//         std::cout << "成功获得得到——>更新节点按钮恢复可点击状态的信号" << std::endl;
//     }
// }






//能够引发段错误的函数
void foo() {
    int *p = NULL;
    *p = 0; // 故意制造段错误
}




void lora_gateway_ui(void)
{
    pid_t threadId = getThreadId();
    std::cout << "Thread-UI with ID: " << threadId << std::endl;

    Logger::logToFile("-----------------------------------------");
    // 注册异常处理函数
    setup_signal_handlers();
    // 当发生段错误会将调用栈信息写入/var/log/lcd_lcd_stacktrace.txt(最多打印36条)
    Logger::logToFile("if segfault occurs, the call stack will be printed in /var/log/lcd_stacktrace.txt");

    //1、绘制主界面UI——节点设备信息界面（点击表格第一三列弹出两类弹窗）
    lora_MainWindow();
    std::cout << "主界面UI绘制完毕!" << std::endl;
    Logger::logToFile("Main Window UI is drawn!");
    lv_obj_add_event_cb(NodeDev_table, NodeDev_table_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    //2、绘制Node弹窗UI——>并添加隐藏属性
    lora_NodeWindow();
    std::cout << "Node弹窗UI绘制完毕!" << std::endl;
    Logger::logToFile("Node pop-up Window UI is drawn!");
    lv_obj_add_flag(Node_window, LV_OBJ_FLAG_HIDDEN);

    //3、绘制RTD弹窗UI——>并添加隐藏属性
    lora_RTDWindow();
    std::cout << "RTD弹窗UI绘制完毕!" << std::endl;
    Logger::logToFile("RTD pop-up Window UI is drawn!");
    lv_obj_add_flag(RTD_Window, LV_OBJ_FLAG_HIDDEN);

    //4、绘制NFC弹窗UI——>并添加隐藏属性
    lora_NFCWindow();
    std::cout << "NFC弹窗UI绘制完毕!" << std::endl;
    Logger::logToFile("NFC pop-up Window UI is drawn!");
    lv_obj_add_flag(NFC_window, LV_OBJ_FLAG_HIDDEN);
    registerNode_PopUp();
    std::cout << "NFC弹窗————节点注册弹窗UI绘制完毕!" << std::endl;
    Logger::logToFile("NFC pop-up Window----registerNode UI is drawn!");
    lv_obj_add_flag(obj_base1, LV_OBJ_FLAG_HIDDEN);
    updateNode_PopUp();
    std::cout << "NFC弹窗————节点更新弹窗UI绘制完毕!" << std::endl;
    Logger::logToFile("NFC pop-up Window----updateNode UI is drawn!");
    lv_obj_add_flag(obj_base2, LV_OBJ_FLAG_HIDDEN);
    FactoryReset_PopUp();
    std::cout << "NFC弹窗————节点恢复出厂设置弹窗UI绘制完毕!" << std::endl;
    Logger::logToFile("NFC pop-up Window----FactoryReset UI is drawn!");
    lv_obj_add_flag(obj_base3, LV_OBJ_FLAG_HIDDEN);

    // 单例模式确保类只有一个实例化对象
    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();
    languageSwitch_ClassOBJ->parse_JsonFile();
    Replace_labelDisplay(CHINESE);

    // 原子操作初始化
    write_ServerStatus(ServerStatus_Flase);

    Logger::logToFile("Prepare to start the sub-thread!");
    std::thread t1(thread1);    // 获取token及Node信息
    std::thread t2(thread2);    // 每5s更新一次Node表格
    std::thread t3(thread3);    // 发送wss请求RTD数据
    std::thread t4(thread4);    // 点击了特定节点后，一收到RTD数据就写入表格
    std::thread t5(thread5);    // NFC的wss长连接（一直不断开）
    std::thread t6(thread6);    // 节点设备一刷NFC就弹出NFC弹窗,并往弹窗中填写NFC信息
    std::thread t7(thread7);    // NFC弹窗中的——节点更新弹窗
    std::thread t8(thread8);    // NFC弹窗中的——自动关闭节点注册的弹窗
    std::thread t9(thread9);
    std::thread t10(thread10);

    // t1.join()       // 会阻塞主线程，会等待这个线程执行结束之后主线程才能继续
    t1.detach();    // 将线程与主线程分离，使得主线程和子线程可以并行执行
    t2.detach();
    t3.detach();
    t4.detach();
    t5.detach();
    t6.detach();
    t7.detach();
    t8.detach();
    t9.detach();
    t10.detach();

    std::thread t11(thread11);   // 每隔1min更新NodeTable表格
    t11.detach();

    // std::thread t12(thread12);   // 每隔5s轮询发送http请求获取网络信息
    // t12.detach();

    Logger::logToFile("All sub-threads are turned on, and ready to enter the event listener...");
}




