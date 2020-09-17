#ifndef STDARG_H
#define STDARG_H

/* Since building with GCC, use built-in stdarg */

typedef __builtin_va_list va_list;

#define va_start(a, l) __builtin_va_start(a, l)
#define va_end(a) __builtin_va_end(a)
#define va_arg(a, t) __builtin_va_arg (a, t)

#endif
