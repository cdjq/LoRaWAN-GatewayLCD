#include "GatewayUI_https.h"
#include "GatewayUI_DS.h"
#include "log.h"

#include <curl/curl.h>
#include "cJSON.h"
#include <chrono>
#include <thread>
#include <condition_variable>

#include <string>
#include <cstring>
#include <cctype>



#include <ctime>
#include <iomanip>
#include <sstream>




#define MaxNodeNum 200
int Linklist_posArray[MaxNodeNum]; //最多能存放200个设备节点(预留了100个位置)——>超过这个限度可以改用动态数组或链表


/**********************************************************************************
 *        http请求部分
 **********************************************************************************/
// 回调函数用于处理HTTP响应(获取服务器响应的消息)
size_t handle_httpResponse(void* contents, size_t size, size_t nmemb, std::string* response) {
    response->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// 发送HTTP请求
std::string send_httpRequest(const std::string url, int method, const std::string& postfields) {
    CURL* curl;
    CURLcode res;
    std::string response;
    struct curl_slist *http_header = NULL;

    //单例模式确保类只有一个实例化对象
    MyHttpsClass *http_ClassOBJ = MyHttpsClass::getInstance();


    // 创建cURL对象
    curl = curl_easy_init();
    if (curl) {
        // 设置请求URL
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // 设置SSL验证
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);//不检查ssl，可访问https
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);//不检查ssl，可访问https

        // 设置回调函数并传参
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, handle_httpResponse);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);


        if(method == Method_get){
            //设置post请求的token值

            http_header = curl_slist_append(NULL, "Accept: application/json");
            http_header = curl_slist_append(NULL, (http_ClassOBJ->gethttpToken()).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);
        }
        else if(method == Method_post){

            //设置post请求和post请求的token值
            http_header = curl_slist_append(NULL, "Accept: application/json");
            http_header = curl_slist_append(NULL, (http_ClassOBJ->gethttpToken()).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);
            if (!postfields.empty()) {
                curl_easy_setopt(curl, CURLOPT_POST, 1L);//设置为非0表示本次操作为post
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());
            }else{
                std::cout << "post请求未携带负载数据可能引发错误" << std::endl;
            }
        }else if(method == Method_put){
            //设置put请求和put请求的token值
            http_header = curl_slist_append(NULL, "Accept: application/json");
            http_header = curl_slist_append(NULL, (http_ClassOBJ->gethttpToken()).c_str());
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_header);
            if (!postfields.empty()) {
                // 设置请求方法为PUT
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());
            }else{
                std::cout << "put请求未携带负载数据可能引发错误" << std::endl;
            }
        }else if(method == Method_delete){
            // printf("send delete request");
        }else if(method == Method_Token){
            if (!postfields.empty()) {
                curl_easy_setopt(curl, CURLOPT_POST, 1L);//设置为非0表示本次操作为post
                curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postfields.c_str());//post负载信息
            }else{
                std::cout << "post请求未携带负载数据可能引发错误" << std::endl;
            }
        }else{
            std::cout << "请求方法参数错误可能引发错误" << std::endl;
        }
 
        // 发送请求并获取响应
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

    }

    // 清理cURL对象
    curl_easy_cleanup(curl);

    // 清理HTTP头部
    if (http_header) {
        curl_slist_free_all(http_header);
        http_header = NULL;
    }

    return response;
}




/**********************************************************************************
 *        cJSON解析部分
 **********************************************************************************/
int cjsonParse_getToken(const char *response, std::string& loginJudgment)
{
    //单例模式确保类只有一个实例化对象
    MyHttpsClass *http_ClassOBJ = MyHttpsClass::getInstance();

    static int log_Once = 1;
    static int log_Once2 = 1;
    //JSON解析
    std::string str(response);
    
   
    cJSON *root = cJSON_Parse(response);
    if (root == nullptr) {
        std::cout << "thread1: http--->JsonParse-getToken Failed! server response content: "  << str << std::endl;
        if(log_Once == 1){
            Logger::logToFile("thread1: http--->JsonParse-getToken Failed! server response content: " + str);
            log_Once = 0;
        }
        return -1;
    }

    

    if (cJSON_IsObject(root)){
        cJSON* error = cJSON_GetObjectItem(root, "error");
        if (error != nullptr && error->type == cJSON_String) {
            std::cout << "thread1: http--->JsonParse-getToken Error: invalid username or password" << std::endl;
            if(log_Once2 == 1){
                Logger::logToFile("thread1: http--->JsonParse-getToken Error: invalid username or password");
                log_Once2 = 0;
            }
            loginJudgment = "loginError";
            cJSON_Delete(root);
            return 0;
        }


        
        cJSON *item = cJSON_GetObjectItem(root,"jwt");
        if (item != nullptr && item->type == cJSON_String) {
            std::string tempStr = item->valuestring;
            http_ClassOBJ->setToken(tempStr);   //更新token值
        }
    }

    log_Once = 1;
    log_Once2 = 1;
    cJSON_Delete(root);
    return 0;
}

int cjsonParse_getTokenStatus(const char *response, std::string& TokenStatus)
{

    static bool log_Once = true;
    static bool log_Once2 = true;
    std::string str(response);

    // 解析JSON字符串
    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        std::cout << "thread1: http--->JsonParse-getTokenStatus Failed! server responce content: " << str << std::endl;
        if(log_Once == true) {
            Logger::logToFile("thread1: http--->JsonParse-getTokenStatus Failed! server responce content: " + str);
        }
        log_Once = false;
        return -1;
    }

    log_Once = true;

    if (cJSON_IsObject(root)){
        // 判断JSON数据是否为空
        if (cJSON_IsObject(root) && cJSON_GetArraySize(root) == 0) {
            TokenStatus = "TokenIsValid";
            cJSON_Delete(root);
            return 0;
        }

        cJSON* error = cJSON_GetObjectItem(root, "error");
        if (error != nullptr && error->type == cJSON_String) {
            std::cout << "thread1: http--->JsonParse-getTokenStatus Error: authentication failed. Token invalid!" << std::endl;
            if(log_Once2 == true) {
                Logger::logToFile("thread1: http--->JsonParse-getTokenStatus Error: authentication failed. Token invalid!");
            }
            log_Once2 = false;
            TokenStatus = "TokenIsInvalid";
        }
    }

    log_Once2 = true;
    cJSON_Delete(root);
    return 0;
}




