#include "log.h"
#include <iostream>
#include <fstream>
#include <chrono>

void Logger::logToFile(const std::string& message)
{
    // 获取当前时间
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // 将时间转换为字符串
    char timestamp[20];
    std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", std::localtime(&now_c));

    // 打开日志文件
    std::ofstream logFile("/var/log/lcdLog.txt", std::ios::app);

    // 写入日志信息
    if (logFile.is_open())
    {
        logFile << "[" << timestamp << "] " << message << std::endl;
        logFile.close();
    }
    else
    {
        std::cout << "无法打开日志文件" << std::endl;
    }
}

