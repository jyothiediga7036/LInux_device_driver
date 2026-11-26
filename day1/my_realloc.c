#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

static uint8_t *heap_start;
static uint8_t *heap_current;
static uint8_t *heap_end;

void heap_init(void *mem, size_t size)
{
    heap_start = (uint8_t *)mem;
    heap_current = heap_start;
    heap_end = heap_start + size;
}

void *my_malloc(size_t size)
{
    if (heap_current + size > heap_end)
        return NULL;

    void *ptr = heap_current;
    heap_current += size;

    return ptr;
}

void my_free(void *ptr)
{
    (void)ptr;
}


void *my_realloc(void *ptr, size_t new_size)
{
    //realloc(NULL,size) behaves like malloc() 
    if (ptr == NULL)
    {
        return my_malloc(new_size);
    }

    //realloc(ptr,0) behaves like free() 
    if (new_size == 0)
    {
        my_free(ptr);
        return NULL;
    }

    //Get block header 
    block_t *block = (block_t *)ptr - 1;

    //Existing block is already large enough
    if (block->size >= new_size)
    {
        return ptr;
    }

    // Allocate a new block 
    void *new_ptr = my_malloc(new_size);

    if (new_ptr == NULL)
    {
        return NULL;
    }

    //Copy old data 
    my_memcpy(new_ptr, ptr, block->size);

  //  Free old block
    my_free(ptr);

    return new_ptr;
}

int main()
{
    char heap[1024];

    heap_init(heap, sizeof(heap));

    int *a = my_malloc(sizeof(int));
    int *b = my_malloc(sizeof(int));

    if (a && b)
    {
        *a = 100;
        *b = 200;

        printf("%d %d\n", *a, *b);
    }

    return 0;
}


