/**
 * malloc
 * CS 241 - Fall 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


typedef struct memory_node {
    size_t mem_size;
    int open;
    struct memory_node *next;
    struct memory_node *prev;
} memory_node;

static memory_node* head = NULL;
static memory_node* tail = NULL;

memory_node* extend_heap(size_t size) {
    memory_node* new_node = sbrk(size + sizeof(memory_node));
    if(new_node == (void*)-1) {
        return NULL;
    }
    *new_node = (memory_node){size, 0, NULL, NULL};

    if(!head) {
        head = new_node;
    } else {
        head->prev = new_node;
        new_node->next = head;
        head = new_node;
    }
    if(!tail) {
        tail = head;
    }

    return new_node;
}

void replace_node(memory_node** elem, memory_node* replacement) {
    if(*elem == head) {
        head = replacement;
    }
    if ((*elem)->prev) {
        (*elem)->prev->next = replacement;
    }
}

memory_node* coalesce(memory_node* free_block){
    if(!free_block || !free_block->prev) {
        return NULL;
    }
    
    if(free_block->open && free_block->prev->open) {
        free_block->mem_size += (free_block->prev->mem_size + sizeof(memory_node));
        replace_node(&free_block->prev, free_block);
        free_block->prev = free_block->prev->prev;
        return free_block;
    } else return NULL;
}

void coalesce_around(memory_node* free_block) {
    if(!free_block || !free_block->prev || !free_block->next) {
        return;
    }

    memory_node* coalesced = NULL;
    if((coalesced = coalesce(free_block))) {
        coalesce(coalesced->next);
    }
}

memory_node* split(memory_node* free_block, size_t size){
    memory_node* new_free_block = (void*)free_block + size + sizeof(memory_node);

    if((void*)new_free_block + free_block->mem_size - (size + sizeof(memory_node)) > sbrk(0)) {
        return free_block;
    }

    *new_free_block = (memory_node){free_block->mem_size - (size + sizeof(memory_node)), 1, free_block, free_block->prev};

    memory_node* new_node = coalesce(new_free_block);
    new_free_block = (new_node) ? new_node : new_free_block;

    replace_node(&free_block, new_free_block);

    free_block->mem_size = size;
    free_block->open = 0;
    free_block->prev = new_free_block;

    return free_block;
}

memory_node* get_free_block(size_t size) {
    memory_node* traverser = head;
    int c = 0;

    while(traverser && c<10){
        if(traverser->open && traverser->mem_size >= size){
            memory_node* split_node = (traverser->mem_size < size + 2*sizeof(memory_node)) ? traverser : split(traverser,size);
            if(split_node == traverser) {
                split_node->open=0;
            }
            return split_node;
        }
        traverser = traverser->next;
        c++;
    }

    return NULL;
}

/**
 * Allocate memory block
 *
 * Allocates a block of size bytes of memory, returning a pointer to the
 * beginning of the block.  The content of the newly allocated block of
 * memory is not initialized, remaining with indeterminate values.
 *
 * @param size
 *    Size of the memory block, in bytes.
 *
 * @return
 *    On success, a pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a null pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/malloc/
 */
void *malloc(size_t size) {
    // implement malloc!
    if(size == 0) {
        return NULL;
    }
    memory_node* new_node = get_free_block(size);
    if(!new_node) {
        new_node = extend_heap(size);
    }
    return new_node+1;
}

/**
 * Allocate space for array in memory
 *
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero. The effective result is
 * the allocation of an zero-initialized memory block of (num * size) bytes.
 *
 * @param num
 *    Number of elements to be allocated.
 * @param size
 *    Size of elements.
 *
 * @return
 *    A pointer to the memory block allocated by the function.
 *
 *    The type of this pointer is always void*, which can be cast to the
 *    desired type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory, a
 *    NULL pointer is returned.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/calloc/
 */
void *calloc(size_t num, size_t size) {
    // implement calloc!
    void* space = malloc(num*size);
    if(!space) return NULL;
    return memset(space, 0, num*size);
}

/**
 * Deallocate space in memory
 *
 * A block of memory previously allocated using a call to malloc(),
 * calloc() or realloc() is deallocated, making it available again for
 * further allocations.
 *
 * Notice that this function leaves the value of ptr unchanged, hence
 * it still points to the same (now invalid) location, and not to the
 * null pointer.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(),
 *    calloc() or realloc() to be deallocated.  If a null pointer is
 *    passed as argument, no action occurs.
 */
void free(void *ptr) {
    // implement free!
    if (!ptr) {
        return;
    }
    
    memory_node* ptr_node = (memory_node*)(ptr - sizeof(memory_node));
    ptr_node->open = 1;

    coalesce_around(ptr_node);
}

/**
 * Reallocate memory block
 *
 * The size of the memory block pointed to by the ptr parameter is changed
 * to the size bytes, expanding or reducing the amount of memory available
 * in the block.
 *
 * The function may move the memory block to a new location, in which case
 * the new location is returned. The content of the memory block is preserved
 * up to the lesser of the new and old sizes, even if the block is moved. If
 * the new size is larger, the value of the newly allocated portion is
 * indeterminate.
 *
 * In case that ptr is NULL, the function behaves exactly as malloc, assigning
 * a new block of size bytes and returning a pointer to the beginning of it.
 *
 * In case that the size is 0, the memory previously allocated in ptr is
 * deallocated as if a call to free was made, and a NULL pointer is returned.
 *
 * @param ptr
 *    Pointer to a memory block previously allocated with malloc(), calloc()
 *    or realloc() to be reallocated.
 *
 *    If this is NULL, a new block is allocated and a pointer to it is
 *    returned by the function.
 *
 * @param size
 *    New size for the memory block, in bytes.
 *
 *    If it is 0 and ptr points to an existing block of memory, the memory
 *    block pointed by ptr is deallocated and a NULL pointer is returned.
 *
 * @return
 *    A pointer to the reallocated memory block, which may be either the
 *    same as the ptr argument or a new location.
 *
 *    The type of this pointer is void*, which can be cast to the desired
 *    type of data pointer in order to be dereferenceable.
 *
 *    If the function failed to allocate the requested block of memory,
 *    a NULL pointer is returned, and the memory block pointed to by
 *    argument ptr is left unchanged.
 *
 * @see http://www.cplusplus.com/reference/clibrary/cstdlib/realloc/
 */
void *realloc(void *ptr, size_t size) {
    if(!ptr) {
        return malloc(size);
    } else if(!size && ptr) {
        free(ptr);
        return NULL;
    }

    memory_node* ptr_node = (memory_node*)(ptr-sizeof(memory_node));
    void* new_loc = ptr;

    if(size > ptr_node->mem_size) {
        new_loc = malloc(size);
    }
    
    if(new_loc != ptr) {
        memcpy(new_loc, ptr, (size < ptr_node->mem_size) ? size : ptr_node->mem_size);
        free(ptr);
    }
    return new_loc;
}