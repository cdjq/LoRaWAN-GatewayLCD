#include "GatewayUI_wss.h"
#include "GatewayUI_DS.h"
#include "GatewayUI_https.h"
#include "log.h"

#include <libwebsockets.h>
#include "cJSON.h"
#include <stdbool.h>
#include <chrono>
#include <ctime>


#include <atomic>





#define MAX_WAIT_TIME_MS 2000
#define MAX_JSON_SIZE 2048

static int connection_established_RTD = 0;
static int connection_established_NFC = 0;

static char WriteNFC_jsonBuffer[MAX_JSON_SIZE] = {};

static int NFC_updateStatus = 0;

struct lws *wsi_NFC = nullptr;

#define NFC_Read 1
#define NFC_Write 2
std::atomic<int> NFC_Mode;


// 原子操作————写操作
void writeNFCMode(int value) {
    NFC_Mode.store(value, std::memory_order_relaxed);
}

// 原子操作————读操作
int readNFCMode() {
    return NFC_Mode.load(std::memory_order_relaxed);
}


/**********************************************************************************
 *        RTD的wss请求及相关函数
 **********************************************************************************/
int My_wss_JSON_GetRealTimeDataInfo(void *in)
{

    //单例模式确保类只有一个实例化对象
    LoraNodeDeviceClass *LoraNode_ClassOBJ = LoraNodeDeviceClass::getInstance();

    string devEUI;      //节点设备唯一标识
    LORA_RTD_INFO RTD_Info = {};


    cJSON *root=NULL;
    cJSON *root_Level_1=NULL;
    cJSON *root_Level_2=NULL;
    cJSON *root_Level_3=NULL;
    cJSON *root_Level_4=NULL;
    cJSON *root_Level_5=NULL;

   //--------------------------------------------------------------


    // 获取当前系统时间的时间戳
    std::time_t now = std::time(nullptr);

    // 将时间戳转化为指定格式的字符串
    char timeString[16];
    std::strftime(timeString, sizeof(timeString), "%H:%M:%S", std::localtime(&now));

    // 打印格式化后的时间字符串
    // std::cout << timeString << std::endl;

    strcpy(RTD_Info.Time, timeString);
    //--------------------------------------------------------------


    static bool log_Once = true;
    static bool log_Once2 = true;
    std::string str((const char *)in);


    root = cJSON_Parse((const char *)in);
    if (root == NULL) {
        std::cout << "thread3: RTD wss--->JsonParse-GetRealTimeDataInfo Failed! server responce content: " << str << std::endl;
        if(log_Once == true) {
            Logger::logToFile("thread3: RTD wss--->JsonParse-GetRealTimeDataInfo Failed! server responce content: " + str);
        }
        log_Once = false;
        return -1;
    }

    log_Once = true;


    if (cJSON_IsObject(root)){

        cJSON* error = cJSON_GetObjectItem(root, "error");
        if (error != nullptr) {

            cJSON *RTD_ERR_grpcCode = cJSON_GetObjectItem(error,"grpcCode");
            if (cJSON_IsNumber(RTD_ERR_grpcCode)) {
                printf("thread3: RTD wss--->GetRealTimeDataInfo ERROR grpcCode: %d\n", RTD_ERR_grpcCode->valueint);
            }
            cJSON *RTD_ERR_message = cJSON_GetObjectItem(error,"message");
            if (cJSON_IsString(RTD_ERR_message)&&cJSON_IsNumber(RTD_ERR_grpcCode)) {
                printf("thread3: RTD wss--->GetRealTimeDataInfo ERROR Message: %s\n", RTD_ERR_message->valuestring);
                std::string str(RTD_ERR_message->valuestring);
                if(log_Once2 == true) {
                    Logger::logToFile("thread3: RTD wss--->GetNFCInfo ERROR Code: " + std::to_string(RTD_ERR_grpcCode->valueint) + ", Message: " + str);
                }
            }

            log_Once2 = false;
            cJSON_Delete(root);
            return -1;      // 如果NFC ERROR Code为13 表示 nfc isn't initialization complecation
        }
        log_Once2 = true;

        //JSON数据的项（可以表示根项，也可以表示子项——>由cJSON_GetObjectItem这个的第一个参数决定）
        root_Level_1 = cJSON_GetObjectItem(root,"result");  //(↓)
        if (root_Level_1 != NULL && cJSON_IsObject(root_Level_1)) {
            //Level_2中——>uplinkFrame(√)———————————————————————————————————————————————————————————|
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "uplinkFrame");		// 获得obj里的值
            if (root_Level_2 != NULL) {
                // printf("上传\n");
                strcpy(RTD_Info.direction, "上传");
            }else{
                root_Level_2 = cJSON_GetObjectItem(root_Level_1, "downlinkFrame");		// 获得obj里的值
                if(root_Level_2 == NULL){
                    cJSON_Delete(root);
                    return -1;
                } 
                // printf("下发\n");
                strcpy(RTD_Info.direction, "下发");
            }
            if(root_Level_2 != NULL && cJSON_IsObject(root_Level_2)){
                //Level_3中——>devEUI(√)———————————————————————————————————|
                root_Level_3 = cJSON_GetObjectItem(root_Level_2, "devEUI");
                if (root_Level_3 != NULL && root_Level_3->type == cJSON_String) {
                    char *value_str = root_Level_3->valuestring;
                    // printf("devEUI = %s\n", value_str);
                    strcpy(RTD_Info.devEUI, value_str);
                }
                //Level_3中——>rxInfo(↓)———————————————————————————————————|
                root_Level_3 = cJSON_GetObjectItem(root_Level_2, "rxInfo");
                if (root_Level_3 != NULL && cJSON_IsArray(root_Level_3)) {
                    int size = cJSON_GetArraySize(root_Level_3);	// 获取的数组大小
                    for (int i = 0; i < size; i++) {
                        //Level_4——>在这里是指数组元素[0]、[1]、[2]......但是这里就只有一个数组元素[0],所以Level_4就取[0]
                        root_Level_4 = cJSON_GetArrayItem(root_Level_3, i);		// 获取的数组里的obj
                        if (root_Level_4 != NULL && root_Level_4->type == cJSON_Object) {	// 判断数字内的元素是不是obj类型
                            //Level_5中——>rssi(√)——————————————|
                            root_Level_5 = cJSON_GetObjectItem(root_Level_4, "rssi");		// 获得obj里的值
                            if (root_Level_5 != NULL && root_Level_5->type == cJSON_Number) {
                                double value_double = root_Level_5->valuedouble;
                                char rssi_str[10];
                                sprintf(rssi_str,"%d",(int)value_double);
                                // printf("rssi = %d\n", (int)value_double);
                                strcpy(RTD_Info.RSSI, rssi_str);
                                LoraNode_ClassOBJ->set_ANodeInfo_RSSI(RTD_Info.devEUI, value_double);

                            }
                            //Level_5中——>loRaSNR(√)———————————|
                            root_Level_5 = cJSON_GetObjectItem(root_Level_4, "loRaSNR");		// 获得obj里的值
                            if (root_Level_5 != NULL && root_Level_5->type == cJSON_Number) {
                                double value_double = root_Level_5->valuedouble;
                                char snr_str[10];
                                sprintf(snr_str,"%.1f",value_double);
                                // printf("rssi = %.1f\n", value_double);
                                strcpy(RTD_Info.SNR, snr_str);
                                LoraNode_ClassOBJ->set_ANodeInfo_SNR(RTD_Info.devEUI, value_double);
                            }
                        }
                    }
                }else{
                    strcpy(RTD_Info.RSSI, " ");
                    strcpy(RTD_Info.SNR, " ");
                }
                //Level_3中——>phyPayloadJSON(↓ √)———————————————————————————————————|
                root_Level_3 = cJSON_GetObjectItem(root_Level_2, "phyPayloadJSON");
                if (root_Level_3 != NULL && root_Level_3->type == cJSON_String) {
                    char *value_str = root_Level_3->valuestring;
                    // printf("phyPayloadJSON = %s\n", value_str);
                    char *p1=strstr(value_str,"JoinRequest");
                    char *p2=strstr(value_str,"JoinAccept");
                    char *p3=strstr(value_str,"UnconfirmedDataDown");
                    char *p4=strstr(value_str,"UnconfirmedDataUp");
                    char *p5=strstr(value_str,"ConfirmedDataDown");
                    char *p6=strstr(value_str,"ConfirmedDataUp");
                    if(p1 != NULL){
                        // printf("JoinRequest\n");
                        strcpy(RTD_Info.packetType, "JoinR");
                    }else if(p2 != NULL){
                        // printf("JoinAccept\n");
                        strcpy(RTD_Info.packetType, "JoinA");
                    }else if(p3 != NULL){
                        // printf("UnconfirmedDataDown\n");
                        strcpy(RTD_Info.packetType, "UncDown");
                    }else if(p4 != NULL){
                        // printf("UnconfirmedDataUp\n");
                        strcpy(RTD_Info.packetType, "UncUp");
                    }else if(p5 != NULL){
                        // printf("ConfirmedDataDown\n");
                        strcpy(RTD_Info.packetType, "CDown");
                    }else if(p6 != NULL){
                        // printf("ConfirmedDataUp\n");
                        strcpy(RTD_Info.packetType, "CUp");
                    }else{
                        // printf("UnknownData\n");
                        strcpy(RTD_Info.packetType, "UnD");
                    }   
                }
            }

        }else{
            return -1;
        }

        //每次获取完节点设备信息之后——>通知填写节点设备信息的线程可以刷新表格数据了
        {
            std::lock_guard<std::mutex> lock(mutex_RTDTabel);//当else循环结束后

            LoraNode_ClassOBJ->set_RTDArray_Info(RTD_Info.devEUI, RTD_Info);

            modified_RTD = true;
            cv_RTDTabel.notify_one();  
            std::cout << "RTD表格————更新表格信号————发出" << std::endl;
        }
    

        cJSON_Delete(root);
    }

    return 0;
 }




