#ifndef _ASSERT_H_
#define _ASSERT_H_

#include <configure.h>

#if !ENABLE_ASSERT
#define assert(expr)  ((void) 0)
#else
void assertion_failed (const char *expr, const char *file, unsigned line);

#define assert(expr)  ((expr) ? ((void) 0) : assertion_failed(#expr, __FILE__, __LINE__))
#endif /* !ENABLE_ASSERT */


#endif
