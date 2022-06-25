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

const size_t meta_size = sizeof(MallocMetadata);

class MetaDataList
{
private:
    MallocMetadata *head, *tail;
    void setHead(MallocMetadata *new_head);
    void setTail(MallocMetadata *new_tail);
    MallocMetadata *getLast();

public:
    MetaDataList() = default;
    ~MetaDataList() = default;
    MallocMetadata *begin();
    MallocMetadata *end();
    MallocMetadata *findFree(int needed_size);
    void pushLast(MallocMetadata *to_add);
};
void MetaDataList::setHead(MallocMetadata *new_head)
{
    this->head = new_head;
}
void MetaDataList::setTail(MallocMetadata *new_tail)
{
    this->tail = new_tail;
}
MallocMetadata *MetaDataList::begin()
{
    return this->head;
}
MallocMetadata *MetaDataList::end()
{
    return nullptr;
}
MallocMetadata *MetaDataList::getLast()
{
    return this->tail;
}
MallocMetadata *MetaDataList::findFree(int needed_size)
{
    for (auto it = begin(); it != end(); it = it->next)
    {
        if (it->is_free && it->size >= needed_size)
        {
            return it;
        }
    }
    return nullptr;
}
void MetaDataList::pushLast(MallocMetadata *to_add)
{
    if (!to_add)
        return;
    MallocMetadata *curr_tail = getLast();
    if (curr_tail)
    {
        curr_tail->next = to_add;
    }
    else
    {
        // If there is no tail the list is empty
        setHead(to_add);
    }
    to_add->prev = curr_tail; // If the list is empty prev will be nullptr
    to_add->next = nullptr;
    setTail(to_add);
}

MetaDataList meta_list = MetaDataList();
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

    MallocMetadata *meta_ptr = meta_list.findFree(size);
    if (!meta_ptr)
    {
        // We need to sbrk
        size_t block_size = size + meta_size;
        void *ptr = sbrk(block_size);
        if (ptr == (void *)(-1))
        {
            return nullptr;
        }
        meta_ptr = ((MallocMetadata *)ptr);
        meta_ptr->is_free = 0;
        meta_ptr->size = size;
        meta_list.pushLast(meta_ptr);

        allocated_blocks++;
        allocated_bytes += size;

        return ptr + meta_size;
    }
    else
    {
        meta_ptr->is_free = 0;
        free_blocks--;
        free_bytes -= meta_ptr->size;

        return (void *)meta_ptr + meta_size;
    }
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
    if (old_meta->size >= size)
    {
        return oldp;
    }
    void *ptr = smalloc(size);
    if (!ptr)
        return ptr;
    std::memmove(ptr, oldp, old_meta->size);
    sfree(oldp);
    return ptr;
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