int cjsonParse_getApplicationID(const char *response)
{
    //单例模式确保类只有一个实例化对象
    MyHttpsClass *http_ClassOBJ = MyHttpsClass::getInstance();

    static bool log_Once = true;
    std::string str(response);

    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        std::cout << "thread1: http--->JsonParse-getApplicationID Failed! server responce content: " << str << std::endl;
        if(log_Once == true) {
            Logger::logToFile("thread1: http--->JsonParse-getGatewayID Failed! server responce content: " + str);
        }
        log_Once = false;
        return -1;
    }

    log_Once = true;

    if (cJSON_IsObject(root)){
        cJSON *item = cJSON_GetObjectItem(root, "result");
        if (item != NULL && cJSON_IsArray(item)) {
            int size = cJSON_GetArraySize(item);	// 获取的数组大小
            for (int i = 0; i < size; i++) {
                cJSON *obj = cJSON_GetArrayItem(item, i);		// 获取的数组里的obj
                cJSON *val = NULL;
                if (obj != NULL && cJSON_IsObject(obj)) {	// 判断数字内的元素是不是obj类型
                    val = cJSON_GetObjectItem(obj, "id");		// 获得obj里的值
                    if (val != NULL && val->type == cJSON_String) {
                        std::string tempStr = val->valuestring;
                        http_ClassOBJ->setApplicationID(tempStr);   //更新GatewayID值
                        std::cout << "thread1: http--->JsonParse-getGatewayID successfully, ApplicationID = " << tempStr << std::endl;
                    }
                }
            }
        }
    }

    cJSON_Delete(root);
    return 0;
}





int cjsonParse_getGatewayID(const char *response)
{
    //单例模式确保类只有一个实例化对象
    MyHttpsClass *http_ClassOBJ = MyHttpsClass::getInstance();

    static bool log_Once = true;
    std::string str(response);

    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        std::cout << "thread1: http--->JsonParse-getGatewayID Failed! server responce content: " << str << std::endl;
        if(log_Once == true) {
            Logger::logToFile("thread1: http--->JsonParse-getGatewayID Failed! server responce content: " + str);
        }
        log_Once = false;
        return -1;
    }

    log_Once = true;

    if (cJSON_IsObject(root)){
        cJSON *item = cJSON_GetObjectItem(root, "result");
        if (item != NULL && cJSON_IsArray(item)) {
            int size = cJSON_GetArraySize(item);	// 获取的数组大小
            for (int i = 0; i < size; i++) {
                cJSON *obj = cJSON_GetArrayItem(item, i);		// 获取的数组里的obj
                cJSON *val = NULL;
                if (obj != NULL && cJSON_IsObject(obj)) {	// 判断数字内的元素是不是obj类型
                    val = cJSON_GetObjectItem(obj, "id");		// 获得obj里的值
                    if (val != NULL && val->type == cJSON_String) {
                        std::string tempStr = val->valuestring;
                        http_ClassOBJ->setGatewayID(tempStr);   //更新GatewayID值
                        std::cout << "thread1: http--->JsonParse-getGatewayID successfully, GatewayID = " << tempStr << std::endl;
                    }
                }
            }
        }
        cJSON_Delete(root);
    }

    return 0;
}


/**********************************************************************************
 *        cJSON解析部分——>http获取网络信息
 **********************************************************************************/
// 解析获取ETH网络信息
int cjsonParse_getEthINFO(const char *response, std::string& eth_IP, std::string& eth_Mask, std::string& eth_DHCP)
{

    std::string str(response);
    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();

    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        std::cout << "http--->JsonParse-cjsonParse_getEthINFO Failed! server responce content: " << str << std::endl;
        return -1;
    }

    if (cJSON_IsObject(root)){
        cJSON *eth = cJSON_GetObjectItem(root, "eth");
        if (eth != NULL && cJSON_IsObject(eth)) {

            cJSON* eth_ip = cJSON_GetObjectItem(eth, "ip");
            if (eth_ip != NULL && cJSON_IsString(eth_ip)) {
                eth_IP = eth_ip->valuestring;
            } 

            cJSON* eth_netmask = cJSON_GetObjectItem(eth, "netmask");
            if (eth_netmask != NULL && cJSON_IsString(eth_netmask)) {
                eth_Mask = eth_netmask->valuestring;
            } 

            cJSON* eth_dhcp = cJSON_GetObjectItem(eth, "dhcp");
            if (eth_dhcp != NULL && cJSON_IsBool(eth_dhcp)) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
                    eth_DHCP = cJSON_IsTrue(eth_dhcp) ? "启用" : "禁用";
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
                    eth_DHCP = cJSON_IsTrue(eth_dhcp) ? "enable" : "disable";
                }
            }
        }
    }

    cJSON_Delete(root);

    return 0;
}


// 解析获取WIFI网络信息
int cjsonParse_getWifiINFO(const char *response, std::string& wifi_Mode, std::string& wifi_IP, std::string& wifi_Mask)
{

    std::string str(response);

    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        std::cout << "http--->JsonParse-cjsonParse_getWifiINFO Failed! server responce content: " << str << std::endl;
        return -1;
    }

    if (cJSON_IsObject(root)){
        cJSON *wifiAP = cJSON_GetObjectItem(root, "wifiAP");
        if (wifiAP != NULL && cJSON_IsObject(wifiAP)) {

            wifi_Mode = "AP";

            cJSON* wifiAP_ip = cJSON_GetObjectItem(wifiAP, "ip");
            if (wifiAP_ip != NULL && cJSON_IsString(wifiAP_ip)) {
                wifi_IP = wifiAP_ip->valuestring;
            }

            cJSON* wifiAP_netmask = cJSON_GetObjectItem(wifiAP, "netmask");
            if (wifiAP_netmask != NULL && cJSON_IsString(wifiAP_netmask)) {
                wifi_Mask = wifiAP_netmask->valuestring;
            }
        }

        cJSON *wifiSTA = cJSON_GetObjectItem(root, "wifiSTA");
        if (wifiSTA != NULL && cJSON_IsObject(wifiSTA)) {

           wifi_Mode = "STA";

            cJSON* wifiSTA_ip = cJSON_GetObjectItem(wifiSTA, "ip");
            if (wifiSTA_ip != NULL && cJSON_IsString(wifiSTA_ip)) {
                wifi_IP = wifiSTA_ip->valuestring;
            }

            cJSON* wifiSTA_netmask = cJSON_GetObjectItem(wifiSTA, "netmask");
            if (wifiSTA_netmask != NULL && cJSON_IsString(wifiSTA_netmask)) {
                wifi_Mask = wifiSTA_netmask->valuestring;
            }
        }
    }

    cJSON_Delete(root);

    return 0;
}