static int callback_ws(struct lws *wsi, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    std::string getTokenStatus_url = "https://127.0.0.1:8080/api/system/token"; // 获取token的有效期

    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:   //连接建立成功标志(三次握手)
            lwsl_notice("RTD WebSocket client connection established\n");
            connection_established_RTD = 1;
            std::cout << "thread3: RTD wss connection established, connection_established_RTD = " << connection_established_RTD << std::endl;
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:   //接收到数据的标志
            My_wss_JSON_GetRealTimeDataInfo(in);
            // logCode = My_wss_JSON_GetRealTimeDataInfo(in);
            // logCode_error的几种情况：1、JSON解析失败(服务器挂掉)     2、RTD服务器返回error  3、解析结果不符合预期(如包类型非上传和下发、关键数据为空...)
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:    //客户端断开的标志(四次挥手)
            connection_established_RTD = 0;
            std::cout << "thread3: RTD wss disconnection and in the close loop, connection_established_RTD = " << connection_established_RTD << std::endl;
            while(1){
                // 用于测试AS能否建立连接，能建立则退出让wss建立连接
                std::this_thread::sleep_for(std::chrono::seconds(10));
                std::string response = send_httpRequest(getTokenStatus_url, Method_get);   // http请求：Token值的有效性判断
                // 如果token无效，则会连上wss断开wss一直重复？？？
                std::string TokenStatus = "";

                cjsonParse_getTokenStatus(response.c_str(), TokenStatus);

                if (response.empty()) {   // AS断开(在循环中，直到AS服务启动)
                    std::cout << "RTD wss close中: AS服务断开, 陷入continue" << std::endl;
                    continue;
                } else {
                    if(TokenStatus == "TokenIsInvalid"){    // 如果AS正常，如果token失效，会一直循环等待，直到最长5min后获得新token
                        std::cout << "RTD wss close中: TokenIsInvalid, 陷入continue" << std::endl;
                        continue;
                    } else {                                // 如果AS正常，如果token有效，才开始重新建立wss连接
                        std::cout << "RTD wss close中: AS服务恢复正常, token有效, break close loop, 开始重连wss" << std::endl;
                        break;
                    }
                }
            }
            break;

        default:
            break;
    }

    return 0;
}





#define WEBSOCKET_CLIENT \
{ \
	"websocket-client", \
	callback_ws, \
	0, \
	8192, \
	0, NULL, 0 \
}

struct lws_protocols protocols[] = {WEBSOCKET_CLIENT,{ NULL, NULL, 0, 0 }};



