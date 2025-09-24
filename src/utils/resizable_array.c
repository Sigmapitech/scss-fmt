#include <stdbool.h>
#include <stddef.h>

#include "debug.h"
#include "resizable_array.h"


static
bool process_initial_alloc(resizable_array_t *arr, size_t objsize)
{
    arr->nmemb = 0;
    arr->capacity = INITIAL_SIZE;
    arr->buff = malloc(INITIAL_SIZE * objsize);
    return arr->buff != NULL;
}

bool sized_struct_ensure_capacity(
    resizable_array_t *arr, size_t requested, size_t objsize
)
{
    size_t endsize = INITIAL_SIZE;
    void *newp;

    if (arr->capacity == 0 && requested <= INITIAL_SIZE)
        return process_initial_alloc(arr, objsize);
    if ((arr->nmemb + requested) < arr->capacity)
        return true;
    for (; endsize < arr->nmemb + requested; endsize <<= 1);
    if (endsize < arr->capacity)
        return true;
    DEBUG("Call to realloc, %p, with req=%zu, objsize=%zu",
        arr->buff, requested, objsize);
    newp = realloc(arr->buff, objsize * endsize);
    DEBUG("=> %p, with tsize=%zu", newp, objsize * endsize);
    if (newp == NULL)
        return false;
    arr->buff = newp;
    arr->capacity = endsize;
    return true;
}
