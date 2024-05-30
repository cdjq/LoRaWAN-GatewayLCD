#include "GatewayUI_DS.h"
#include <cstring>

#include <fstream>
#include <sstream>
#include <string>



std::mutex mutex_Mqtt;
std::condition_variable cv_Mqtt;
bool modified_Mqtt = false;



//只是声明了一个 MyHttpsClass 类的静态成员变量 instance，但没有在代码中实际定义它。为了解决这个问题，需要在类外部定义该静态成员变量
languageSwitchClass* languageSwitchClass::instance;

languageSwitchClass::languageSwitchClass() {
    // 构造函数的具体实现
    // 可以在这里对成员变量进行初始化
    CurrentLanguageType = 0;
}

//析构函数
languageSwitchClass::~languageSwitchClass(){
    
}


// 设置当前语言类型
void languageSwitchClass::set_CurrentLanguageType(int Languages) {
    CurrentLanguageType = Languages;
}

// 设置当前语言类型
int languageSwitchClass::get_CurrentLanguageType(void) {
    return CurrentLanguageType;
}


// 清空存储多语言信息的结构
void languageSwitchClass::clear_map_allLabelInfo(void) {
    ALL_LabelInfo.clear();
}

void languageSwitchClass::add_languageLabelInfo(string key_labelInfo, string value1_Chinese, string value2_Japanese, string value3_Korean, string value4_English) {
    ALL_LabelInfo[key_labelInfo] = {value1_Chinese, value2_Japanese, value3_Korean, value4_English};
}

string languageSwitchClass::get_languageLabelInfo(string key_labelInfo, int index) {
    auto it = ALL_LabelInfo.find(key_labelInfo);
    if (it != ALL_LabelInfo.end()) {
        return it->second[index];
    } else {
        return "";
    }
}

void languageSwitchClass::print_ALL_LabelInfo(void) {
    // 遍历map并打印每个键值对的信息
    for (const auto& pair : ALL_LabelInfo) {
        std::cout << "Key: " << pair.first << std::endl;
        std::cout << "Value: " << pair.second[0] << ", " << pair.second[1] << pair.second[2] << pair.second[3] << std::endl;
    }
}



std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}

int languageSwitchClass::parse_JsonFile(void) {

    // 清空 ALL_LabelInfo 中的元素
    clear_map_allLabelInfo();

    // 读取JSON文件
    std::string filename = "/home/lorawan/LorawanUI_multiLanguage.json";
    std::string data = readFile(filename);

    // std::cout << "File content:" << std::endl;
    // // 解析json
    // std::cout << "文件内容为：" << data << std::endl;

    // 解析JSON字符串
    cJSON *json = cJSON_Parse(data.c_str());
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "Error before: %s\n", error_ptr);
        }
        return 1;
    }

    // 确保我们解析到的是一个对象
    if (!cJSON_IsObject(json)) {
        fprintf(stderr, "JSON is not an object\n");
        cJSON_Delete(json);
        return 1;
    }


    // 遍历所有的键（Info0, Info1, ...）
    cJSON *info = NULL;
    cJSON_ArrayForEach(info, json) {
        // 获取当前信息对象的键名（例如："Info0"）
        const char *info_key = info->string;

        // 获取当前信息对象的内容
        cJSON *chinese_text = cJSON_GetObjectItemCaseSensitive(info, "Chinese_text");
        cJSON *japanese_text = cJSON_GetObjectItemCaseSensitive(info, "Japanese_text");
        cJSON *korean_text = cJSON_GetObjectItemCaseSensitive(info, "Korean_text");
        cJSON *english_text = cJSON_GetObjectItemCaseSensitive(info, "English_text");

        // 打印信息
        if (cJSON_IsString(chinese_text) && cJSON_IsString(english_text) && (chinese_text->valuestring != NULL) && (english_text->valuestring != NULL)) {
            // printf("%s: %s\n", info_key, chinese_text->valuestring);
            // 向ALL_LabelInfo 中添加元素
            add_languageLabelInfo(info_key, chinese_text->valuestring,"", "", english_text->valuestring);
        }

        

        
        // 重复上面的步骤来打印Japanese_text, Korean_text, 和 English_text...
    }

    // 输出解析结果
    // print_ALL_LabelInfo();

    // 删除cJSON对象释放内存
    cJSON_Delete(json);

    return 0;
}