int My_wssRequest_RTD(void)
{
    //单例模式确保类只有一个实例化对象
    MyHttpsClass *http_ClassOBJ = MyHttpsClass::getInstance();


    struct lws_context_creation_info info;
    struct lws_client_connect_info connect_info;
    struct lws_context *context;
    int err;

    char url[128];
    char path[128];
    sprintf(url,"wss://127.0.0.1:8080/api/gateways/%s/frames",(http_ClassOBJ->getGatewayID()).c_str());
    // char *url = "wss://127.0.0.1:8080/api/gateways/46DB241C399A029D/frames";
    sprintf(path, "/api/gateways/%s/frames", http_ClassOBJ->getGatewayID().c_str());

    
//_____________________________________________
    char *protocol = (char*)malloc(256);
    sprintf(protocol,"%s",(http_ClassOBJ->getwssToken()).c_str());

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    // 初始化ssl相关的库
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    // 设置自签名证书路径
    info.ssl_cert_filepath = "/etc/nginx/cert/lorawan.dfrobot.top.pem";
    info.ssl_private_key_filepath = "/etc/nginx/cert/lorawan.dfrobot.top.key";


    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("Failed to create libwebsockets context\n");
        return -1;
    }

    memset(&connect_info, 0, sizeof(connect_info));
    connect_info.context = context;
    connect_info.address = "127.0.0.1";
    connect_info.port = 8080;
    connect_info.path = path;
    connect_info.host = "127.0.0.1:8080";
    connect_info.origin = "https://127.0.0.1";
    connect_info.protocol = protocol;
    free(protocol);
    //启用SSL相关证书
    connect_info.ssl_connection |= LCCSCF_USE_SSL;
    //跳过服务器主机名检查：
    //在建立SSL连接时，客户端会检查服务器的证书中的主机名与实际连接的主机名是否匹配，这是为了防止中间人攻击。
    //跳过服务器主机名检查可能会有安全风险，因为它会放宽对服务器身份验证的要求。建议只在非生产环境或特定情况下使用这个选项，并且在生产环境中使用正确的证书配置。
    connect_info.ssl_connection |= LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    //使用自签名证书
    connect_info.ssl_connection |= LCCSCF_ALLOW_SELFSIGNED;

    bool log_Once1 = true;//用于只显示程序运行的状态（不是动作）
    struct lws *wsi;
    while (1) {
        // 每次重新连接的时候，重新组装token
        std::string protocol_str = http_ClassOBJ->getwssToken();

        connect_info.protocol = protocol_str.c_str();
        
    
        wsi = lws_client_connect_via_info(&connect_info);

        if (wsi == NULL) {
            fprintf(stderr, "Failed to connect to server\n");
            if(log_Once1 == true){
                Logger::logToFile("thread3: RTD wss---> Failed to establish! (wsi = NULL))");
                log_Once1 = false;
            }
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
            // break;
        }

        while (!connection_established_RTD) {
            log_Once1 = true;
            lws_service(context, 0);
            // std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        while (connection_established_RTD) {
            log_Once1 = true;
            lws_service(context, 0);    
            // std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    Logger::logToFile("thread3: RTD wss--->RTD Exited the loop, and destroy context");
    lws_context_destroy(context);

}



int Get_NFC_updateStatus(void)
{
    return NFC_updateStatus;
}


/**********************************************************************************
 *        NFC的wss请求及相关函数
 **********************************************************************************/
int My_wss_JSON_GetNFCInfo(void *in)
{
    LORA_NFC_INFO NFC_Info = {};

    cJSON *root=NULL;
    cJSON *root_Level_1=NULL;
    cJSON *root_Level_2=NULL;

    static bool log_Once = true;
    static bool log_Once2 = true;
    std::string str((const char *)in);

    root = cJSON_Parse((const char *)in);



    if (root == NULL) {
        std::cout << "thread5: NFC wss--->JsonParse-GetNFCInfo Failed! server responce content: " << str << std::endl;
        if(log_Once == true) {
            Logger::logToFile("thread5: NFC wss--->JsonParse-GetNFCInfo Failed! server responce content: " + str);
        }
        log_Once = false;
        return 1;
    }

    log_Once = true;


    if (cJSON_IsObject(root)){

        cJSON* error = cJSON_GetObjectItem(root, "error");
        if (error != nullptr) {
            cJSON *NFC_ERR_grpcCode = cJSON_GetObjectItem(error,"grpcCode");
            if (cJSON_IsNumber(NFC_ERR_grpcCode)) {
                printf("thread5: NFC wss--->GetNFCInfo ERROR grpcCode: %d\n", NFC_ERR_grpcCode->valueint);
            }
            cJSON *NFC_ERR_message = cJSON_GetObjectItem(error,"message");
            if (cJSON_IsString(NFC_ERR_message)&&cJSON_IsNumber(NFC_ERR_grpcCode)) {
                printf("thread5: NFC wss--->GetNFCInfo ERROR Message: %s\n", NFC_ERR_message->valuestring);
                std::string str(NFC_ERR_message->valuestring);
                if(log_Once2 == true) {
                    Logger::logToFile("thread5: NFC wss--->GetNFCInfo Code: " + std::to_string(NFC_ERR_grpcCode->valueint) + ", ERROR Message: " + str);
                }
            }

            log_Once2 = false;
            cJSON_Delete(root);
            return 13;      // 如果为13 表示 nfc isn't initialization complecation
        }

        log_Once2 = true;

        //JSON数据的项（可以表示根项，也可以表示子项——>由cJSON_GetObjectItem这个的第一个参数决定）
        root_Level_1 = cJSON_GetObjectItem(root,"result");  //(↓)

        if (root_Level_1 != NULL && cJSON_IsObject(root_Level_1)) {

            //更新节点成功
            //Level_2中——>empty(↓)———————————————————————————————————————————————————————————|
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "empty");		// 获得obj里的值
            if (root_Level_2 != NULL) {
                LORA_NFC_INFO tempNFC = read_NFCInfo();
                // std::cout << " ___________________________________________________________recover = " << tempNFC.recover << std::endl;
                if(tempNFC.recover == 0){
                    std::unique_lock<std::mutex> lock(mutex_NFC_Nodeupdate);
                    signal_NFC_Nodeupdate = NFC_update_True; // 生成更新状态的信号
                    cv_NFC_Nodeupdate.notify_one(); // 通知所有等待的线程
                    std::cout << "NFC-更新节点-成功-信号发出" << std::endl;
                }else if(tempNFC.recover == 1){
                    std::unique_lock<std::mutex> lock(mutex_NFC_Nodeupdate);
                    signal_NFC_Nodeupdate = NFC_FactoryReset_True; // 生成更新状态的信号
                    cv_NFC_Nodeupdate.notify_one(); // 通知所有等待的线程
                    std::cout << "NFC-恢复出厂设置-成功-信号发出" << std::endl;
                }


                cJSON_Delete(root);
                return 0; 
            }

            //更新节点失败
            //Level_2中——>error(↓)———————————————————————————————————————————————————————————|
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "error");		// 获得obj里的值
            if (root_Level_2 != NULL) {
                LORA_NFC_INFO tempNFC = read_NFCInfo();
                if(tempNFC.recover == 0){
                    std::unique_lock<std::mutex> lock(mutex_NFC_Nodeupdate);
                    signal_NFC_Nodeupdate = NFC_update_False; // 生成更新状态的信号
                    std::cout << "NFC-更新节点-失败-信号发出" << std::endl;
                    cv_NFC_Nodeupdate.notify_one(); // 通知所有等待的线程
                }else if(tempNFC.recover == 1){
                    std::unique_lock<std::mutex> lock(mutex_NFC_Nodeupdate);
                    signal_NFC_Nodeupdate = NFC_FactoryReset_False; // 生成更新状态的信号
                    cv_NFC_Nodeupdate.notify_one(); // 通知所有等待的线程
                    std::cout << "NFC-恢复出厂设置-失败-信号发出" << std::endl;
                }

                cJSON_Delete(root);
                return 0;
            }

            //Level_2中——>errors(↓)———————————————————————————————————————————————————————————|
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "errors");		// 获得obj里的值
            if (root_Level_2 != NULL) {
                cJSON_Delete(root);
                return 0;
            }


            //Level_2中——>payload(↓)———————————————————————————————————————————————————————————|
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "payload");		// 获得obj里的值
                //此处拿到的payload Json对象的value值为一个长字符串——>需要将其转换为一个个Json对象
            if (root_Level_2 != NULL && root_Level_2->type == cJSON_String) {
                // 解析字符串为 cJSON 对象
                // printf("\n\n%s\n\n", root_Level_2->valuestring);
                cJSON *nfc_obj_Level2_1 = cJSON_Parse(root_Level_2->valuestring);
                if(nfc_obj_Level2_1 == NULL){
                    Logger::logToFile("thread5: NFC wss--->JsonParse-GetNFCInfo Failed! nfc payload field in NFCJSON (Failed to parse JSON)");
                    cJSON_Delete(root);
                    return 0;
                }
                //Level_3中——>nfc(↓)—————————————————————————————————————————|
                cJSON *nfc_obj_Level3 = cJSON_GetObjectItemCaseSensitive(nfc_obj_Level2_1, "nfc");
                if(nfc_obj_Level3 != NULL){
                    //Level_4中——>basic(↓)—————————————————————————|
                    cJSON *basic_obj_Level4 = cJSON_GetObjectItemCaseSensitive(nfc_obj_Level3, "basic");
                    if(basic_obj_Level4 != NULL){
                        //Level_5中——>SN(√)—————————————|
                        cJSON *sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "SN");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.SN, v_str);
                            printf("\nSN: %s\n", NFC_Info.SN);
                        }
                        //Level_5中——>active(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "active");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.active = sub_obj_Level5->valueint;
                            printf("active: %d\n", NFC_Info.active);
                        }
                        //Level_5中——>adr(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "adr");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.adr = sub_obj_Level5->valueint;
                            printf("adr: %d\n", NFC_Info.adr);
                        }
                        //Level_5中——>aport(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "aport");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.aport = sub_obj_Level5->valueint;
                            printf("aport: %d\n", NFC_Info.aport);
                        }
                        //Level_5中——>appSkey(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "appSkey");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.appSkey, v_str);
                            printf("appSkey: %s\n", NFC_Info.appSkey);
                        }
                        //Level_5中——>appkey(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "appkey");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.appkey, v_str);
                            printf("appkey: %s\n", NFC_Info.appkey);
                        }
                        //Level_5中——>battery(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "battery");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.battery = sub_obj_Level5->valueint;
                            printf("battery: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>class(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "class");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.loraClass, v_str);
                            printf("loraClass: %s\n", NFC_Info.loraClass);
                        }
                        //Level_5中——>confirm(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "confirm");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.confirm = sub_obj_Level5->valueint;
                            printf("confirm: %d\n", NFC_Info.confirm);
                        }
                        //Level_5中——>counter(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "counter");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.counter = sub_obj_Level5->valueint;
                            printf("counter: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>datarate(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "datarate");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.datarate = sub_obj_Level5->valueint;
                            printf("datarate: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>devAddr(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "devAddr");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.devAddr, v_str);
                            printf("devAddr: %s\n", NFC_Info.devAddr);
                        }
                        //Level_5中——>devEUI(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "devEUI");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.devEUI, v_str);
                            printf("devEUI: %s\n", NFC_Info.devEUI);
                        }
                        //Level_5中——>fVersion(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "fVersion");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.fVersion, v_str);
                            printf("fVersion: %s\n", NFC_Info.fVersion);
                        }
                        //Level_5中——>hVersion(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "hVersion");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.hVersion, v_str);
                            printf("hVersion: %s\n", NFC_Info.hVersion);
                        }
                        //Level_5中——>joinEUI(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "joinEUI");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.joinEUI, v_str);
                            printf("joinEUI: %s\n", NFC_Info.joinEUI);
                        }
                        //Level_5中——>joinType(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "joinType");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.joinType, v_str);
                            printf("joinType: %s\n", NFC_Info.joinType);
                        }
                        //Level_5中——>location(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "location");
                        if (sub_obj_Level5 != NULL) {
                            //Level_6中——>accuracy(√)———|
                            cJSON *sub_obj_Level6 = cJSON_GetObjectItemCaseSensitive(sub_obj_Level5, "accuracy");
                            if (sub_obj_Level6 != NULL) {
                                NFC_Info.accuracy = sub_obj_Level5->valueint;
                                printf("accuracy: %d\n", sub_obj_Level5->valueint);
                            }
                            //Level_6中——>altitude(√)———|
                            sub_obj_Level6 = cJSON_GetObjectItemCaseSensitive(sub_obj_Level5, "altitude");
                            if (sub_obj_Level6 != NULL) {
                                char *v_str = sub_obj_Level6->valuestring;
                                for (int i = 0; v_str[i] != '\0'; i++) {
                                    v_str[i] = toupper(v_str[i]);
                                }
                                strcpy(NFC_Info.altitude, v_str);
                                printf("altitude: %s\n", NFC_Info.altitude);
                            }
                            //Level_6中——>latitude(√)———|
                            sub_obj_Level6 = cJSON_GetObjectItemCaseSensitive(sub_obj_Level5, "latitude");
                            if (sub_obj_Level6 != NULL) {
                                char *v_str = sub_obj_Level6->valuestring;
                                for (int i = 0; v_str[i] != '\0'; i++) {
                                    v_str[i] = toupper(v_str[i]);
                                }
                                strcpy(NFC_Info.latitude, v_str);
                                printf("latitude: %s\n", NFC_Info.latitude);
                            }
                            //Level_6中——>longitude(√)———|
                            sub_obj_Level6 = cJSON_GetObjectItemCaseSensitive(sub_obj_Level5, "longitude");
                            if (sub_obj_Level6 != NULL) {
                                char *v_str = sub_obj_Level6->valuestring;
                                for (int i = 0; v_str[i] != '\0'; i++) {
                                    v_str[i] = toupper(v_str[i]);
                                }
                                strcpy(NFC_Info.longitude, v_str);
                                printf("longitude: %s\n", NFC_Info.longitude);
                            }
                            //Level_6中——>source(√)———|
                            sub_obj_Level6 = cJSON_GetObjectItemCaseSensitive(sub_obj_Level5, "source");
                            if (sub_obj_Level6 != NULL) {
                                char *v_str = sub_obj_Level6->valuestring;
                                for (int i = 0; v_str[i] != '\0'; i++) {
                                    v_str[i] = toupper(v_str[i]);
                                }
                                strcpy(NFC_Info.source, v_str);
                                printf("source: %s\n", NFC_Info.source);
                            }
                        }
                        //Level_5中——>macVersion(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "macVersion");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.macVersion, v_str);
                            printf("macVersion: %s\n", NFC_Info.macVersion);
                        }
                        //Level_5中——>name(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "name");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            strcpy(NFC_Info.name, v_str);
                            printf("name: %s\n", NFC_Info.name);
                        }
                        //Level_5中——>nbtrials(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "nbtrials");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.nbtrials = sub_obj_Level5->valueint;
                            printf("nbtrials: %d\n", NFC_Info.nbtrials);
                        }
                        //Level_5中——>nbtrials(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "nbtrials");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.nbtrials = sub_obj_Level5->valueint;
                            printf("nbtrials: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>nwkSKey(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "nwkSKey");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.nwkSKey, v_str);
                            printf("nwkSKey: %s\n", NFC_Info.nwkSKey);
                        }
                        //Level_5中——>pid(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "pid");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.pid = sub_obj_Level5->valueint;
                            printf("pid: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>product(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "product");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.product, v_str);
                            printf("product: %s\n", NFC_Info.product);
                        }
                        //Level_5中——>region(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "region");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.region, v_str);
                            printf("region: %s\n", NFC_Info.region);
                        }
                        //Level_5中——>rejoin(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "rejoin");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.rejoin = sub_obj_Level5->valueint;
                            printf("rejoin: %d\n", NFC_Info.rejoin);
                        }
                        //Level_5中——>recover(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "recover");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.recover = sub_obj_Level5->valueint;
                            printf("recover: %d\n", NFC_Info.recover);
                        }
                        //Level_5中——>rssi(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "rssi");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.rssi = sub_obj_Level5->valueint;
                            printf("rssi: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>snr(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "snr");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.snr = sub_obj_Level5->valueint;
                            printf("snr: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>subband(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "subband");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.subband = sub_obj_Level5->valueint;
                            printf("subband: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>txpower(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "txpower");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.txpower = sub_obj_Level5->valueint;
                            printf("txpower: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>uploadInterval(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "uploadInterval");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.uploadInterval = sub_obj_Level5->valueint;
                            printf("uploadInterval: %d\n", sub_obj_Level5->valueint);
                        }
                        //Level_5中——>vendor(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "vendor");
                        if (sub_obj_Level5 != NULL) {
                            char *v_str = sub_obj_Level5->valuestring;
                            for (int i = 0; v_str[i] != '\0'; i++) {
                                v_str[i] = toupper(v_str[i]);
                            }
                            strcpy(NFC_Info.vendor, v_str);
                            printf("vendor: %s\n", NFC_Info.vendor);
                        }
                        //Level_5中——>vid(√)—————————————|
                        sub_obj_Level5 = cJSON_GetObjectItemCaseSensitive(basic_obj_Level4, "vid");
                        if (sub_obj_Level5 != NULL) {
                            NFC_Info.vid = sub_obj_Level5->valueint;
                            printf("vid: %d\n", sub_obj_Level5->valueint);
                        }
                        // //差启用通道+开机状态+入网状态 
                    }

                    //Level_4中——>collect(↓)—————————————————————————|
                    basic_obj_Level4 = cJSON_GetObjectItemCaseSensitive(nfc_obj_Level3, "collect");
                    if(basic_obj_Level4 != NULL){
                        // 解析analogChannel
                        cJSON* analogChannel = cJSON_GetObjectItem(basic_obj_Level4, "analogChannel");
                        int analogChannel_size = cJSON_GetArraySize(analogChannel);
                        for (int i = 0; i < analogChannel_size; i++) {
                            cJSON* channel = cJSON_GetArrayItem(analogChannel, i);
                            bool enable = cJSON_GetObjectItem(channel, "enable")->valueint;
                            int id = cJSON_GetObjectItem(channel, "id")->valueint;
                            int max = cJSON_GetObjectItem(channel, "max")->valueint;
                            int min = cJSON_GetObjectItem(channel, "min")->valueint;
                            NFC_Info.analogChannel[i].enable = enable;
                            NFC_Info.analogChannel[i].id = id;
                            NFC_Info.analogChannel[i].max = max;
                            NFC_Info.analogChannel[i].max = max;
                            printf("Analog Channel: enable=%d, id=%d, max=%d, min=%d\n", enable, id, max, min);
                        }
                        // 解析digitalChannel
                        cJSON* digitalChannel = cJSON_GetObjectItem(basic_obj_Level4, "digitalChannel");
                        int digitalChannel_size = cJSON_GetArraySize(digitalChannel);
                        for (int i = 0; i < digitalChannel_size; i++) {
                            cJSON* channel = cJSON_GetArrayItem(digitalChannel, i);
                            bool enable = cJSON_GetObjectItem(channel, "enable")->valueint;
                            int id = cJSON_GetObjectItem(channel, "id")->valueint;
                            int mode = cJSON_GetObjectItem(channel, "mode")->valueint;
                            int trigger = cJSON_GetObjectItem(channel, "trigger")->valueint;
                            int upload = cJSON_GetObjectItem(channel, "upload")->valueint;
                            NFC_Info.digitalChannel[i].enable = enable;
                            NFC_Info.digitalChannel[i].id = id;
                            NFC_Info.digitalChannel[i].mode = mode;
                            NFC_Info.digitalChannel[i].trigger = trigger;
                            NFC_Info.digitalChannel[i].upload = upload;
                            printf("Digital Channel: enable=%d, id=%d, mode=%d, trigger=%d, upload=%d\n", enable, id, mode, trigger, upload);
                        }
                        // 解析power
                        cJSON* power = cJSON_GetObjectItem(basic_obj_Level4, "power");
                        if(power != NULL){
                            cJSON* item = cJSON_GetObjectItem(power, "12V");
                            if(item != NULL){
                                int duration = cJSON_GetObjectItem(item, "duration")->valueint;
                                NFC_Info.duration_12V = duration;
                                std::cout << " NFC_Info.duration_12V = " << NFC_Info.duration_12V << std::endl;
                            }
                            item = cJSON_GetObjectItem(power, "5V");
                                if(item != NULL){
                                int duration = cJSON_GetObjectItem(item, "duration")->valueint;
                                NFC_Info.duration_5V = duration;
                                std::cout << " NFC_Info.duration_5V = " << NFC_Info.duration_5V << std::endl;
                            }
                        }
                    }
                } 
            }
        }

    }
    write_NFCInfo(NFC_Info);

    cJSON_Delete(root);
    return 0; 
}


