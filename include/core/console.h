#ifndef CORE_CONSOLE_H
#define CORE_CONSOLE_H

#include <stdbool.h>

typedef void (*console_command_fn)(int argc, const char** argv);

void console_init(void);
bool console_register_command(const char* name, const char* help, console_command_fn fn);
void console_execute_line(const char* line);
void console_print_prompt(void);
void console_handle_char(char c);
void console_poll(void);
void console_run_scripted_boot_demo(void);

#endif