//-----------------------------------------------------------------------------------------------------------------
std::mutex mutex_NodeTabel;// 互斥量，用于保护共享变量
std::condition_variable cv_NodeTabel;// 条件变量，用于线程间的同步
bool modified_Node = false;

std::mutex mutex_RTDTabel;// 互斥量，用于保护共享变量
std::condition_variable cv_RTDTabel;// 条件变量，用于线程间的同步
bool modified_RTD = false;
char RTDTabel_option[50] = {};


std::mutex mutex_NFC_event;// 互斥量，用于保护共享变量
std::condition_variable cv_NFC_event;
bool signal_NFC_event1 = false;
bool signal_NFC_event2 = false;

void NFC_Register_sendSignal(void)
{
    std::lock_guard<std::mutex> lock(mutex_NFC_event);
    std::cout << "节点注册————关闭节点注册窗口的信号————发送" << std::endl;
    signal_NFC_event1 = true;
    cv_NFC_event.notify_all();
}

void NFC_Update_sendSignal(void)
{
    std::lock_guard<std::mutex> lock(mutex_NFC_event);
    std::cout << "节点更新————关闭节点更新窗口的信号————发送" << std::endl;
    signal_NFC_event2 = true;
    cv_NFC_event.notify_all();
}

int NFC_Event_ReceiveSignal(void)
{
    std::unique_lock<std::mutex> lock(mutex_NFC_event);
    cv_NFC_event.wait(lock, []{ return signal_NFC_event1 || signal_NFC_event2; });
    

    if (signal_NFC_event1) {
        std::cout << "节点注册————关闭节点注册窗口的信号————接收" << std::endl;
        signal_NFC_event1 = false;
        return 1;
    }

    if (signal_NFC_event2) {
        std::cout << "节点更新————关闭节点更新窗口的信号————接收" << std::endl;
        signal_NFC_event2 = false;
        return 2;
    }

    return 0;
}

std::mutex mutex_NFC_Nodeupdate;// 互斥量，用于保护共享变量
std::condition_variable cv_NFC_Nodeupdate;// 条件变量，用于线程间的同步
std::condition_variable cv_NFC_Nodeupdate2;// 条件变量2，用于线程间的同步
int signal_NFC_Nodeupdate = 0;

LORA_NFC_INFO Global_NFC_Info = {};    //用于存储节点的NFC信息


// std::mutex mutex_NFC_updatebtnStatus;// 互斥量，用于保护共享变量
// std::condition_variable cv_NFC_updatebtnStatus;// 条件变量1，用于线程间的同步
// // extern std::condition_variable cv_NFC_Nodeupdate2;// 条件变量2，用于线程间的同步
// bool signal_NFC_updatebtnStatus = false;


// ------------------------------------------------------------------------------------------------------------------------
// 读写NFC结构体信息，确保线程安全
std::mutex mutex_NFC;// 互斥量，用于保护共享变量
std::condition_variable cv_NFC;// 条件变量，用于线程间的同步
bool isNFCModified = false;

LORA_NFC_INFO read_NFCInfo(void) {
    std::lock_guard<std::mutex> lock(mutex_NFC);
    return Global_NFC_Info;
}

void write_NFCInfo_loraClass(const char* loraClass) {
    std::lock_guard<std::mutex> lock(mutex_NFC);
    strcpy(Global_NFC_Info.loraClass, loraClass);

    // std::cout << Global_NFC_Info.name << std::endl;
    // std::cout << Global_NFC_Info.devEUI << std::endl;
    // std::cout << Global_NFC_Info.loraClass << std::endl;
}

void write_NFCInfo_uploadInterval(int uploadInterval) {
    std::lock_guard<std::mutex> lock(mutex_NFC);
    Global_NFC_Info.uploadInterval = uploadInterval;

    // std::cout << Global_NFC_Info.name << std::endl;
    // std::cout << Global_NFC_Info.devEUI << std::endl;
    // std::cout << Global_NFC_Info.uploadInterval << std::endl;
}