// 解析获取蜂窝网络信息
int cjsonParse_getCat1INFO(const char *response,  std::string& Cat1_IP, std::string& Cat1_SIM, std::string& Cat1_Mode, std::string& Cat1_SNR)
{

    std::string str(response);
    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();

    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        std::cout << "http--->JsonParse-cjsonParse_getCat1INFO Failed! server responce content: " << str << std::endl;
        return -1;
    }

    if (cJSON_IsObject(root)){
        cJSON *cat1 = cJSON_GetObjectItem(root, "cat1");
        if (cat1 != NULL && cJSON_IsObject(cat1)) {

            cJSON* cat1_ip = cJSON_GetObjectItem(cat1, "publicIP");
            if (cat1_ip != NULL && cJSON_IsString(cat1_ip)) {
                Cat1_IP = cat1_ip->valuestring;
            }

            cJSON* cat1_card = cJSON_GetObjectItem(cat1, "card");
            if (cat1_card != NULL && cJSON_IsBool(cat1_card)) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
                    Cat1_SIM = cJSON_IsTrue(cat1_card) ? "已识别" : "未识别";
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
                    Cat1_SIM = cJSON_IsTrue(cat1_card) ? "Recognized" : "Not recognized";
                }
            }

            cJSON* cat1_enable = cJSON_GetObjectItem(cat1, "enable");
            if (cat1_enable != NULL && cJSON_IsBool(cat1_enable)) {
                if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
                    Cat1_Mode = cJSON_IsTrue(cat1_enable) ? "启用" : "禁用";
                } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
                    Cat1_Mode = cJSON_IsTrue(cat1_enable) ? "enable" : "disable";
                }
            }

            cJSON* cat1_snr = cJSON_GetObjectItem(cat1, "snr");
            if (cat1_snr != NULL && cJSON_IsNumber(cat1_snr)) {
                if (cat1_snr->valueint>0 && cat1_snr->valueint <= 5 ) {
                    if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
                        Cat1_SNR = "差";
                    } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
                        Cat1_SNR = "Bad";
                    }
                } else if (cat1_snr->valueint > 5 && cat1_snr->valueint < 15) {
                    if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
                        Cat1_SNR = "中";
                    } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
                        Cat1_SNR = "Medium";
                    }
                } else if (cat1_snr->valueint == 0 || cat1_snr->valueint < 0) {
                    Cat1_SNR = "";
                }else {
                    if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 0){
                        Cat1_SNR = "优";
                    } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == 3) {
                        Cat1_SNR = "Good";
                    }
                }
            }
        }
    }

    cJSON_Delete(root);

    return 0;
}






std::tm parseDateTime(const std::string& dateTimeStr) {
    std::tm dateTime = {};
    std::istringstream ss(dateTimeStr);
    ss >> std::get_time(&dateTime, "%Y-%m-%dT%H:%M:%S");
    return dateTime;
}

void My_TimeDiff(const char *datetime, char* Mystr) {

    
    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();


    // 获取当前时间
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm* nowDateTime = std::localtime(&nowTime);

    // 将字符串解析为时间
    std::string dateTimeStr(datetime);
    std::tm targetDateTime = parseDateTime(dateTimeStr);

    // 计算时间差
    std::chrono::system_clock::time_point targetTime = std::chrono::system_clock::from_time_t(std::mktime(&targetDateTime));
    std::chrono::duration<double> diff = now - targetTime;

    int timeDiff = diff.count();
    char Temp_String[20];

    const int SECONDS_PER_MINUTE = 60;
    const int MINUTES_PER_HOUR = 60;
    const int HOURS_PER_DAY = 24;
    const int DAYS_PER_YEAR = 365;

    int seconds = timeDiff % SECONDS_PER_MINUTE;
    int minutes = (timeDiff / SECONDS_PER_MINUTE) % MINUTES_PER_HOUR;
    int hours = (timeDiff / (SECONDS_PER_MINUTE * MINUTES_PER_HOUR)) % HOURS_PER_DAY;
    int days = (timeDiff / (SECONDS_PER_MINUTE * MINUTES_PER_HOUR * HOURS_PER_DAY)) % DAYS_PER_YEAR;
    days = days / 30;
    int mouths = days % 30;
    int years = timeDiff / (SECONDS_PER_MINUTE * MINUTES_PER_HOUR * HOURS_PER_DAY * DAYS_PER_YEAR);

    if(years > 0){
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            sprintf(Temp_String,"%d年前",years);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && years > 1) {
            sprintf(Temp_String,"%d years ago",years);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            sprintf(Temp_String,"%d year ago",years);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        }
    }else if(mouths > 0){
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            sprintf(Temp_String,"%d月前",mouths);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && mouths > 1) {
            sprintf(Temp_String,"%d mouths ago",mouths);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            sprintf(Temp_String,"%d mouth ago",mouths);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        }
    }else if(days > 0){
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            sprintf(Temp_String,"%d天前",days);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && days > 1) {
            sprintf(Temp_String,"%d days ago",days);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            sprintf(Temp_String,"%d day ago",days);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        }
    }else if(hours > 0){
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            sprintf(Temp_String,"%d小时前",hours);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && hours > 1) {
            sprintf(Temp_String,"%d hours ago",hours);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            sprintf(Temp_String,"%d hour ago",days);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        }
    }else if(minutes > 0) {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            sprintf(Temp_String,"%d分钟前",minutes);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH && minutes > 1) {
            sprintf(Temp_String,"%d minutes ago",minutes);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            sprintf(Temp_String,"%d minute ago",days);
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        }
    }else if(seconds > 0) {
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            sprintf(Temp_String,"刚刚");
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            sprintf(Temp_String,"just now");
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        }
    }else{
        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
            sprintf(Temp_String,"未来时间");
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
            sprintf(Temp_String,"Time in the future");
            memcpy(Mystr, Temp_String, strlen(Temp_String) + 1);
        }
    }
}


