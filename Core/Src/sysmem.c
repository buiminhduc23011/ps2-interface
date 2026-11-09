/* sysmem.c — quản lý heap _sbrk cho newlib-nano */
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

extern char _end;
extern char _estack;

static char *__sbrk_heap_end = 0;

void *_sbrk(ptrdiff_t incr)
{
  char *prev_heap_end;
  if (__sbrk_heap_end == 0) __sbrk_heap_end = &_end;
  prev_heap_end = __sbrk_heap_end;
  uintptr_t next = (uintptr_t)__sbrk_heap_end + (uintptr_t)incr;
  /* giữ 1KB cho stack chính */
  if (next > (uintptr_t)&_estack - 1024U) {
    errno = ENOMEM;
    return (void *)-1;
  }
  __sbrk_heap_end = (char *)next;
  return (void *)prev_heap_end;
}