void write_NFCInfo_recover(bool recover) {
    std::lock_guard<std::mutex> lock(mutex_NFC);
    Global_NFC_Info.recover = recover;

    // std::cout << Global_NFC_Info.name << std::endl;
    // std::cout << Global_NFC_Info.devEUI << std::endl;
    // std::cout << Global_NFC_Info.uploadInterval << std::endl;
}

void write_NFCInfo(LORA_NFC_INFO nfc_info) {
    std::lock_guard<std::mutex> lock(mutex_NFC);
    memset(&Global_NFC_Info, '\0', sizeof(Global_NFC_Info)); 
    std::cout << "NFC弹窗————更新弹窗信号————发送" << std::endl;
    Global_NFC_Info = nfc_info;

    isNFCModified = true;
    cv_NFC.notify_one();
}




//-------------------------------------------------------------------------------------------------------------------------
LORA_NODESTATE_INFO Global_NODESTATE_Info = {};

std::mutex mutex_NodeState;
// std::condition_variable cv_NodeState;
// bool bool_NodeState = false;


//读写NFC结构体信息，确保线程安全
LORA_NODESTATE_INFO read_NodeStateInfo(void) {
    std::lock_guard<std::mutex> lock(mutex_NodeState);
    return Global_NODESTATE_Info;
}


void write_NodeStateInfo(LORA_NODESTATE_INFO nodeState_info) {
    std::lock_guard<std::mutex> lock(mutex_NodeState);
    memset(&Global_NODESTATE_Info, '\0', sizeof(Global_NODESTATE_Info)); 
    Global_NODESTATE_Info = nodeState_info;
}


void write_NFCInfo_isDisabled(bool isDisabled) {
    std::lock_guard<std::mutex> lock(mutex_NodeState);
    Global_NODESTATE_Info.isDisabled = isDisabled;
}



//-------------------------------------------------------------------------------------------------------------------------
std::atomic<int> ServerStatus;

// 原子操作————写服务器状态的操作
void write_ServerStatus(int value) {
    // std::cout << " ______________________________________ServerStatus = " << ServerStatus << std::endl;
    ServerStatus.store(value, std::memory_order_relaxed);
}

// 原子操作————读服务器状态的操作
int read_ServerStatus() {
    return ServerStatus.load(std::memory_order_relaxed);
}

//-------------------------------------------------------------------------------------------------------------------------
std::atomic<const char*> NodeState_devEUI;

// 原子操作————写节点状态的devEUI操作
void write_NodeState(const char *devEUI) {
    NodeState_devEUI.store(devEUI, std::memory_order_relaxed);
}

// 原子操作————读节点状态的devEUI操作
const char* read_NodeState() {
    return NodeState_devEUI.load(std::memory_order_relaxed);
}


//-------------------------------------------------------------------------------------------------------------------------



//只是声明了一个 MyHttpsClass 类的静态成员变量 instance，但没有在代码中实际定义它。为了解决这个问题，需要在类外部定义该静态成员变量
LoraNodeDeviceClass* LoraNodeDeviceClass::instance;

LoraNodeDeviceClass::LoraNodeDeviceClass() {
    // 构造函数的具体实现
    // 可以在这里对成员变量进行初始化
}

//析构函数
LoraNodeDeviceClass::~LoraNodeDeviceClass(){
    
}

//给list容器添加一个空节点并进行初始化
void LoraNodeDeviceClass::Add_NodeDev_AfterInit(char *devEUI) {
    std::lock_guard<std::mutex> lock(mutex_Node);
    LORA_NODEDEVICE_INFO nodeDevice;
    memset(&nodeDevice, 0, sizeof(LORA_NODEDEVICE_INFO));
    strcpy(nodeDevice.devEUI, devEUI);
    nodeDevice.dataNumbers = 0;
    nodeDevice.timeTamp = 0;
    nodeDevice.RSSI = 0;
    nodeDevice.SNR = 0;
    nodeDeviceList.push_back(nodeDevice);//在list容器中尾插元素
}