int cjsonParse_getappEUI(const char *response, char *appEUI)
{

    static bool log_Once = true;
    std::string str(response);

    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        std::cout << "thread1: http--->JsonParse-getappEUI Failed! server responce content: " << str << std::endl;
        if(log_Once == true) {
            Logger::logToFile("thread1: http--->JsonParse-getappEUI Failed! server responce content: " + str);
        }
        log_Once = false;
        return -1;
    }

    log_Once = true;

    if (cJSON_IsObject(root)){
        //JSON数据的项（可以表示根项，也可以表示子项——>由cJSON_GetObjectItem这个的第一个参数决定）
        cJSON *item = cJSON_GetObjectItem(root,"deviceActivation");
        if (item != NULL && cJSON_IsObject(item)) {
            item = cJSON_GetObjectItem(item, "joinEUI");		// 获得obj里的值
            if (item != NULL && item->type == cJSON_String) {
                strcpy(appEUI, item->valuestring);
                // memcpy(appEUI, item->valuestring, strlen(item->valuestring) + 1);
                // std::cout << "/* message */" <<appEUI<< std::endl;
                // 将拆分得到的字符串转换为大写并存储到结构体成员中
                for (int i = 0; appEUI[i] != '\0'; i++) {
                    appEUI[i] = toupper(appEUI[i]);
                }
            }
        }else{
                strcpy(appEUI, "");
        }
        cJSON_Delete(root);
    }
    return 0;
}






int cjsonParse_getNodeDevINFO(const char *response)
{
    //单例模式确保类只有一个实例化对象
    LoraNodeDeviceClass *LoraNode_ClassOBJ = LoraNodeDeviceClass::getInstance();
    
    languageSwitchClass *languageSwitch_ClassOBJ = languageSwitchClass::getInstance();



    LORA_ANODE_INFO ANode_Info = {};

    static bool log_Once = true;
    std::string str(response);

    int index = 0;
    for(int i = 0; i < MaxNodeNum; i++){
        Linklist_posArray[i] = 0;
    }

    cJSON *root = cJSON_Parse(response);
    if (root == NULL) {
        std::cout << "thread1: http--->JsonParse-getNodeDevINFO Failed! server responce content: " << str << std::endl;
        if(log_Once == true) {
            Logger::logToFile("thread1: http--->JsonParse-getNodeDevINFO Failed! server responce content: " + str);
        }
        log_Once = false;
        return -1;
    }

    log_Once = true;

    if (cJSON_IsObject(root)){
        cJSON *item = cJSON_GetObjectItem(root,"totalCount");
        if (item != NULL && item->type == cJSON_Number) {
            int totalCount = item->valueint;    //注册的节点个数
        }

        item = cJSON_GetObjectItem(root, "result");
        if (item != NULL && cJSON_IsArray(item)) {
            int size = cJSON_GetArraySize(item);	// 获取的数组大小
            for (int i = 0; i < size; i++) {
                cJSON *obj = cJSON_GetArrayItem(item, i);		// 获取的数组里的obj
                cJSON *val = NULL;
                if (obj != NULL && cJSON_IsObject(obj)) {	// 判断数字内的元素是不是obj类型
                    //-----------------------------------------------------------
                    val = cJSON_GetObjectItem(obj, "name");
                    if (val != NULL && val->type == cJSON_String) {
                        char *v_str = val->valuestring;
                        strcpy(ANode_Info.devName, v_str);
                        // printf("devName = %s\n", ANode_Info.devName);
                    }
                    //-----------------------------------------------------------
                    val = cJSON_GetObjectItem(obj, "devEUI");
                    if (val != NULL && val->type == cJSON_String) {
                        char *v_str = val->valuestring;
                        for (int i = 0; v_str[i] != '\0'; i++) {
                            v_str[i] = toupper(v_str[i]);
                        }
                        strcpy(ANode_Info.devEUI, v_str);
                        // printf("devEUI = %s\n", ANode_Info.devEUI);
                    }
                    //-----------------------------------------------------------
                    val = cJSON_GetObjectItem(obj, "lastSeenAt");		// 获得obj里的值
                    if (val != NULL && val->type == cJSON_String) {
                        char *v_str = val->valuestring;
                        char *MYstr = new char[20];
                        My_TimeDiff(v_str, MYstr);
                        strcpy(ANode_Info.lastOnlineTime, MYstr);
                        // printf("lastSeenAt = %s\n", ANode_Info.lastOnlineTime);
                        delete[] MYstr;
                    }else{
                        if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == CHINESE){
                            strcpy(ANode_Info.lastOnlineTime, "未通信");
                        } else if (languageSwitch_ClassOBJ->get_CurrentLanguageType() == ENGLISH) {
                            strcpy(ANode_Info.lastOnlineTime, "Not communicated");
                        }
                    }
                    //-----------------------------------------------------------
                    val = cJSON_GetObjectItem(obj, "description");
                    if (val != NULL && val->type == cJSON_String) {
                        char *v_str = val->valuestring;
                        strcpy(ANode_Info.Remark, v_str);
                        // printf("description = %s\n", ANode_Info.Remark);
                    }
                    //-----------------------------------------------------------
                    val = cJSON_GetObjectItem(obj, "deviceStatusBattery");
                    if (val != NULL && val->type == cJSON_Number) {
                        int value = val->valueint;
                        ANode_Info.Battery = value;
                        // printf("deviceStatusBattery = %d\n", ANode_Info.Battery);
                    }
                    //-----------------------------------------------------------
                    char *split_StrArray;
                    val = cJSON_GetObjectItem(obj, "deviceProfileName");
                    if (val != NULL && val->type == cJSON_String) {
                        char *v_str = val->valuestring;
                        // 使用strtok函数拆分字符串
                        split_StrArray = strtok(v_str, "_");
                        if (split_StrArray != NULL) {
                            // 将拆分得到的字符串转换为大写并存储到结构体成员中
                            for (int i = 0; split_StrArray[i] != '\0'; i++) {
                                split_StrArray[i] = toupper(split_StrArray[i]);
                            }
                            strcpy(ANode_Info.joinType, split_StrArray);
                            // printf("joinType = %s\n", ANode_Info.joinType);
                            // 继续拆分下一个字符串
                            split_StrArray = strtok(NULL, "_");
                        }
                        if (split_StrArray != NULL) {
                            // 将拆分得到的字符串转换为大写并存储到结构体成员中
                            for (int i = 0; split_StrArray[i] != '\0'; i++) {
                                split_StrArray[i] = toupper(split_StrArray[i]);
                            }
                            strcpy(ANode_Info.loraClass, split_StrArray);
                            // printf("loraClass = %s\n", ANode_Info.loraClass);
                            // 继续拆分下一个字符串
                            split_StrArray = strtok(NULL, "_");
                        }
                        if (split_StrArray != NULL) {
                            const char* str = split_StrArray;
                            strcpy(ANode_Info.MacVersion, split_StrArray);
                            // printf("MacVersion = %s\n", ANode_Info.MacVersion);
                        }
                    }
                    //-----------------------------------------------------------
                    // 发送http请求获取指定devEUI设备的appEUI
                    char geturl[96];
                    sprintf(geturl,"https://10.6.6.6:8080/api/devices/%s/activation", ANode_Info.devEUI);
                    // std::cout << geturl << std::endl;
                    std::string url(geturl, sizeof(geturl));
                    std::string response_appEUI = send_httpRequest(url, Method_get);
                    char *appEUI_str = new char[36];
                    cjsonParse_getappEUI(response_appEUI.c_str(), appEUI_str);
                    strcpy(ANode_Info.appEUI, appEUI_str);
                    delete[] appEUI_str;
                    //-----------------------------------------------------------
                    int res = LoraNode_ClassOBJ->findNodePositionWithDevEUI(ANode_Info.devEUI);
                    if( res == 0){
                        LoraNode_ClassOBJ->Add_NodeDev_AfterInit(ANode_Info.devEUI);    //尾插list节点
                        int add_pos = LoraNode_ClassOBJ->get_ListLength();
                        //将新增的位置添加至Linklist_posArray数组中，方便后面比对删除(因为是尾插元素，所以链表长度就是插入的元素位置)
                        Linklist_posArray[index] = add_pos;
                        index++;
                    }else{
                        //将相同devEUI的位置返回 放入一个数组中
                        Linklist_posArray[index] = res;
                        index++;
                    }
                    LoraNode_ClassOBJ->set_ANode_Info(ANode_Info.devEUI, ANode_Info);
                    std::cout << "线程1:Node信息结构更新完毕" << std::endl;
                    
                }
            }
        }

    }


    //for循环完毕之后——>所有的devEUI都在list容器中进行了比对
    //开始在list容器中进行删除操作
    //Linklist_posArray数组存储的位置也确定了，并对数组从大到小排序，找到未被叫到号的节点将其删除
    int size_temp = sizeof(Linklist_posArray) / sizeof(Linklist_posArray[0]);
    for (int i = 0; i < size_temp-1; i++) {
        int maxIndex = i;
        for (int j = i + 1; j < size_temp; j++) {
            if (Linklist_posArray[j] > Linklist_posArray[maxIndex]) {
                maxIndex = j;
            }
        }
        int temp = Linklist_posArray[i];
        Linklist_posArray[i] = Linklist_posArray[maxIndex];
        Linklist_posArray[maxIndex] = temp;
    }

    //获取list容器的最大长度,并创建一个数组用于比对
    int pLink_MaxNum = LoraNode_ClassOBJ->get_ListLength();
    int* array = new int[pLink_MaxNum];// 在堆区动态分配内存
    memset(array, 0, pLink_MaxNum * sizeof(int));// 使用 memset 初始化数组为 0
    //根据list容器元素数量设计54321的数组
    for (int i = 0; i < pLink_MaxNum; i++) {
        array[i] = pLink_MaxNum - i;
    }

    //找出array在Linklist_posArray没有的元素——>并进行链表的删除操作
    int found;
    for (int i = 0; i < pLink_MaxNum; i++) {
        found = 0;
        for (int j = 0; j < pLink_MaxNum; j++) {
            if (array[i] == Linklist_posArray[j]) {
                found = 1;  //找到相同元素,无需删除，继续遍历
                // printf("找到了相同元素\n");
                break;
            }
        }
        if (!found) {   //一次循环中未找到相同元素的，就进行删除操作
            //进行删除节点操作
            // printf("删除指定位置的节点:%d\n",array[i]);
            LoraNode_ClassOBJ->Remove_NodeDev_ByIndex(array[i]-1);
        }
    }
    // 释放动态分配的内存
    delete[] array;

    cJSON_Delete(root);
    return 0;
}






