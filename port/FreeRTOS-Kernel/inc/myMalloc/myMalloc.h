#ifndef MY_MALLOC_H

#include <FreeRTOS.h>

void *pvPortRealloc(void *mem, size_t newsize);
void *pvPortCalloc(size_t count, size_t size);

#endif //MY_MALLOC_H