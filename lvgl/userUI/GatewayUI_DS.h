#ifndef __GatewayUI_DS_H__
#define __GatewayUI_DS_H__

#include <iostream>
using namespace std;

#include <fstream>
#include "cJSON.h"

#include <map>
#include <array>
#include <list>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>

#include <atomic>
#include <ctime>

#define RTDMaxNum 5

//--------------------------------------------------------------------------------

#define CHINESE 0
#define JAPANESE 1
#define KOREAN 2
#define ENGLISH 3



// 语言切换的类
class languageSwitchClass {
private:
    //创建一个私有的map容器存放所有语种的标签信息
    std::map<std::string, std::array<std::string, 4>> ALL_LabelInfo; // map关联容器（ID: [中，日，韩，英]）
    //单例模式确保类只有一个实例化对象，并提供一个全局访问点来获取该实例化对象
    static languageSwitchClass* instance; // 静态成员变量用于保存单例实例对象
    // std::mutex mutex_languageSwitch;
    int CurrentLanguageType;

    // 私有构造函数和拷贝构造函数
    languageSwitchClass();
    languageSwitchClass(const languageSwitchClass&);
    ~languageSwitchClass();
public:
    static languageSwitchClass* getInstance() {
        if (instance == nullptr) {
            instance = new languageSwitchClass();
        }
        return instance;
    }

    // 设置当前语言类型
    void set_CurrentLanguageType(int Languages);
    // 获取当前语言类型
    int get_CurrentLanguageType(void);
    // 清空存储多语言信息的结构
    void clear_map_allLabelInfo(void);
    // 添加各种语言的标签信息
    void add_languageLabelInfo(string key_labelInfo, string value1_Chinese, string value2_Japanese, string value3_Korean, string value4_English);
    // 获取指定语言的标签信息
    string get_languageLabelInfo(string key_labelInfo, int index);
    // 用于解析项目工程中的.json文件，将中日韩英对应的标签信息存入map中
    void print_ALL_LabelInfo(void);
    int parse_JsonFile(void);




};


//--------------------------------------------------------------------------------
//4G信号质量的枚举
enum LORA_NODE_QOS
{
    GOOD   = 0,
    MEDIUM = 1,
    BAD    = 2,
};
//http请求方法的枚举
enum HTTP_RESQUEUE{
    Method_get = 1,
    Method_post,
    Method_put,
    Method_delete,
    Method_Token,
};
//节点注册与节点更新————成功与失败的标志
enum JOIN_UPDATE{
    Join_OK = 1,
    Join_ON = 2,
    Update_OK = 3,
    Update_ON = 4,
};


//-------------------------------------------------------------------------------
typedef struct {
    bool enable;
    int id;
    int max;
    int min;
}analogChannel_INFO;

typedef struct {
    bool enable;
    int id;
    int mode;
    int trigger;
    int upload;
}digitalChannel_INFO;

//NFC信息
typedef struct {
    //NFC的basic内容
    char SN[24];              
    bool active;            
    bool adr;
    int aport;
    char appSkey[36];
    char appkey[36];
    int battery;
    char loraClass[5];
    bool confirm;
    int counter;
    int datarate;
    char devAddr[12];
    char devEUI[24];
    char fVersion[12];
    char hVersion[12];
    char joinEUI[24];
    char joinType[12];
    int accuracy;
    char altitude[12];
    char latitude[12];
    char longitude[12];
    char source[12];
    char macVersion[12];
    char name[36];
    int nbtrials;
    char nwkSKey[36];
    int pid;
    char product[36];
    char region[16];
    bool rejoin;
    bool recover;
    int rssi;
    int snr;
    int subband;
    int txpower;
    int uploadInterval;
    char vendor[24];
    int vid;
    int applicationID;
//------------------
    //NFC的collect内容
    //数字通道数组+模拟通道数组+电源的数组
    analogChannel_INFO analogChannel[2];
    digitalChannel_INFO digitalChannel[4];
    int duration_12V;
    int duration_5V;
}LORA_NFC_INFO;


extern LORA_NFC_INFO Global_NFC_Info;
#define FactoryResetValue_True 1
#define FactoryResetValue_Flase 0
//-------------------------------------------------------------------------------
//NFC信息
typedef struct {
    //NFC的basic内容
    char devEUI[24];              
    char name[36];
    char applicationID[5];
    char description[50];
    char deviceProfileID[50];
    bool skipFCntCheck;
    double referenceAltitude;
    bool isDisabled;
    char createdAt[50];
    char updatedAt[50];
    char lastSeenAt[50];
    int deviceStatusBattery;
    int deviceStatusMargin;
}LORA_NODESTATE_INFO;