int cjsonParse_NodeRegistration(const char *response, std::string& Join_status)
{

    static bool log_Once = true;
    std::string str(response);

    // 解析JSON字符串
    cJSON *root = cJSON_Parse(response);
    std::cout << "节点注册————服务器返回的Json数据:"<< response << std::endl;
    if (root == NULL) {
        std::cout << "Registration Botton: http--->JsonParse-NodeRegistration Failed! server responce content: " << str << std::endl;
        if(log_Once == true) {
            Logger::logToFile("Registration Botton: http--->JsonParse-NodeRegistration Failed! server responce content: " + str);
        }
        log_Once = false;
        Join_status = "Join_argError";
        return 1;
    }
    log_Once = true;

    if (cJSON_IsObject(root)){

        // 判断JSON数据是否为空
        if (cJSON_IsObject(root) && cJSON_GetArraySize(root) == 0) {
            printf("节点注册————检测到Json空包,入网成功\n");

            Join_status = "Join_OK";
            
            // 释放cJSON结构内存
            cJSON_Delete(root);
            return 0;
        }

        // 提取各个字段的值
        // cJSON *errorNode = cJSON_GetObjectItem(root, "error");
        cJSON *codeNode = cJSON_GetObjectItem(root, "code");
        // cJSON *messageNode = cJSON_GetObjectItem(root, "message");
        // cJSON *detailsNode = cJSON_GetObjectItem(root, "details");

        // 打印各字段的值
        // printf("error: %s\n", errorNode->valuestring);
        // printf("code: %d\n", codeNode->valueint);
        // printf("message: %s\n", messageNode->valuestring);
        if(codeNode->valueint == 3){
            Join_status = "Join_argError";
        }else if(codeNode->valueint == 6){
            Join_status = "Join_isExist";
        }
        
        // 释放cJSON结构内存
        cJSON_Delete(root);
    }

    return 0;
}



/**********************************************************************************
 *        相关功能的http请求封装
 **********************************************************************************/