//移出指定devEUI的节点设备（函数在执行过程中不能修改devEUI所指向的string对象的内容）
void LoraNodeDeviceClass::Remove_NodeDev_BydevEUI(const char* devEUI) {
    for (auto it = nodeDeviceList.begin(); it != nodeDeviceList.end(); ++it) {
        if (!strncmp(it->devEUI,devEUI,16)) {
            nodeDeviceList.erase(it);//删除指定位置的元素
            break;
        }
    }
}

void LoraNodeDeviceClass::Remove_NodeDev_ByIndex(int index) {
    if (index < 0 || index >= nodeDeviceList.size()) {
        // 越界检查，可根据需求进行处理
        return;
    }
    int pos_index = 0;
    for (auto it = nodeDeviceList.begin(); it != nodeDeviceList.end(); ++it) {
        if (pos_index == index) {
            nodeDeviceList.erase(it);//删除指定位置的元素
            break;
        }
        pos_index++;
    }
}


//该函数返回值：可以获取到list容器信息，但不能通过这个返回的引用来修改nodeDeviceList中的元素
const list<LORA_NODEDEVICE_INFO>& LoraNodeDeviceClass::getNodeDeviceList() const {
    return nodeDeviceList;
}

//如果找到了匹配的节点设备，函数将返回一个指向该节点设备的指针，同样的不能通过这个返回的引用来修改该节点中的信息
//如果没有找到匹配的节点设备，则函数返回nullptr。
LORA_NODEDEVICE_INFO* LoraNodeDeviceClass::getNodeDeviceByDevEUI(const char* devEUI) {
    for (auto it = nodeDeviceList.begin(); it != nodeDeviceList.end(); ++it) {
        if (!strncmp(it->devEUI,devEUI,16)) {
            return &(*it);
        }
    }
    return nullptr; // 如果未找到匹配的节点，则返回nullptr
}

LORA_NODEDEVICE_INFO* LoraNodeDeviceClass::getNodeDeviceByDevName(const char* devName) {
    for (auto it = nodeDeviceList.begin(); it != nodeDeviceList.end(); ++it) {
        if (!strcmp(it->ANode_Info.devName ,devName)) {
            return &(*it);
        }
    }
    return nullptr; // 如果未找到匹配的节点，则返回nullptr
}


 void LoraNodeDeviceClass::testFunction(void){
    cout<<"LoraNodeDeviceClass Testing function..."<<endl;
 }

int LoraNodeDeviceClass::get_ListLength(){
    return nodeDeviceList.size();
}


int LoraNodeDeviceClass::findNodePositionWithDevEUI(const char* devEUI) {
    int position = 1;
    for (auto it = nodeDeviceList.begin(); it != nodeDeviceList.end(); ++it) {
        if (!strncmp(it->devEUI, devEUI, 16)) {
            return position;
        }
        position++;
    }
    return 0;
}


bool LoraNodeDeviceClass::findNodeWithDevName(const char* devName) {
    for (auto it = nodeDeviceList.begin(); it != nodeDeviceList.end(); ++it) {
        if (!strcmp(it->ANode_Info.devName, devName)) {
            return true;
        }
    }
    return false;
}


void LoraNodeDeviceClass::set_ANode_Info(const char* devEUI, LORA_ANODE_INFO ANode_Info){
    std::lock_guard<std::mutex> lock(mutex_Node);
    LORA_NODEDEVICE_INFO* pNode = getNodeDeviceByDevEUI(devEUI);
    if (pNode != nullptr) {
        strcpy(pNode->ANode_Info.devEUI, ANode_Info.devEUI);
        strcpy(pNode->ANode_Info.devName, ANode_Info.devName);
        strcpy(pNode->ANode_Info.lastOnlineTime, ANode_Info.lastOnlineTime);
        strcpy(pNode->ANode_Info.Remark, ANode_Info.Remark);
        strcpy(pNode->ANode_Info.loraClass, ANode_Info.loraClass);
        strcpy(pNode->ANode_Info.joinType, ANode_Info.joinType);
        strcpy(pNode->ANode_Info.MacVersion, ANode_Info.MacVersion);
        strcpy(pNode->ANode_Info.appEUI,  ANode_Info.appEUI);
        pNode->ANode_Info.Battery = ANode_Info.Battery;
    }
}