void NFC_sendJsonData(const char *json)
{
    // Copy JSON data to the buffer
    strncpy(WriteNFC_jsonBuffer, json, MAX_JSON_SIZE);
    WriteNFC_jsonBuffer[MAX_JSON_SIZE - 1] = '\0';
    writeNFCMode(NFC_Write);
    // 调用可写的回调函数——>在wss长连接中发送JSON数据
    if (wsi_NFC != nullptr) {
        
        lws_callback_on_writable(wsi_NFC);
    }
}


void NFC_sendJsonData_Read(void)
{
    writeNFCMode(NFC_Read);
    // 调用可写的回调函数——>在wss长连接中发送JSON数据
    if (wsi_NFC != nullptr) {
        lws_callback_on_writable(wsi_NFC);
    }
}

//删除字符串中的空白字符
static void removeWhitespace(char *str) {
    int i = 0, j = 0;
    while (str[i]) {
        if (!isspace((unsigned char)str[i])) {
            str[j++] = str[i];
        }
        i++;
    }
    str[j] = '\0';
}



//wss发送的JSON组装
void NFC_JsonDatAassembly(int FactoryResetValue)
{
    LORA_NFC_INFO  NFC_Info = read_NFCInfo();
    //_____________________________________________________________________________________________________________________________________________________________L1
    // 创建一个cJSON对象
    cJSON *root = cJSON_CreateObject();
    //________________________________________________________________________________________________________________________________________L2
    // 创建一个nfc对象
    cJSON *nfc = cJSON_CreateObject();
    //____________________________________________________________________________________________________________________L3
    // 创建一个basic对象
    cJSON *basic = cJSON_CreateObject();
    //___________________________________________________________________________________________________L4
    // 添加basic的属性
    cJSON_AddStringToObject(basic, "SN", NFC_Info.SN);
    cJSON_AddBoolToObject(basic, "active", NFC_Info.active);
    cJSON_AddBoolToObject(basic, "adr", NFC_Info.adr);
    cJSON_AddNumberToObject(basic, "aport", NFC_Info.aport);
    cJSON_AddStringToObject(basic, "appSkey", NFC_Info.appSkey);
    cJSON_AddStringToObject(basic, "appkey", NFC_Info.appkey);
    cJSON_AddNumberToObject(basic, "battery", NFC_Info.battery);
    cJSON_AddStringToObject(basic, "class", NFC_Info.loraClass);
    cJSON_AddBoolToObject(basic, "confirm", NFC_Info.confirm);
    cJSON_AddNumberToObject(basic, "counter", NFC_Info.counter + 1);
    cJSON_AddNumberToObject(basic, "datarate", NFC_Info.datarate);
    cJSON_AddStringToObject(basic, "devAddr", NFC_Info.devAddr);
    cJSON_AddStringToObject(basic, "devEUI", NFC_Info.devEUI);
    cJSON_AddStringToObject(basic, "fVersion", NFC_Info.fVersion);
    cJSON_AddStringToObject(basic, "hVersion", NFC_Info.hVersion);
    cJSON_AddStringToObject(basic, "joinEUI", NFC_Info.joinEUI);
    cJSON_AddStringToObject(basic, "joinType", NFC_Info.joinType);

    // 添加location的属性
    cJSON *location = cJSON_CreateObject();
    //_______________________________________________________________________________L5
    cJSON_AddNumberToObject(location, "accuracy", NFC_Info.accuracy);
    cJSON_AddStringToObject(location, "altitude", NFC_Info.altitude);
    cJSON_AddStringToObject(location, "latitude", NFC_Info.latitude);
    cJSON_AddStringToObject(location, "longitude", NFC_Info.longitude);
    cJSON_AddStringToObject(location, "source", NFC_Info.source);

    cJSON_AddItemToObject(basic, "location", location);// 将location添加到basic中

    cJSON_AddStringToObject(basic, "macVersion", NFC_Info.macVersion);
    cJSON_AddStringToObject(basic, "name", NFC_Info.name);
    cJSON_AddNumberToObject(basic, "nbtrials", NFC_Info.nbtrials);
    cJSON_AddStringToObject(basic, "nwkSKey", NFC_Info.nwkSKey);
    cJSON_AddNumberToObject(basic, "pid", NFC_Info.pid);
    cJSON_AddStringToObject(basic, "product", NFC_Info.product);
    cJSON_AddStringToObject(basic, "region", NFC_Info.region);
    cJSON_AddBoolToObject(basic, "rejoin", NFC_Info.rejoin);
    if(FactoryResetValue == FactoryResetValue_True){
        write_NFCInfo_recover(1);
        cJSON_AddBoolToObject(basic, "recover", 1);
    }else if(FactoryResetValue == FactoryResetValue_Flase){
        write_NFCInfo_recover(0);
        cJSON_AddBoolToObject(basic, "recover", 0);
    }
    cJSON_AddNumberToObject(basic, "rssi", NFC_Info.rssi);
    cJSON_AddNumberToObject(basic, "snr", NFC_Info.snr);
    cJSON_AddNumberToObject(basic, "subband", NFC_Info.subband);
    cJSON_AddNumberToObject(basic, "txpower", NFC_Info.txpower);
    cJSON_AddNumberToObject(basic, "uploadInterval", NFC_Info.uploadInterval);
    cJSON_AddStringToObject(basic, "vendor", NFC_Info.vendor); 
    cJSON_AddNumberToObject(basic, "vid", NFC_Info.vid);

    
    //____________________________________________________________________________________________________________________L3
    // 创建一个collect对象
    cJSON *collect = cJSON_CreateObject();

    //___________________________________________________________________________________________________L4
    // 创建一个analogChannel数组
    cJSON *analogChannel = cJSON_CreateArray();
    cJSON_AddItemToObject(collect, "analogChannel", analogChannel);

    int Array_Size1 = sizeof(NFC_Info.analogChannel) / sizeof(NFC_Info.analogChannel[0]);

    // 创建数组中的第一个元素
    cJSON *analogElement[Array_Size1];
    for(int i = 0; i<Array_Size1; ++i){
        analogElement[i] = cJSON_CreateObject();
        cJSON_AddBoolToObject(analogElement[i], "enable", NFC_Info.analogChannel[i].enable);
        cJSON_AddNumberToObject(analogElement[i], "id", NFC_Info.analogChannel[i].id);
        cJSON_AddNumberToObject(analogElement[i], "max", NFC_Info.analogChannel[i].max);
        cJSON_AddNumberToObject(analogElement[i], "min", NFC_Info.analogChannel[i].min);
        cJSON_AddItemToArray(analogChannel, analogElement[i]);    //把{}放到[]中
    }

    //___________________________________________________________________________________________________L4
    // 创建一个digitalChannel数组
    cJSON *digitalChannel = cJSON_CreateArray();
    cJSON_AddItemToObject(collect, "digitalChannel", digitalChannel);

    int Array_Size2 = sizeof(NFC_Info.digitalChannel) / sizeof(NFC_Info.digitalChannel[0]);

    // 创建数组中的第一个元素
    cJSON *digitalElement[Array_Size2];
    for(int i = 0; i<Array_Size2; ++i){
        digitalElement[i] = cJSON_CreateObject();
        cJSON_AddBoolToObject(digitalElement[i], "enable", NFC_Info.digitalChannel[i].enable);
        cJSON_AddNumberToObject(digitalElement[i], "id", NFC_Info.digitalChannel[i].id);
        cJSON_AddNumberToObject(digitalElement[i], "mode", NFC_Info.digitalChannel[i].mode);
        cJSON_AddNumberToObject(digitalElement[i], "trigger", NFC_Info.digitalChannel[i].trigger);
        cJSON_AddNumberToObject(digitalElement[i], "upload", NFC_Info.digitalChannel[i].upload);
        cJSON_AddItemToArray(digitalChannel, digitalElement[i]);    //把{}放到[]中
    }

    //___________________________________________________________________________________________________L4
    // 创建一个power对象
    cJSON *power = cJSON_CreateObject();

    //_______________________________________________________________________________L5
    // 添加12V键值对到power对象中
    cJSON *power12V = cJSON_CreateObject();
    cJSON_AddNumberToObject(power12V, "duration", NFC_Info.duration_12V);
    cJSON_AddItemToObject(power, "12V", power12V);

    //_______________________________________________________________________________L5
    // 添加5V键值对到power对象中
    cJSON *power5V = cJSON_CreateObject();
    cJSON_AddNumberToObject(power5V, "duration", NFC_Info.duration_5V);
    cJSON_AddItemToObject(power, "5V", power5V);

    // 将power对象添加到collect对象中
    cJSON_AddItemToObject(collect, "power", power);


//_______________________________________________________________
    // 将basic对象添加到nfc对象中
    cJSON_AddItemToObject(nfc, "basic", basic);
    // 将collect值添加到nfc对象中
    cJSON_AddItemToObject(nfc, "collect", collect);

    // 将nfc对象添加到root对象中
    cJSON_AddItemToObject(root, "nfc", nfc);

    // 将root对象转化为字符串
    char *json_str = cJSON_Print(root);
    
    std::cout << "节点更新————上发的更新信息:\n" << json_str << std::endl;

    removeWhitespace(json_str);
    // printf("%s\n", json_str);



//-----------------------------------------------------------------------
    cJSON *root2 = cJSON_CreateObject();

    // 添加payload属性
    cJSON_AddStringToObject(root2, "payload", json_str);



    // 将root转换为字符串
    char *json_str2 = cJSON_Print(root2);
    // printf("%s\n", json_str2);
    // std::cout << "节点更新————上发的更新信息:" << json_str2 << std::endl;

    removeWhitespace(json_str2);
    // printf("%s\n", json_str2);

    NFC_sendJsonData(json_str2);

    // 释放内存
    cJSON_Delete(root2);
    free(json_str2);

    // 释放cJSON对象和字符串
    cJSON_Delete(root);
    free(json_str);

}






