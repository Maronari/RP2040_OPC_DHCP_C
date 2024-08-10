#include "myMalloc.h"
#include "string.h"

void *pvPortRealloc(void *mem, size_t newsize)
{
    if (newsize == 0) {
        //vPortFree(mem);
        return NULL;
    }

    if (mem == NULL) {
        return pvPortCalloc(1, newsize);
    }

    void *p;
    p = pvPortMalloc(newsize);
    if (p) {
        /* zero the memory */
        if (mem != NULL) {
            memcpy(p, mem, newsize);
            vPortFree(mem);
        }
    }
    return p;
}

void *pvPortCalloc(size_t count, size_t size)
{
    void *p;

    /* allocate 'count' objects of size 'size' */
    p = pvPortMalloc(count * size);
    if (p) {
        /* zero the memory */
        memset(p, 0, count * size);
    }
    return p;
}