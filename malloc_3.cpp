#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstring>
#include <list>
#define SPLIT_SIZE 128
using std::list;

const long max_size = std::pow(10, 8);

struct MallocMetadata
{
    size_t size;
    bool is_free;
    // MallocMetadata *next;
    // MallocMetadata *prev;
    // need to preserve the 8-multiple attribute somehow
    MallocMetadata(size_t _size) : size(_size), is_free(true) {};
};

const size_t meta_size = sizeof(MallocMetadata);
list<MallocMetadata> free_list;
list<MallocMetadata> mmap_list;
MallocMetadata *wilderness= &MallocMetadata(0); //may be free and may not. Thus - not in free_list!
size_t free_blocks = 0;
size_t free_bytes = 0;
size_t allocated_blocks = 0; //total num of blocks
size_t allocated_bytes = 0;
size_t meta_data_bytes = 0;

/**
 * @brief compare function for the free list or the mmap list
 *
 * @param first first block
 * @param second second block
 * @return true if first block is before/same as second in the free_list
 * @return false otherwise
 */
bool compare(const MallocMetadata &first, const MallocMetadata &second)
{
    if (first.size < second.size)
    {
        return true;
    }
    if (first.size > second.size)
    {
        return false;
    }
    //  sizes are equal, we compare addresses:
    if(&first < &second)
        return true;

    return false;
}

/**
 * @brief compare function for the list of *all blocks*
 *
 * @param first first block
 * @param second second block
 * @return true if first block is before/same as second in the free_list
 * @return false otherwise
 */
bool compare_address(const MallocMetadata &first, const MallocMetadata &second)
{
    // we compare addresses:
    if(&first < &second)
        return true;

    return false;
}

/**
 * @brief padds size to be multiple of 8, including the metaData size
 * 
 * @param size the size to padd
 * @return size_t whole & divides by 8
 */
size_t padd_size(size_t size) 
{
    // some mathmatical calc
    int full_size = size + meta_size;
    int mod8 = full_size % 8;
    if(mod8 == 0) {
        return full_size;
    }
    return (full_size/8 + 1) * 8;
}

bool isSplitable(const size_t remainder) 
{
    return remainder >= SPLIT_SIZE + meta_size;
}

void *smalloc(size_t size)
{
    if (size == 0)
    {
        return nullptr;
    }
    if(size > max_size) 
    {
        // need to mmap this fucker
    }
    // metaData size + 8-multiple padding  
    size = padd_size(size);

    // look for free block in free_list
    for (auto it_free_list = free_list.begin(); it_free_list != free_list.end(); it_free_list++)
    {
        if(it_free_list->size >= size) // there is enough space here
        {
            int remaining = it_free_list->size - size;
            auto block = it_free_list;
            void *address = &(*block);
            if(isSplitable(remaining))  // to split or not to split?
            {
                block->size = size;
                MallocMetadata *curr_meta = &(*block);
                free_list.erase(block); 
                MallocMetadata *new_block = (MallocMetadata *)(curr_meta + size);
                *new_block = MallocMetadata(remaining);
                free_list.push_front(*new_block);
                // stats:
                allocated_blocks++;
                meta_data_bytes += meta_size;
            }
            else {
                block->size = size;
                free_list.erase(block);
                // stats:
                free_blocks--;
            }
            free_bytes -= (size - meta_size); 
            free_list.sort(compare);
            return address + meta_size;
        }
    }
    // reaching here means we couldn't find a large enough spot in the free_list
    // let's try wilderness instead:
    if(wilderness->is_free)
    {
        if(wilderness->size >= size)
        {
            if(isSplitable(wilderness->size - size))
            {
                MallocMetadata *old_wilderness = wilderness;
                old_wilderness->size = size;
                // should assign new wilderness and keep it free
                MallocMetadata *new_wilderness = (MallocMetadata *)(wilderness + size);
                *new_wilderness = MallocMetadata(wilderness->size - size);
                new_wilderness->is_free = true;
                wilderness = new_wilderness;
                // stats:
                free_bytes -= (size - meta_size);
                allocated_blocks++;
                allocated_bytes += (wilderness->size - size);
                meta_data_bytes += meta_size;
                return old_wilderness + meta_size;
            }
            else {
                wilderness->is_free = false;
                // stats:
                free_blocks--;
                free_bytes -= (wilderness->size - meta_size);
            }
        }
        else 
        {
            // need to sbrk()
            void *ptr = sbrk(size - wilderness->size);
            if (ptr == (void *)(-1))
            {
                return nullptr;
            }
            wilderness->size = size;
            wilderness->is_free = false;
            // stats:
            free_blocks--;
            free_bytes -= (size - meta_size);
            allocated_bytes += (size - wilderness->size);
            return wilderness + meta_size;
        }
    }
    else //need to assign new wilderness and use it
    {
        void *ptr = sbrk(size);
        if (ptr == (void *)(-1))
        {
            return nullptr;
        }
        MallocMetadata *new_wilderness = (MallocMetadata *)(wilderness + wilderness->size);
        *new_wilderness = MallocMetadata(size);
        new_wilderness->is_free = false;
        wilderness = new_wilderness;
        // stats:
        allocated_blocks++;
        allocated_bytes += (size - meta_size);
        meta_data_bytes += meta_size;
    }

    return wilderness + meta_size;
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

MallocMetadata *_merge(MallocMetadata *previous, MallocMetadata *next)
{
    return previous; //for now
}

MallocMetadata *_findClosestNext(MallocMetadata *block)
{

    return block; //for now
}

MallocMetadata *_findClosestPrevious(MallocMetadata *block)
{
    return block; //for now
}

MallocMetadata *_previousToWilderness()
{
    return wilderness; //for now
}

void sfree(void *p)
{
    if (!p)
        return;
    MallocMetadata *meta = ((MallocMetadata *)(p - meta_size));
    if (meta->is_free)
        return;
    
    // check and handle if mmapped

    // check and handle if wilderness is meta
    if(wilderness == meta)
    {
        MallocMetadata *prev = _previousToWilderness();
        if(prev + prev->size == wilderness) //adjacent from the back
        {
            prev->size += wilderness->size - meta_size;
            wilderness = prev;
            free_list.remove(*prev);
            free_bytes += wilderness->size;
            allocated_blocks--;
            meta_data_bytes -= meta_size;
        }
        else 
        {
            wilderness->is_free = true;
        }
        return;
    }
    // if we get here we are not freeing wilderness
    meta->is_free = true;
    MallocMetadata *previous = _findClosestPrevious(meta);
    MallocMetadata *next = _findClosestNext(meta);
    if(previous + previous->size  == meta) //adjacent from the back
    {
        free_list.remove(*meta);
        free_list.remove(*previous);
        meta = _merge(previous, meta);
        if(meta + meta->size == next) //also from the front
        {
            free_list.remove(*next);
            meta = _merge(meta, next);
        }
    }
    else {
        if(meta + meta->size == next) //adjacent from the front
        {
            free_list.remove(*next);
            meta = _merge(meta, next);
        }
    }

    free_list.push_back(*meta);
    free_list.sort(compare);
    // stats:
    free_blocks++;
    free_bytes += (meta->size - meta_size);

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