void LoraNodeDeviceClass::set_ANodeInfo_dataNumbers(const char* devEUI, int dataNumbers){
    std::lock_guard<std::mutex> lock(mutex_Node);
    LORA_NODEDEVICE_INFO* pNode = getNodeDeviceByDevEUI(devEUI);
    if (pNode != nullptr) {
        pNode->ANode_Info.dataNumbers = dataNumbers;
    }
}


void LoraNodeDeviceClass::set_ANodeInfo_RSSI(const char* devEUI, double RSSI){
    std::lock_guard<std::mutex> lock(mutex_Node);
    LORA_NODEDEVICE_INFO* pNode = getNodeDeviceByDevEUI(devEUI);
    if (pNode != nullptr) {
        pNode->RSSI = RSSI;
    }
}

void LoraNodeDeviceClass::set_ANodeInfo_SNR(const char* devEUI, double SNR){
    std::lock_guard<std::mutex> lock(mutex_Node);
    LORA_NODEDEVICE_INFO* pNode = getNodeDeviceByDevEUI(devEUI);
    if (pNode != nullptr) {
        pNode->SNR = SNR;
    }
}


void LoraNodeDeviceClass::set_RTDArray_Info(const char* devEUI, const LORA_RTD_INFO& newData) {
    int index = 0;
    LORA_NODEDEVICE_INFO* pNode = getNodeDeviceByDevEUI(devEUI);
    if (pNode != nullptr) {
        index = pNode->dataNumbers;
        
        time_t tempTimestamp = time(nullptr); // 获取当前时间的时间戳

        pNode->timeTamp = tempTimestamp;
        // 输出时间戳
        // std::cout << "最新RTD数据的时间戳为: " << tempTimestamp << " AND " << pNode->timeTamp <<std::endl;

        // 如果数组已满，则删除最旧的数据并前移后续的数据
        if(index < 5){
            pNode->RTDArray_Info[index] = newData;// 插入新数据到当前索引位置
            pNode->dataNumbers += 1;
        }else{
            for (int i = 0; i < RTDMaxNum-1; i++) {
                pNode->RTDArray_Info[i] = pNode->RTDArray_Info[i + 1];
            }
            pNode->RTDArray_Info[RTDMaxNum-1] = newData;// 将新数据插入到最后一个位置
            pNode->dataNumbers += 1;
        }
    }
}





//------------------------------------------------------------------------------------------------
//只是声明了一个 MyHttpsClass 类的静态成员变量 instance，但没有在代码中实际定义它。为了解决这个问题，需要在类外部定义该静态成员变量
MyHttpsClass* MyHttpsClass::instance;


void MyHttpsClass::setToken(const string newToken) {
    std::lock_guard<std::mutex> lock(mutex_http);
    httptoken = "Grpc-Metadata-Authorization: Bearer " + newToken;
    wsstoken = "Bearer, " + newToken;
    std::cout << "httptoken:" << httptoken << std::endl;
    std::cout << "wsstoken:" << wsstoken << std::endl;
}

string MyHttpsClass::gethttpToken() {
    std::lock_guard<std::mutex> lock(mutex_http);
    return httptoken;
}

string MyHttpsClass::getwssToken() {
    std::lock_guard<std::mutex> lock(mutex_http);
    return wsstoken;
}

void MyHttpsClass::setGatewayID(const string newGatewayID) {
    std::lock_guard<std::mutex> lock(mutex_http);
    gateway_ID = newGatewayID;
}

string MyHttpsClass::getGatewayID() {
    std::lock_guard<std::mutex> lock(mutex_http);
    return gateway_ID;
}

void MyHttpsClass::setApplicationID(const string newAPPID) {
    std::lock_guard<std::mutex> lock(mutex_http);
    application_ID = newAPPID;
}

string MyHttpsClass::getApplicationID() {
    std::lock_guard<std::mutex> lock(mutex_http);
    return application_ID;
}

//------------------------------------------------------------------------------------------------
void dfprint_2(void){
	cout<<"dfrobot print........."<<endl;
}





// 构造函数的实现
MyCustomClass::MyCustomClass() {
    myVariable = 0;
}

// 成员函数的实现
void MyCustomClass::myFunction() {
    myVariable++;
    cout<<"myFunction = "<<myVariable<<endl;
}



