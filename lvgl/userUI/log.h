#ifndef LOG_H
#define LOG_H

#include <string>

class Logger
{
public:
    static void logToFile(const std::string& message);
};

#endif