extern LORA_NODESTATE_INFO Global_NODESTATE_Info;

LORA_NODESTATE_INFO read_NodeStateInfo(void);
void write_NodeStateInfo(LORA_NODESTATE_INFO nodeState_info);
void write_NFCInfo_isDisabled(bool isDisabled);


//--------------------------------------------------------------------------------
//Node弹窗信息
struct LORA_ANODE_INFO {
    char devEUI[24];              //设备唯一表示
    char devName[50];             //设备名称
    char lastOnlineTime[24];      //上次在线时间(活跃状态)
    int dataNumbers;        //数据条数
    int signalQuality;      //信号质量
    char Remark[50];              //备注
    // string nodeModel;           //节点型号
    char loraClass[12];           //设备类型（ABC）
    char joinType[12];            //激活方式（OTAA/ABP）
    char MacVersion[12];          //LoRaWAN的MAC版本
    char appEUI[36];              //JoinEUI/APPEUI
    int Battery;            //电量（百分比0-100）
};



//RTD弹窗信息
struct LORA_RTD_INFO{
    char packetType[16];         //包类型
    char Time[16];               //时间
    char direction[16];          //包传输方向
    char devEUI[24];
    char SNR[10];                 //信噪比        
    char RSSI[10];                //信号强度
}; 

 
//单个节点设备的所有信息（Node_Info、NFC_Info、RTD_Info...）
struct LORA_NODEDEVICE_INFO{
    char devEUI[24];            //节点设备唯一标识
    int dataNumbers;            //单个节点设备的数据总条数(>5后继续存储，但是要标识一下) 
    time_t timeTamp;         // 记录最新RTD数据到来时的时间戳（对于单个节点而言，效果同dataNumbers）
    float SNR;                 //信噪比        
    float RSSI;                //信号强度
    LORA_ANODE_INFO ANode_Info;
    // LORA_NFC_INFO NFC_Info;
    LORA_RTD_INFO RTDArray_Info[RTDMaxNum];
};


//--------------------------------------------------------------------------------
//创建Lora节点设备的类
class LoraNodeDeviceClass {
private:
    //创建一个私有的list容器存放所有节点的信息
    list<LORA_NODEDEVICE_INFO> nodeDeviceList;
    //单例模式确保类只有一个实例化对象，并提供一个全局访问点来获取该实例化对象
    static LoraNodeDeviceClass* instance; // 静态成员变量用于保存单例实例对象
    std::mutex mutex_Node;

    // 私有构造函数和拷贝构造函数
    LoraNodeDeviceClass();
    LoraNodeDeviceClass(const LoraNodeDeviceClass&);
    ~LoraNodeDeviceClass();
public:
    static LoraNodeDeviceClass* getInstance() {
        if (instance == nullptr) {
            instance = new LoraNodeDeviceClass();
        }
        return instance;
    }

    void Add_NodeDev_AfterInit(char *devEUI);//给list容器添加一个空节点并进行初始化
    void Remove_NodeDev_BydevEUI(const char* devEUI); //函数在执行过程中不能修改devEUI所指向的string对象的内容
    void Remove_NodeDev_ByIndex(int index);
    const list<LORA_NODEDEVICE_INFO>& getNodeDeviceList() const;    //常量成员函数不会修改对象的成员变量
    LORA_NODEDEVICE_INFO* getNodeDeviceByDevEUI(const char* devEUI);
    LORA_NODEDEVICE_INFO* getNodeDeviceByDevName(const char* devName);
    void testFunction(void);
    int get_ListLength(); 
    int findNodePositionWithDevEUI(const char* devEUI);
    bool findNodeWithDevName(const char* devName);

    void set_ANode_Info(const char* devEUI, LORA_ANODE_INFO ANode_Info);
    void set_ANodeInfo_dataNumbers(const char* devEUI, int dataNumbers);
    void set_ANodeInfo_SNR(const char* devEUI, double SNR);
    void set_ANodeInfo_RSSI(const char* devEUI, double RSSI);

    void set_RTDArray_Info(const char* devEUI, const LORA_RTD_INFO& newData);//同时也会设置每个节点的实时数据条数

    // int set_NFC_Info(const char* devEUI, LORA_NFC_INFO NFC_Info);
};


