#include <system.h>
#include <stdio.h>

#if ENABLE_ASSERT

void assertion_failed(const char *expr, const char *file, u32 line)
{
  printf("%s(%u): assertion failed: %s\n", file, line, expr);
}

#endif /* ENABLE_ASSERT */