void My_httpRequest_GetToken(void)
{

    // 初始化cURL库
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // 获取token
    std::string getToken_url = "https://127.0.0.1:8080/api/internal/login";
    std::string postfields = "{\"email\":\"admin\",\"password\":\"admin\"}";

    // 获取网关ID
    std::string getGatewayID_url = "https://127.0.0.1:8080/api/gateways?limit=10&offset=0"; //如果是多网关此代码就会有异常<如收不到RTD...>（需要修改获取网关的JSON解析——>获取指定网关设备）

    // 获取token的有效期
    std::string getTokenStatus_url = "https://127.0.0.1:8080/api/system/token";

    // 获取applicationID
    std::string getapplicationID_url = "https://127.0.0.1:8080/api/applications";

    // 当重新获取token时，获取一次节点设备列表（其他情况都是数据库对节点信息有增删改查操作才进行get）
    std::string getNodeInfo_url = "https://127.0.0.1:8080/api/devices?applicationID=0&limit=200&offset=0";

    bool connected = false;
    bool log_Once = true;
    //token的有效期是24h, 使用自签名证书（获取得到token后，启动token有效期判断的http请求，每5min请求一次，当token失效后重新获取token）
    while (true) {
        if (!connected) {
            std::string response = send_httpRequest(getToken_url, Method_Token, postfields);       // http请求1：获取token值并判断服务器状态
            std::string loginJudgment = "";
            cjsonParse_getToken(response.c_str(), loginJudgment);
            if (!response.empty() && loginJudgment != "loginError") {   //post账号密码错误后continue重连
                
                connected = true;
                write_ServerStatus(ServerStatus_True);  //记录服务器的连接状态（多线程安全的无锁原子操作）

                std::cout << "thread1: http--->Connected to the server." << std::endl;
                Logger::logToFile("thread1: http--->Connected to the server.");
                log_Once = true;
                
                
                std::string response_getID = send_httpRequest(getGatewayID_url, Method_get);        // http请求2：获取得到token值后立马获取GatewayID值
                cjsonParse_getGatewayID(response_getID.c_str());

                
                std::string response_getappID = send_httpRequest(getapplicationID_url, Method_get); // http请求3：获取applicationID值
                cjsonParse_getApplicationID(response_getappID.c_str());
                
                {
                    std::lock_guard<std::mutex> lock(mutex_NodeTabel);
                    modified_Node = true;
                    std::string response_NodeINFO = send_httpRequest(getNodeInfo_url, Method_get);  // http请求4：获取节点设备信息
                    cjsonParse_getNodeDevINFO(response_NodeINFO.c_str());
                    std::cout << "Node表格————更新表格信号————发出" << std::endl;
                    cv_NodeTabel.notify_one();  
                }


            } else {          
                write_ServerStatus(ServerStatus_Flase);  //记录服务器的连接状态（多线程安全的无锁原子操作）
                std::cout << "thread1: http--->Server connection failed, Enter the loop connection..." << std::endl;
                if(log_Once == true){
                    Logger::logToFile("thread1: http--->Server connection failed, Enter the loop connection...");
                    log_Once = false;
                }
                std::this_thread::sleep_for(std::chrono::seconds(3));
            }
        } 
        else {      // 连接成功后才会进入此分支

            std::string response = send_httpRequest(getTokenStatus_url, Method_get);                // http请求5：Token值的有效性判断
            std::string TokenStatus = "";

            cjsonParse_getTokenStatus(response.c_str(), TokenStatus);

            if (response.empty()) {                                                                 // http请求没有返回结果，说明AS服务器断开，回到上面的分支进行重新请求服务器连接(并获取token)
                connected = false;

                std::cout << "thread1: http--->Server disconnected, Reconnecting in 3 seconds..." << std::endl;
                Logger::logToFile("thread1: http--->Server disconnected, Reconnecting in 3 seconds...");
                std::this_thread::sleep_for(std::chrono::seconds(3));
            } else {
                if(TokenStatus == "TokenIsInvalid"){    // 获取的token失效后，回到上面的分支重新获取token
                    connected = false;
                    continue;
                }
                std::this_thread::sleep_for(std::chrono::seconds(300));     // 每5min获取一次token的有效性（失效的5min内，获取得到的所有http，wss都将返回认证失败的JSON）
            }
        }
    }

    
    // 清理cURL库
    curl_global_cleanup();
}



// 在name末尾插入数字
void insertNumberToName(char* name, int number, char* Name_Old) {
    std::string numStr = std::to_string(number);
    std::string newName = std::string(Name_Old) + "-" + numStr;
    
    strcpy(name, newName.c_str());
}



