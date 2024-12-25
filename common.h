#pragma once

#define va_list __builtin_va_list /* buildintはclangが用意してくれてる */
#define va_start __builtin_va_start
#define va_end __builtin_va_end
#define va_arg __builtin_va_arg

void printf(const char *fmt, ...);
