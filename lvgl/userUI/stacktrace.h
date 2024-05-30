#ifndef STACKTRACE_H
#define STACKTRACE_H

void setup_signal_handlers();
void signal_handler(int sig);
void print_stack_trace_to_file(const char *filename);

#endif // STACKTRACE_H