static int callback_ws_NFC(struct lws *wsi_NFC, enum lws_callback_reasons reason, void *user, void *in, size_t len)
{
    std::string getTokenStatus_url = "https://127.0.0.1:8080/api/system/token"; // 获取token的有效期
    int res = 0;
    bool log_Once4 = true;
    static bool logOnce_NFC_NotInit = true;
    switch (reason) {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:   //连接建立成功标志(三次握手)
            lwsl_notice("NFC WebSocket client connection established\n");

            connection_established_NFC = 1;
            std::cout << "thread5: NFC wss connection established, connection_established_NFC = " << connection_established_NFC << std::endl;

            //每次连接成功之后发送json数据获取nfc信息
            if (wsi_NFC != nullptr) {
                char json_data[] = "{\"type\":\"lcd\"}";
                std::cout << "NFC————切换至————读模式" << std::endl;
                lws_write(wsi_NFC, (unsigned char *)json_data, strlen(json_data), LWS_WRITE_TEXT);
            }
            break;

        case LWS_CALLBACK_CLIENT_WRITEABLE:
            if (wsi_NFC != nullptr && readNFCMode() == NFC_Write) {
                std::cout << "NFC————切换至————写模式" << std::endl;
                lws_write(wsi_NFC, (unsigned char *)WriteNFC_jsonBuffer, strlen(WriteNFC_jsonBuffer), LWS_WRITE_TEXT);
                writeNFCMode(NFC_Read);
            }
            else if(wsi_NFC != nullptr && readNFCMode() == NFC_Read){
                char json_data[] = "{\"type\":\"lcd\"}";
                std::cout << "NFC————切换至————读模式" << std::endl;
                lws_write(wsi_NFC, (unsigned char *)json_data, strlen(json_data), LWS_WRITE_TEXT);
            }
            break;

        case LWS_CALLBACK_CLIENT_RECEIVE:   //接收到数据的标志
            My_wss_JSON_GetNFCInfo(in);
            break;

        case LWS_CALLBACK_CLIENT_CLOSED:    //客户端断开的标志(四次挥手)
            connection_established_NFC = 0;
            std::cout << "thread5: NFC wss disconnection and in the close loop, connection_established_NFC = " << connection_established_NFC << std::endl;
            while(1){
                // 用于测试AS能否建立连接，能建立则退出让wss建立连接
                std::this_thread::sleep_for(std::chrono::seconds(8));
                std::string response = send_httpRequest(getTokenStatus_url, Method_get);   // http请求：Token值的有效性判断
                std::string TokenStatus = "";

                cjsonParse_getTokenStatus(response.c_str(), TokenStatus);

                if (response.empty()) {   // AS断开(在循环中，直到AS服务启动)
                    std::cout << "NFC wss close中: AS服务断开, 陷入continue" << std::endl;
                    continue;
                } else {
                    if(TokenStatus == "TokenIsInvalid"){    // 如果AS正常，如果token失效，会一直循环等待，直到最长5min后获得新token
                        std::cout << "NFC wss close中: TokenIsInvalid, 陷入continue" << std::endl;
                        continue;
                    } else {                                // 如果AS正常，如果token有效，才开始重新建立wss连接
                        std::cout << "NFC wss close中: AS服务恢复正常, token有效, break close loop, 开始重连wss" << std::endl;
                        break;
                    }
                }
            }

            break;

        default:
            break;
    }

    return 0;
}

