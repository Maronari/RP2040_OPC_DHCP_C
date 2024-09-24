#include "myMalloc.h"
#include <task.h>
#include "string.h"
#include <stdio.h>

void *pvPortRealloc(void *mem, size_t size)
{
    if (size == 0)
    {
        //vPortFree(mem);
        return NULL;
    }

    if (mem == NULL)
    {
        return pvPortCalloc(1, size);
    }

    void *rc;
    /* allocate 'count' objects of size 'size' */
    {
        rc = pvPortMalloc(size);
        if (rc == NULL)
        {
            printf("malloc return NULL!\n");
        }
    }
    vTaskSuspendAll();
    if (rc)
    {
        /* zero the memory */
        if (mem != NULL)
        {
            memcpy(rc, mem, size);
            vPortFree(mem);
        }
    }
#if PICO_DEBUG_MALLOC
    if (!rc || ((uint8_t *)rc) + size > (uint8_t*)PICO_DEBUG_MALLOC_LOW_WATER) {
        printf("realloc %p %d->%p\n", mem, (unsigned int) size, rc);
    }
#endif
    xTaskResumeAll();
    return rc;
}

void *pvPortCalloc(size_t count, size_t size)
{
    void *rc;

    /* allocate 'count' objects of size 'size' */
    {
        void *rc = pvPortMalloc(count * size);
        if (rc == NULL)
        {
            printf("malloc return NULL!\n");
        }
    }
    vTaskSuspendAll();
    if (rc) {
        /* zero the memory */
        memset(rc, 0, count * size);
    }
#if PICO_DEBUG_MALLOC
    if (!rc || ((uint8_t *)rc) + size > (uint8_t*)PICO_DEBUG_MALLOC_LOW_WATER) {
        printf("calloc %d %p->%p\n", (unsigned int) (count * size), rc, ((uint8_t *) rc) + size);
    }
#endif
    xTaskResumeAll();
    return rc;
}