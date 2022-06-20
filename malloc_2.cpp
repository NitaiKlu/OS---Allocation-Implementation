#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstring>

const long max_size = std::pow(10, 8);

struct MallocMetadata
{
    size_t size;
    bool is_free;
    MallocMetadata *next;
    MallocMetadata *prev;
    MallocMetadata(size_t _size, MallocMetadata *_prev) : size(_size), is_free(false), next(nullptr), prev(_prev){};
};

const size_t meta_size = sizeof(struct MallocMetadata);

MallocMetadata *first_meta = &MallocMetadata(0, nullptr);
size_t free_blocks = 0;
size_t free_bytes = 0;
size_t allocated_blocks = 0;
size_t allocated_bytes = 0;

size_t meta_data_bytes = 0;

void *smalloc(size_t size)
{
    if (size == 0 || size > max_size)
    {
        return nullptr;
    }

    MallocMetadata *curr_meta = first_meta;
    MallocMetadata *next_meta = curr_meta->next;
    while (next_meta && (!next_meta->is_free || next_meta->size < size))
    {
        curr_meta = next_meta;
        next_meta = next_meta->next;
    }
    if (!next_meta)
    {
        // We need to sbrk
        size_t block_size = size + meta_size;
        void *ptr = sbrk(block_size);
        if (ptr == (void *)(-1))
        {
            return nullptr;
        }
        curr_meta->next = (MallocMetadata *)ptr;
        *curr_meta->next = MallocMetadata(size, curr_meta);

        allocated_blocks++;
        allocated_bytes += size;

        return ptr + meta_size;
    }
    // We can re-use
    curr_meta->size = size;

    free_blocks--;
    free_bytes -= curr_meta->size;

    return curr_meta + meta_size;
}

void *scalloc(size_t num, size_t size)
{
    size_t total_size = num * size;
    void *ptr = smalloc(total_size);
    if (!ptr)
        return ptr;
    std::memset(ptr, 0, total_size);
    return ptr;
}

void sfree(void *p)
{
    if (!p)
        return;
    MallocMetadata *meta = ((MallocMetadata *)(p - meta_size));
    if (meta->is_free)
        return;
    meta->is_free = true;

    free_blocks++;
    free_bytes += meta->size;
}

void *srealloc(void *oldp, size_t size)
{
    if (!oldp)
        return smalloc(size);
    MallocMetadata *old_meta = ((MallocMetadata *)(oldp - meta_size));
    if (old_meta->size <= size)
    {
        return oldp;
    }
    void *ptr = smalloc(size);
    if (!ptr)
        return ptr;
    std::memmove(ptr, oldp, old_meta->size);
    sfree(oldp);
}

size_t _num_free_blocks()
{
    return free_blocks;
}
size_t _num_free_bytes()
{
    return free_bytes;
}
size_t _num_allocated_blocks()
{
    return allocated_blocks;
}
size_t _num_allocated_bytes()
{
    return allocated_bytes;
}
size_t _size_meta_data()
{
    return meta_size;
}
size_t _num_meta_data_bytes()
{
    return _size_meta_data() * _num_allocated_blocks();
}