//--------------------------------------------------------------------------------
//https请求的类
class MyHttpsClass {
private:
    static MyHttpsClass* instance; // 静态成员变量用于保存单例实例对象
    string httptoken;           //用于存放token值
    string wsstoken;
    string gateway_ID;      //用于存放gateway_ID值
    string application_ID;  //用于存放application_ID值
    std::mutex mutex_http;

    MyHttpsClass() {}
    ~MyHttpsClass() {}
public:
    static MyHttpsClass* getInstance() {
        if (instance == nullptr) {
            instance = new MyHttpsClass();
        }
        return instance;
    }

    void setToken(const string newToken);  //设置http通信的token值
    string gethttpToken();                //获取http通信的token值
    string getwssToken();                //获取wss通信的token值
    void setGatewayID(const string newToken);  //设置gateway_ID的值
    string getGatewayID();                //获取gateway_ID的值

    void setApplicationID(const string newAPPID);
    string getApplicationID();
};




//wss请求的类
class MyWssClass {
private:

public:
    MyWssClass() {}
    ~MyWssClass() {}
};


//原子操作的变量
#define ServerStatus_True 1
#define ServerStatus_Flase 2
extern std::atomic<int> ServerStatus;
void write_ServerStatus(int value);// 原子操作————写服务器状态的操作
int read_ServerStatus();// 原子操作————读服务器状态的操作

extern std::atomic<const char*> NodeState_devEUI;
void write_NodeState(const char *devEUI);// 原子操作————写节点状态的devEUI操作
const char* read_NodeState();// 原子操作————读节点状态的devEUI操作




//互斥锁、条件变量
extern std::mutex mutex_NodeTabel;// 互斥量，用于保护共享变量
extern std::condition_variable cv_NodeTabel;// 条件变量，用于线程间的同步
extern bool modified_Node;

extern std::mutex mutex_RTDTabel;// 互斥量，用于保护共享变量
extern std::condition_variable cv_RTDTabel;// 条件变量，用于线程间的同步
extern bool modified_RTD;
extern char RTDTabel_option[50];  //用于选择指定节点的RTD数据流表格（devName）




extern std::mutex mutex_NFC;// 互斥量，用于保护共享变量
extern std::condition_variable cv_NFC;// 条件变量，用于线程间的同步
extern bool isNFCModified;

//读写NFC结构体信息，确保线程安全
LORA_NFC_INFO read_NFCInfo(void);
void write_NFCInfo_loraClass(const char* loraClass);
void write_NFCInfo_uploadInterval(int uploadInterval);
void write_NFCInfo_recover(bool recover);
void write_NFCInfo(LORA_NFC_INFO nfc_info);



extern std::mutex mutex_NFC_event;// 互斥量，用于保护共享变量
extern std::condition_variable cv_NFC_event;
extern bool signal_NFC_event1;
extern bool signal_NFC_event2;

void NFC_Register_sendSignal(void);
void NFC_Update_sendSignal(void);
int NFC_Event_ReceiveSignal(void);



// MQTT监听发送条件变量
//互斥锁、条件变量
extern std::mutex mutex_Mqtt;
extern std::condition_variable cv_Mqtt;
extern bool modified_Mqtt;




#define NFC_subWIN_Close 7

#define NFC_FactoryReset_True 5
#define NFC_FactoryReset_False 6

#define NFC_update_True 1
#define NFC_update_False 2
#define NFC_update_TimeOut 3
#define NFC_update_ISOK 4
extern std::mutex mutex_NFC_Nodeupdate;// 互斥量，用于保护共享变量
extern std::condition_variable cv_NFC_Nodeupdate;// 条件变量1，用于线程间的同步
extern std::condition_variable cv_NFC_Nodeupdate2;// 条件变量2，用于线程间的同步
extern std::condition_variable cv_NFC_NodeFactoryReset;// 条件变量1，用于线程间的同步
extern int signal_NFC_Nodeupdate;

// extern std::mutex mutex_NFC_updatebtnStatus;// 互斥量，用于保护共享变量
// extern std::condition_variable cv_NFC_updatebtnStatus;// 条件变量1，用于线程间的同步
// // extern std::condition_variable cv_NFC_Nodeupdate2;// 条件变量2，用于线程间的同步
// extern bool signal_NFC_updatebtnStatus;





class MyCustomClass {
public:
    // 构造函数
    MyCustomClass();
    
    // 成员函数
    void myFunction();
    
private:
    int myVariable;
};

//-----------------------------------------------------------------------------------------
#ifdef __cplusplus
extern "C"{
#endif


void dfprint_2(void);

#ifdef __cplusplus
}
#endif


#endif