std::string My_httpRequest_NodeRegistration()
{

    //单例模式确保类只有一个实例化对象
    MyHttpsClass *http_ClassOBJ = MyHttpsClass::getInstance();

    LORA_NFC_INFO  NFC_Info = read_NFCInfo();

    char* NameStr = new char[60];
    sprintf(NameStr, "%s-%s", NFC_Info.name, NFC_Info.devEUI);

    std::cout << "节点注册时的Name = " << NameStr << std::endl;

    cJSON *root = cJSON_CreateObject();// 创建根节点
    cJSON *otaaNode = cJSON_CreateObject();// 创建OTAA与ABP节点
    cJSON *abpNode = cJSON_CreateObject();




    // 在OTAA节点下创建子节点
    if(strncmp(NFC_Info.joinType,"ABP",3) == 0){
        cJSON_AddItemToObject(abpNode, "APPSKEY", cJSON_CreateString(NFC_Info.appSkey));
        cJSON_AddItemToObject(abpNode, "NWKSKEY", cJSON_CreateString(NFC_Info.nwkSKey));
        cJSON_AddItemToObject(abpNode, "DEVADDR", cJSON_CreateString(NFC_Info.devAddr));
        cJSON_AddItemToObject(abpNode, "DEVEUI", cJSON_CreateString(NFC_Info.devEUI));
        cJSON_AddItemToObject(abpNode, "MAC", cJSON_CreateString(NFC_Info.macVersion));
        cJSON_AddItemToObject(abpNode, "TYPE", cJSON_CreateString(NFC_Info.loraClass));
        cJSON_AddItemToObject(abpNode, "applicationID", cJSON_CreateString(http_ClassOBJ->getApplicationID().c_str()));
        cJSON_AddItemToObject(abpNode, "description", cJSON_CreateString("暂无"));
        cJSON_AddItemToObject(abpNode, "skipFCntCheck", cJSON_CreateBool(cJSON_True));
        // cJSON_AddItemToObject(abpNode, "name", cJSON_CreateString(NFC_Info.name));
        cJSON_AddItemToObject(abpNode, "name", cJSON_CreateString(NameStr));
        cJSON_AddItemToObject(root, "ABP", abpNode);
    }else if(strncmp(NFC_Info.joinType,"OTAA",4) == 0){
        // cJSON_AddItemToObject(otaaNode, "name", cJSON_CreateString(NFC_Info.name));
        cJSON_AddItemToObject(otaaNode, "name", cJSON_CreateString(NameStr));
        cJSON_AddItemToObject(otaaNode, "DEVEUI", cJSON_CreateString(NFC_Info.devEUI));
        cJSON_AddItemToObject(otaaNode, "APPEUI", cJSON_CreateString(NFC_Info.joinEUI));
        cJSON_AddItemToObject(otaaNode, "APPKEY", cJSON_CreateString(NFC_Info.appkey));
        cJSON_AddItemToObject(otaaNode, "MAC", cJSON_CreateString(NFC_Info.macVersion));
        cJSON_AddItemToObject(otaaNode, "TYPE", cJSON_CreateString(NFC_Info.loraClass));
        cJSON_AddItemToObject(otaaNode, "applicationID", cJSON_CreateString(http_ClassOBJ->getApplicationID().c_str()));
        cJSON_AddItemToObject(abpNode, "description", cJSON_CreateString("暂无"));
        cJSON_AddItemToObject(otaaNode, "skipFCntCheck", cJSON_CreateBool(cJSON_True));
        cJSON_AddItemToObject(root, "OTAA", otaaNode);
    }
    
    // 将cJSON结构转换为字符串
    char *post_jsonfields = cJSON_Print(root);
    
    // 打印结果
    // printf("%s\n", post_jsonfields);
    std::string jsonString(post_jsonfields);
    std::cout << "节点注册————post的注册信息:\n" << post_jsonfields << std::endl;

    free(post_jsonfields);  //cJSON_Print函数会动态分配内存来存储生成的JSON字符串，因此在使用完毕后需要手动释放该内存，以防止内存泄漏
    cJSON_Delete(root);

    char *nodeRegister_url = "https://127.0.0.1:8080/api/devices";

    std::string response = send_httpRequest(nodeRegister_url, Method_post, jsonString);         // 内存泄漏？？？？？？？？？

    std::string Join_status = "";
    cjsonParse_NodeRegistration(response.c_str(), Join_status);
    std::cout << "Join_status" << Join_status << std::endl;

    delete[] NameStr;
    return Join_status;
}



//_______________________________________________________________________________________________________________


int cjsonParse_getNodeState(const char *response)
{
    LORA_NODESTATE_INFO nodeState_Info = {};

    cJSON *root=NULL;
    cJSON *root_Level_1=NULL;
    cJSON *root_Level_2=NULL;

    // 解析JSON字符串
    root = cJSON_Parse(response);
    std::cout << "get节点状态————服务器返回的Json数据:"<< response << std::endl;
    if (root == NULL) {
        printf("Failed to parse JSON string.\n");
        Logger::logToFile("http--->getNodeState (Failed to parse JSON)");
        return -1;
    }

        
    if (cJSON_IsObject(root)){
        root_Level_1 = cJSON_GetObjectItem(root,"device");  //(↓)

        if (root_Level_1 != NULL && cJSON_IsObject(root_Level_1)) {
            //Level_2中——>devEUI(↓)———————————————————————————————————————————————————————————|
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "devEUI");		// 获得obj里的值
            if (root_Level_2 != NULL && root_Level_2->type == cJSON_String) {
                strncpy(nodeState_Info.devEUI, root_Level_2->valuestring, sizeof(nodeState_Info.devEUI));
                std::cout << " devEUI " << nodeState_Info.devEUI << std::endl;
            }
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "name");		// 获得obj里的值
            if (root_Level_2 != NULL && root_Level_2->type == cJSON_String) {
                strncpy(nodeState_Info.name, root_Level_2->valuestring, sizeof(nodeState_Info.name));
                std::cout << " name " << nodeState_Info.name << std::endl;
            }        
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "applicationID");		// 获得obj里的值
            if (root_Level_2 != NULL && root_Level_2->type == cJSON_String) {
                strncpy(nodeState_Info.applicationID, root_Level_2->valuestring, sizeof(nodeState_Info.applicationID));
                std::cout << " applicationID " << nodeState_Info.applicationID << std::endl;
            }        
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "description");		// 获得obj里的值
            if (root_Level_2 != NULL && root_Level_2->type == cJSON_String) {
                strncpy(nodeState_Info.description, root_Level_2->valuestring, sizeof(nodeState_Info.description));
                std::cout << " description " << nodeState_Info.description << std::endl;
            }
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "deviceProfileID");		// 获得obj里的值
            if (root_Level_2 != NULL && root_Level_2->type == cJSON_String) {
                strncpy(nodeState_Info.deviceProfileID, root_Level_2->valuestring, sizeof(nodeState_Info.deviceProfileID));
                std::cout << " deviceProfileID " << nodeState_Info.deviceProfileID << std::endl;
            }
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "skipFCntCheck");		// 获得obj里的值
            if (root_Level_2 != NULL && cJSON_IsBool(root_Level_2)) {
                nodeState_Info.skipFCntCheck = root_Level_2->valueint;
                std::cout << " skipFCntCheck " << nodeState_Info.skipFCntCheck << std::endl;
            }
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "referenceAltitude");		// 获得obj里的值
            if (root_Level_2 != NULL && root_Level_2->type == cJSON_Number) {
                nodeState_Info.referenceAltitude = root_Level_2->valuedouble;
                std::cout << " referenceAltitude " << nodeState_Info.referenceAltitude << std::endl;
            }
            root_Level_2 = cJSON_GetObjectItem(root_Level_1, "isDisabled");		// 获得obj里的值
            if (root_Level_2 != NULL && cJSON_IsBool(root_Level_2)) {
                nodeState_Info.isDisabled = root_Level_2->valueint;
                std::cout << " isDisabled " << nodeState_Info.isDisabled << std::endl;
            }
        }

        root_Level_1 = cJSON_GetObjectItem(root, "createdAt");		// 获得obj里的值
        if (root_Level_1 != NULL && root_Level_1->type == cJSON_String) {
        strncpy(nodeState_Info.createdAt, root_Level_1->valuestring, sizeof(nodeState_Info.createdAt));
        std::cout << " createdAt " << nodeState_Info.createdAt << std::endl;
        }
        root_Level_1 = cJSON_GetObjectItem(root, "updatedAt");		// 获得obj里的值
        if (root_Level_1 != NULL && root_Level_1->type == cJSON_String) {
        strncpy(nodeState_Info.updatedAt, root_Level_1->valuestring, sizeof(nodeState_Info.updatedAt));
        std::cout << " updatedAt " << nodeState_Info.updatedAt << std::endl;
        }
        root_Level_1 = cJSON_GetObjectItem(root, "lastSeenAt");		// 获得obj里的值
        if (root_Level_1 != NULL && root_Level_1->type == cJSON_String) {
        strncpy(nodeState_Info.lastSeenAt, root_Level_1->valuestring, sizeof(nodeState_Info.lastSeenAt));
        std::cout << " lastSeenAt " << nodeState_Info.lastSeenAt << std::endl;
        }
        root_Level_1 = cJSON_GetObjectItem(root, "deviceStatusBattery");		// 获得obj里的值
        if (root_Level_1 != NULL && root_Level_1->type == cJSON_Number) {
        nodeState_Info.deviceStatusBattery = root_Level_1->valueint;
        std::cout << " deviceStatusBattery " << nodeState_Info.deviceStatusBattery << std::endl;
        }
        root_Level_1 = cJSON_GetObjectItem(root, "deviceStatusMargin");		// 获得obj里的值
        if (root_Level_1 != NULL && root_Level_1->type == cJSON_Number) {
        nodeState_Info.deviceStatusMargin = root_Level_1->valueint;
        std::cout << " deviceStatusMargin " << nodeState_Info.deviceStatusMargin << std::endl;
        }

        write_NodeStateInfo(nodeState_Info);
        std::cout << "成功退出了get节点状态的函数" << std::endl;

        cJSON_Delete(root);
    }
    return 0;
}



