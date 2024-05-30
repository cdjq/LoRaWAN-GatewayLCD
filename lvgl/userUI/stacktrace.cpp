#include "stacktrace.h"
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

void signal_handler(int sig) {
    std::cout << "stacktrace.cpp——>9——>程序中发生段错误，进入/var/log/lcd_stacktrace.txt查看栈调用" << std::endl;
    const char *signal_name = sig == SIGSEGV ? "SIGSEGV" : "Unknown";
    fprintf(stderr, "Error: signal %s:\n", signal_name);
    
    const char *filename = "/var/log/lcd_stacktrace.txt";
    // 打印调用栈到stderr
    print_stack_trace_to_file(filename);
    
    // 终止程序
    exit(1);
}

void setup_signal_handlers() {
    signal(SIGSEGV, signal_handler); // 捕获段错误信号
}

void print_stack_trace_to_file(const char *filename) {
    void *array[36];
    size_t size;
    char **strings;
    size_t i;

    // 获取调用栈
    size = backtrace(array, 36);
    strings = backtrace_symbols(array, size);

    FILE *out = filename ? fopen(filename, "a") : stderr;
    if (out == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    fprintf(out, "-------------------------------------------------------------\n");
    // 打印调用栈
    for (i = 0; i < size; i++) {
        fprintf(out, "%s\n", strings[i]);
    }

    free(strings); // 释放backtrace_symbols分配的内存
    if (out != stderr) {
        fclose(out); // 关闭文件
    }
}