#define WEBSOCKET_CLIENT_NFC \
{ \
	"websocket-client_NFC", \
	callback_ws_NFC, \
	0, \
	4096, \
	0, NULL, 0 \
}

struct lws_protocols protocols_NFC[] = {WEBSOCKET_CLIENT_NFC,{ NULL, NULL, 0, 0 }};



int My_wssRequest_NFC(void)
{
    //单例模式确保类只有一个实例化对象
    MyHttpsClass *http_ClassOBJ = MyHttpsClass::getInstance();


    struct lws_context_creation_info info;
    struct lws_client_connect_info connect_info;
    struct lws_context *context;
    int err;

    char *protocol = (char*)malloc(256);
    sprintf(protocol,"%s",(http_ClassOBJ->getwssToken()).c_str());

    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols_NFC;
    //初始化SSL相关的库
    info.options |= LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;

    // 设置自签名证书路径
    info.ssl_cert_filepath = "/etc/nginx/cert/lorawan.dfrobot.top.pem";
    info.ssl_private_key_filepath = "/etc/nginx/cert/lorawan.dfrobot.top.key";

    context = lws_create_context(&info);
    if (!context) {
        lwsl_err("Failed to create libwebsockets context\n");
        return -1;
    }

    memset(&connect_info, 0, sizeof(connect_info));
    connect_info.context = context;
    connect_info.address = "127.0.0.1";
    connect_info.port = 8080;
    connect_info.path = "/api/devices/nfc/stream";
    connect_info.host = "127.0.0.1:8080";
    // connect_info.origin = "https://127.0.0.1";
    connect_info.origin = "127.0.0.1:8080";
    connect_info.protocol = protocol;
    //启用SSL相关证书
    connect_info.ssl_connection |= LCCSCF_USE_SSL;
    //跳过服务器主机名检查：
    //在建立SSL连接时，客户端会检查服务器的证书中的主机名与实际连接的主机名是否匹配，这是为了防止中间人攻击。
    //跳过服务器主机名检查可能会有安全风险，因为它会放宽对服务器身份验证的要求。建议只在非生产环境或特定情况下使用这个选项，并且在生产环境中使用正确的证书配置。
    connect_info.ssl_connection |= LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    //使用自签名证书
    connect_info.ssl_connection |= LCCSCF_ALLOW_SELFSIGNED;

    bool log_Once2 = true;//用于只显示程序运行的状态（不是动作）
    while (1) {
        // 每次重新连接的时候，重新组装token
        std::string protocol_str = http_ClassOBJ->getwssToken();

        connect_info.protocol = protocol_str.c_str();
    
        wsi_NFC = lws_client_connect_via_info(&connect_info);
        
        if (wsi_NFC == NULL) {
            fprintf(stderr, "Failed to connect to server\n");
            if(log_Once2 == true){
                Logger::logToFile("thread5: NFC wss---> Failed to establish! (wsi = NULL))");
                log_Once2 = false;
            }
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }
        

        while (!connection_established_NFC) {
            log_Once2 = true;
            lws_service(context, 0);
            // std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        while (connection_established_NFC) {
            log_Once2 = true;
            lws_service(context, 0);     // wss的时间监听，每隔一段时间会退出函数一次
            // std::this_thread::sleep_for(std::chrono::milliseconds(200));
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    Logger::logToFile("thread5: NFC wss--->RTD Exited the loop, and destroy context");
    lws_context_destroy(context);

}