void My_httpRequest_getNodeState(const char* devEUI)
{
    char temp[96];
    sprintf(temp, "https://127.0.0.1:8080/api/devices/%s", devEUI);

    std::string nodeState_url(temp, sizeof(temp));

    std::string response = send_httpRequest(nodeState_url, Method_get);
    cjsonParse_getNodeState(response.c_str());
}


//---------------------------------------------------------------------------------------------------

int cjsonParse_postNodeState(const char *response)
{
    // 解析JSON字符串
    cJSON *root = cJSON_Parse(response);
    std::cout << "post节点状态————服务器返回的Json数据:"<< response << std::endl;
    if (root == NULL) {
        printf("Failed to parse JSON string.\n");
        Logger::logToFile("http--->postNodeState (Failed to parse JSON)");
        return 1;
    }

        
    if (cJSON_IsObject(root)){
        // 判断JSON数据是否为空
        if (cJSON_IsObject(root) && cJSON_GetArraySize(root) == 0) {
            printf("post节点状态————检测到Json空包,设置入网状态成功\n");

            // 释放cJSON结构内存
            cJSON_Delete(root);
            return 0;
        }
    }


    // 释放cJSON结构内存
    cJSON_Delete(root);
    return 0;
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


void My_httpRequest_putNodeState(const char* devEUI ,bool isDisabled)
{
    std::cout << "进入了put节点状态信息函数:" << isDisabled << std::endl;

    LORA_NODESTATE_INFO nodeState_Info = read_NodeStateInfo();


    // 创建根节点
    cJSON *root = cJSON_CreateObject();

    // 创建device子节点
    cJSON *device = cJSON_CreateObject();
    cJSON_AddStringToObject(device, "devEUI", nodeState_Info.devEUI);
    cJSON_AddStringToObject(device, "name", nodeState_Info.name);
    cJSON_AddStringToObject(device, "applicationID", nodeState_Info.applicationID);
    cJSON_AddStringToObject(device, "description", nodeState_Info.description);
    cJSON_AddStringToObject(device, "deviceProfileID", nodeState_Info.deviceProfileID);
    cJSON_AddBoolToObject(device, "skipFCntCheck", nodeState_Info.skipFCntCheck);
    cJSON_AddNumberToObject(device, "referenceAltitude", nodeState_Info.referenceAltitude);
    cJSON_AddObjectToObject(device, "variables");
    cJSON_AddObjectToObject(device, "tags");
    cJSON_AddStringToObject(device, "debug", "");
    if (isDisabled == true) {
        cJSON_AddBoolToObject(device, "isDisabled", true);
    } else {
        cJSON_AddBoolToObject(device, "isDisabled", false);
    }
    

    cJSON_AddItemToObject(root, "device", device);

    // 添加其他字段
    cJSON_AddStringToObject(root, "createdAt", nodeState_Info.createdAt);
    if(strcmp(nodeState_Info.updatedAt, "") == 0){
        cJSON_AddNullToObject(root, "updatedAt");    
    }else{
        cJSON_AddStringToObject(root, "updatedAt", nodeState_Info.updatedAt);
    }
    if(strcmp(nodeState_Info.lastSeenAt, "") == 0){
        cJSON_AddNullToObject(root, "lastSeenAt");    
    }else{
        cJSON_AddStringToObject(root, "lastSeenAt", nodeState_Info.lastSeenAt);
    }
    cJSON_AddNumberToObject(root, "deviceStatusBattery", nodeState_Info.deviceStatusBattery);
    cJSON_AddNumberToObject(root, "deviceStatusMargin", nodeState_Info.deviceStatusMargin);
    cJSON_AddNullToObject(root, "location");

    // 将JSON数据转换为字符串
    char *jsonStr = cJSON_Print(root);

    removeWhitespace(jsonStr);

    std::string jsonString(jsonStr);
    std::cout << "节点状态————put的节点状态信息:" << jsonString << std::endl;



    char temp[100];
    sprintf(temp, "https://127.0.0.1:8080/api/devices/%s", devEUI);

    std::string nodeState_posturl(temp);

    std::string response2 = send_httpRequest(nodeState_posturl, Method_put, jsonString);

    cjsonParse_postNodeState(response2.c_str());

    // 释放内存
    cJSON_Delete(root);
    free(jsonStr);  //cJSON_Print函数会动态分配内存来存储生成的JSON字符串，因此在使用完毕后需要手动释放该内存，以防止内存泄漏
}












