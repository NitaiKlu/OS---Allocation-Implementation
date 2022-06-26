#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstring>
#include <sys/mman.h>
#define SPLIT_SIZE 128
#define LARGE_MEM 128 * 1024
#define DOUBLE 0
#define ADDRESS 1

#define PAYLOAD(x) ((void *)x + offset)

struct MallocMetadata;
struct MallocTip
{
    MallocMetadata *front;
    // MallocTip(MallocMetadata *front) : front(front){};
};
struct MallocMetadata
{
    size_t size;
    bool is_free;
    MallocMetadata *next;
    MallocMetadata *prev;
    MallocMetadata(size_t _size = 0) : size(_size), is_free(true){};
    MallocTip *setTip()
    {
        MallocTip *tip = (MallocTip *)((void *)this + this->size - sizeof(MallocTip));
        // *tip = MallocTip(this);
        tip->front = this;
        return tip;
    }
};
class CompareBy
{
    int sort_type;

public:
    CompareBy(int type) : sort_type(type)
    {
        if (sort_type > 1 || sort_type < 0)
        {
            std::cout << "WRONG Compare type" << std::endl;
        }
    }
    ~CompareBy() = default;
    bool operator()(const MallocMetadata &first, const MallocMetadata &second)
    {
        if (sort_type == DOUBLE)
        {
            if (first.size < second.size)
            {
                return true;
            }
            if (first.size > second.size)
            {
                return false;
            }
        }
        //  sizes are equal/ sort_type is address, we compare addresses:
        if (&first < &second)
            return true;

        return false;
    }
};
class MetaDataList
{
private:
    int size;
    CompareBy cmp;
    MallocMetadata *head, *tail;
    void setHead(MallocMetadata *new_head);
    void setTail(MallocMetadata *new_tail);

public:
    MetaDataList(int compare = DOUBLE);
    ~MetaDataList() = default;
    MallocMetadata *begin();
    MallocMetadata *end();
    MallocMetadata *getLast();
    void erase(MallocMetadata *to_delete);
    void push(MallocMetadata *to_add);
    bool find(MallocMetadata *to_find);
    int getSize();
};
MetaDataList::MetaDataList(int compare)
    : size(0), cmp(compare), head(nullptr), tail(nullptr)
{
}
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
void MetaDataList::push(MallocMetadata *to_add)
{
    MallocMetadata *curr, *next;
    curr = begin();
    this->size++;
    // List is empty
    if (!curr)
    {
        to_add->prev = nullptr;
        to_add->next = nullptr;
        setHead(to_add);
        setTail(to_add);
        return;
    }
    // Check if new node should be the Head
    if (!cmp(*curr, *to_add))
    {
        setHead(to_add);
        to_add->prev = nullptr;
        to_add->next = curr;
        curr->prev = to_add;
        setTail(curr);
        return;
    }
    // Otherwise- look for right place
    next = curr->next;
    while (next && cmp(*next, *to_add))
    {
        curr = next;
        next = next->next;
    }
    // New node will be the last
    if (!next)
    {
        curr->next = to_add;
        to_add->next = nullptr;
        to_add->prev = curr;
        setTail(to_add);
        return;
    }
    // New node should be between curr and next
    curr->next = to_add;
    to_add->prev = curr;
    to_add->next = next;
    next->prev = to_add;
}
void MetaDataList::erase(MallocMetadata *to_delete)
{
    MallocMetadata *prev = to_delete->prev;
    MallocMetadata *next = to_delete->next;
    if (prev)
    {
        prev->next = next;
    }
    else
    {
        setHead(next);
    }
    if (next)
    {
        next->prev = prev;
    }
    else
    {
        setTail(prev);
    }
    this->size--;
}
bool MetaDataList::find(MallocMetadata *to_find)
{
    MallocMetadata *curr = begin();
    while (curr && curr != to_find && curr->size <= to_find->size)
    {
        curr = curr->next;
    }
    // Will return True if to_find is nullptr and list is empty
    return (curr == to_find);
}
int MetaDataList::getSize()
{
    return this->size;
}

const long max_size = std::pow(10, 8);

const size_t meta_size = sizeof(MallocMetadata) + sizeof(MallocTip);
const size_t offset = sizeof(MallocMetadata);
MetaDataList free_list = MetaDataList(DOUBLE);
MetaDataList mmap_list = MetaDataList(DOUBLE);
MallocMetadata *wilderness; // may be free and may not. Thus - not in free_list!
size_t free_blocks = 0;
size_t free_bytes = 0;
size_t allocated_blocks = 0; // total num of blocks
size_t allocated_bytes = 0;
size_t meta_data_bytes = 0;
void *base_addr;
bool initialized = false;

void initialize()
{
    base_addr = sbrk(0);

    void *ptr = sbrk(meta_size);
    if (ptr == (void *)(-1))
    {
        return;
    }
    wilderness = (MallocMetadata *)ptr;
    wilderness->size = meta_size;
    wilderness->is_free = true;
    wilderness->next = nullptr;
    wilderness->prev = nullptr;
    // stats:
    allocated_blocks++; // total num of blocks
    meta_data_bytes += meta_size;

    initialized = true;
}

void eraseFreeBlock(MallocMetadata *block)
{
    if (block != wilderness)
    {
        free_list.erase(block);
    }
    free_bytes -= block->size - meta_size;
    block->is_free = false;
}

void addFreeBlock(MallocMetadata *block)
{
    if (block != wilderness)
    {
        free_list.push(block);
    }
    free_bytes += (block->size - meta_size);
    block->is_free = true;
}

/**
 * @brief merges 2 adjacent blocks. assumes previous is indeed adjacent to next from the left.
 * takes care of updating the new tip. DOESN'T envolve the free_list in any way
 *
 * @param previous block
 * @param next block
 * @return MallocMetadata* of the merged block
 */
MallocMetadata *_merge(MallocMetadata *previous, MallocMetadata *next)
{
    previous->size += next->size;
    previous->setTip();
    allocated_bytes += meta_size;
    allocated_blocks--;
    meta_data_bytes -= meta_size;
    // free_bytes += meta_size; // Maybe only in mergeFree?
    return previous;
}

/**
 * @brief merge blocks to be free
 * updates only: alloc_bytes, alloc_blocks, meta_data_bytes
 * @param previous
 * @param next
 * @return MallocMetadata*
 */
MallocMetadata *_mergeFree(MallocMetadata *previous, MallocMetadata *next)
{
    previous = _merge(previous, next);
    previous->is_free = true;
    next->is_free = true;
    return previous;
}

/**
 * @brief follows the same pattern as _marge but also copies the data of the block
 * to the new merged one
 * @param previous
 * @param next
 * @param which if which == 1 then next's data is copied to previous. if which == 0, opposite.
 * @return MallocMetadata* of the merged block
 */
MallocMetadata *_mergeAndCopy(MallocMetadata *previous, MallocMetadata *next, int should_copy)
{
    int next_size = next->size;
    int previous_size = previous->size;
    previous = _merge(previous, next);
    if (should_copy)
    { // move next's data to previous
        std::memmove(PAYLOAD(previous), PAYLOAD(next), next_size - meta_size);
        free_bytes -= previous->size;
    }
    else
    {
        free_bytes -= next->size;
    }
    previous->is_free = false;
    return previous;
}

/**
 * @brief assuming the block is not in the free list (used)
 *
 * @param block
 * @param remaining
 * @param first
 * @param second
 */
MallocMetadata *_split(MallocMetadata *block, int remaining)
{
    block->size -= remaining;
    MallocMetadata *new_block = (MallocMetadata *)((void *)block + block->size);
    *new_block = MallocMetadata(remaining);
    new_block->is_free = true;

    // tip update:
    MallocTip *middle_tip = block->setTip();
    MallocTip *edge_tip = new_block->setTip();

    // stats:
    allocated_bytes -= meta_size;
    allocated_blocks++;
    meta_data_bytes += meta_size;

    return new_block;
}

/**
 * @brief finds the closest free block to "block" by memory address
 *
 * @param block to find its higher address closest neighbor
 * @return MallocMetadata* address of the neighbor if it's free, nullptr if it is used
 */
MallocMetadata *_findClosestNext(MallocMetadata *block)
{
    MallocMetadata *next_block = (MallocMetadata *)((void *)block + block->size);
    if (next_block->is_free)
        return next_block;
    return nullptr;
}

/**
 * @brief finds the closest block (from below) to "block" by memory address
 *
 * @param block to find its lower address closest neighbor
 * @return MallocMetadata* address of the neighbor if it's free, nullptr if it is used
 */
MallocMetadata *_findClosestPrevious(MallocMetadata *block)
{

    if ((void *)block <= base_addr)
    {
        return nullptr;
    }
    MallocTip *prev_tip = (MallocTip *)((void *)block - sizeof(MallocTip));
    MallocMetadata *prev_block = (MallocMetadata *)(prev_tip->front);
    if (prev_block->is_free)
        return prev_block;
    return nullptr;
}

/**
 * @brief finds the closest block (from below) to wilderness by memory address
 *
 * @return MallocMetadata* address of the neighbor if it's free, nullptr if it is used
 */
MallocMetadata *_previousToWilderness()
{
    return _findClosestPrevious(wilderness);
}

/**
 * @brief returns the best fit possible given the size we need to allocate.
 *
 * @param size size in bytes to allocate, already including metaData and padding.
 * @return MallocMetadata* of the best free block in free_block. nullptr if couldn't find
 */
MallocMetadata *_findBestFit(size_t size)
{
    for (auto it = free_list.begin(); it != free_list.end(); it = it->next)
    {
        if (it->size >= size) // there is enough room in this block
        {
            return it;
        }
    }
    return nullptr;
}

/**
 * @brief padds size to be multiple of 8, including the metaData size
 * including the tip
 * @param size the size to padd
 * @return size_t whole & divides by 8
 */
size_t padd_size(size_t size)
{
    // some mathmatical calc
    int full_size = size + meta_size;
    int mod8 = full_size % 8;
    if (mod8 == 0)
    {
        return full_size;
    }
    return (full_size / 8 + 1) * 8;
}

/**
 * @brief padds size to be multiple of 8, including the metaData size
 * **not including** the tip. Used for mmap reigons
 * @param size the size to padd
 * @return size_t whole & divides by 8
 */
size_t padd_size_no_tip(size_t size)
{
    int full_size = size + sizeof(MallocMetadata);
    int mod8 = full_size % 8;
    if (mod8 == 0)
    {
        return full_size;
    }
    return (full_size / 8 + 1) * 8;
}

bool isSplitable(const size_t remainder)
{
    return remainder >= SPLIT_SIZE + meta_size;
}

void *smalloc(size_t size)
{
    if (size == 0 || size > max_size)
    {
        return nullptr;
    }

    if (!initialized)
        initialize();

    if (size > LARGE_MEM) // mmap size
    {
        size = padd_size_no_tip(size);
        void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, 0, 0);
        if (ptr == MAP_FAILED)
        {
            return nullptr;
        }
        MallocMetadata *new_mmap = (MallocMetadata *)(ptr);
        *new_mmap = MallocMetadata(size);
        new_mmap->is_free = false;

        mmap_list.push(new_mmap);
        // should update stats??***********************
        return PAYLOAD(new_mmap);
    }

    // metaData size + 8-multiple padding
    size = padd_size(size);

    // look for best free block in free_list
    MallocMetadata *block = _findBestFit(size);

    if (block != nullptr) // we found a block
    {
        eraseFreeBlock(block);
        int remaining = block->size - size;
        if (isSplitable(remaining)) // to split or not to split?
        {
            MallocMetadata *new_block = _split(block, remaining);
            // list update:
            addFreeBlock(new_block);
        }
        return PAYLOAD(block);
    }
    // reaching here means we couldn't find a large enough spot in the free_list
    // let's try wilderness instead:
    if (wilderness->is_free)
    {
        if (wilderness->size >= size)
        {
            if (isSplitable(wilderness->size - size))
            {
                eraseFreeBlock(wilderness);
                MallocMetadata *old_wilderness = wilderness;
                wilderness = _split(wilderness, wilderness->size - size);
                addFreeBlock(wilderness);
                return PAYLOAD(old_wilderness);
            }
            else
            {
                // no splitting - no need to update the Tips or size
                wilderness->is_free = false;

                // stats:
                free_bytes -= (wilderness->size - meta_size);
                return PAYLOAD(wilderness);
            }
        }
        else
        {
            // need to sbrk()
            int addition = size - wilderness->size;
            void *ptr = sbrk(addition);
            if (ptr == (void *)(-1))
            {
                return nullptr;
            }
            // bool was_empty = wilderness->size == meta_size;
            wilderness->size = size;
            wilderness->is_free = false;

            // tip update:
            wilderness->setTip();

            // stats:
            free_bytes -= (wilderness->size - addition - meta_size);
            allocated_bytes += addition;
            return PAYLOAD(wilderness);
        }
    }
    else // need to assign new wilderness and use it
    {
        void *ptr = sbrk(size);
        if (ptr == (void *)(-1))
        {
            return nullptr;
        }
        MallocMetadata *new_wilderness = (MallocMetadata *)((void *)wilderness + wilderness->size);
        *new_wilderness = MallocMetadata(size);
        new_wilderness->is_free = false;

        // create new tip:
        new_wilderness->setTip();

        // update wilderness
        wilderness = new_wilderness;

        // stats:
        allocated_blocks++;
        allocated_bytes += (size - meta_size);
        meta_data_bytes += meta_size;
    }
    wilderness->is_free = false;
    return PAYLOAD(wilderness);
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

    if (!initialized)
        initialize();

    MallocMetadata *meta = ((MallocMetadata *)(p - offset));
    if (meta->is_free)
        return;

    // check and handle if mmapped
    if (meta->size > LARGE_MEM)
    {
        if (mmap_list.find(meta) == false)
            return;
        mmap_list.erase(meta);
        int err = munmap(meta, meta->size);
        if (err != 0)
        {
            perror("unmapping failed.\n");
            return;
        }
    }

    // check and handle if wilderness is meta
    if (wilderness == meta)
    {
        MallocMetadata *prev = _previousToWilderness();
        if (prev != nullptr) // adjacent from the back
        {
            // wilderness = _merge(prev, wilderness);
            // free_list.erase(prev);
            // allocated_blocks--;
            // meta_data_bytes -= meta_size;
            // free_bytes += wilderness->size;
            eraseFreeBlock(prev);
            wilderness = _mergeFree(prev, wilderness);
        }
        addFreeBlock(wilderness);
        return;
    }
    // if we get here we are not freeing wilderness
    meta->is_free = true;
    MallocMetadata *previous = _findClosestPrevious(meta);
    MallocMetadata *next = _findClosestNext(meta);
    if (previous != nullptr) // adjacent from the back
    {
        // free_list.erase(previous);
        // meta = _merge(previous, meta);
        // free_bytes -= previous->size - meta_size;
        // allocated_bytes += meta_size;
        // allocated_blocks--;
        // meta_data_bytes -= meta_size;
        eraseFreeBlock(previous);
        meta = _mergeFree(previous, meta);
        if (next != nullptr) // also from the front
        {
            // free_list.erase(next);
            // meta = _merge(meta, next);
            // meta_data_bytes -= meta_size;
            // free_bytes -= next->size - meta_size;
            // allocated_bytes += meta_size;
            // allocated_blocks--;
            eraseFreeBlock(next);
            meta = _mergeFree(meta, next);
            if (next == wilderness)
            {
                wilderness = meta;
            }
        }
    }
    else
    {
        if (next != nullptr) // adjacent from the front
        {
            // free_list.erase(next);
            // meta = _merge(meta, next);
            // meta_data_bytes -= meta_size;
            // allocated_blocks--;
            eraseFreeBlock(next);
            meta = _mergeFree(meta, next);
        }
    }
    addFreeBlock(meta);
}

void *srealloc(void *oldp, size_t size)
{
    if (size == 0 || size > max_size)
    {
        return nullptr;
    }

    if (!initialized)
        initialize();

    MallocMetadata *meta = (MallocMetadata *)(oldp - offset);
    if (meta->size > LARGE_MEM) // mmap allocation
    {
        size = padd_size_no_tip(size);
        if (size <= meta->size) // check if this is the right way or need to check only if size == meta.size
            return PAYLOAD(meta);

        // allocating new mmap:
        void *ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, 0, 0);
        if (ptr == MAP_FAILED)
        {
            return nullptr;
        }
        MallocMetadata *new_mmap = (MallocMetadata *)(ptr);
        *new_mmap = MallocMetadata(size);
        new_mmap->is_free = false;
        mmap_list.push(new_mmap);

        // moving data:
        std::memmove(PAYLOAD(new_mmap), PAYLOAD(meta), meta->size - offset);

        // deleting current
        mmap_list.erase(meta);
        int err = munmap(meta, meta->size);
        if (err != 0)
        {
            perror("unmapping failed.\n");
            return nullptr;
        }
        return PAYLOAD(new_mmap);
    }
    else // sbrk allocation
    {
        size = padd_size(size);
        if (size <= meta->size)
            return PAYLOAD(meta);
        if (meta == wilderness)
        {
            MallocMetadata *prev = _previousToWilderness();
            if (prev != nullptr)
            { // previous is free. merging:

                eraseFreeBlock(prev);

                wilderness = _mergeAndCopy(prev, wilderness, 1);

                if (wilderness->size >= size)
                { // then previous was enough
                    // need to check if spiltable**************************************
                    int remaining = wilderness->size - size;
                    if (isSplitable(remaining))
                    { // split what remains
                        MallocMetadata *old_wilderness = wilderness;
                        wilderness = _split(meta, remaining);
                        return (PAYLOAD(old_wilderness));
                    }
                    free_bytes -= (wilderness->size - meta_size);
                    return PAYLOAD(wilderness);
                }
                // enlarge wilderness:
                int addition = size - wilderness->size;
                void *ptr = sbrk(addition);
                if (ptr == (void *)(-1))
                {
                    return nullptr;
                }
                allocated_bytes += addition;
                wilderness->size += addition;

                return PAYLOAD(wilderness);
            }

            // enlarge wilderness:
            int addition = size - wilderness->size;
            void *ptr = sbrk(addition);
            if (ptr == (void *)(-1))
            {
                return nullptr;
            }
            allocated_bytes += addition;
            wilderness->size += addition;

            return PAYLOAD(wilderness);
        }

        MallocMetadata *prev = _findClosestPrevious(meta);
        MallocMetadata *next = _findClosestNext(meta);
        if (prev != nullptr)
        { // previous is free
            // try to merge:
            if (prev->size + meta->size >= size)
            { // then previous is enough
                // update list:
                eraseFreeBlock(prev);

                meta = _mergeAndCopy(prev, meta, 1);
                int remaining = meta->size - size;

                if (isSplitable(remaining))
                { // split what remains
                    MallocMetadata *new_block = _split(meta, remaining);
                    sfree((PAYLOAD(new_block)));
                }

                return PAYLOAD(meta);
            }
        }
        if (next != nullptr)
        { // next is free
            // try to merge:
            if (next->size + meta->size >= size)
            { // then next is enough
                eraseFreeBlock(next);
                if (next != wilderness)
                {
                    meta = _mergeAndCopy(meta, next, 0);
                    int remaining = meta->size - size;

                    if (isSplitable(remaining))
                    { // split what remains
                        MallocMetadata *new_block = _split(meta, remaining);
                        sfree((PAYLOAD(new_block)));
                    }
                    return PAYLOAD(meta);
                }
                else
                {
                    // next is wilderness
                    wilderness = _mergeAndCopy(meta, next, 0);
                    int remaining = wilderness->size - size;

                    if (isSplitable(remaining))
                    { // split what remains
                        MallocMetadata *old_wilderness = wilderness;
                        wilderness = _split(meta, remaining);
                        return (PAYLOAD(old_wilderness));
                    }
                    return (PAYLOAD(wilderness));
                }
            }
        }
        // reaching here- next and previous were not enough or not adjacent!
        if (next != nullptr && prev != nullptr)
        { // both adjacent
            if (next->size + meta->size + prev->size >= size)
            { // then merge all 3
                if (next != wilderness)
                {
                    // update list:
                    eraseFreeBlock(prev);
                    eraseFreeBlock(next);
                    // merge:
                    meta = _mergeAndCopy(prev, meta, 1);
                    meta = _mergeAndCopy(meta, next, 0);

                    int remaining = meta->size - size;
                    if (isSplitable(remaining))
                    { // split what remains
                        MallocMetadata *new_block = _split(meta, remaining);
                        sfree((PAYLOAD(new_block)));
                    }
                    return PAYLOAD(meta);
                }
                else
                {
                    eraseFreeBlock(prev);
                    eraseFreeBlock(next);
                    meta = _mergeAndCopy(prev, meta, 1);
                    meta = _mergeAndCopy(meta, next, 0);
                    wilderness = meta;
                    int remaining = meta->size - size;
                    if (isSplitable(remaining))
                    { // split what remains
                        wilderness = _split(meta, remaining);
                    }
                    return PAYLOAD(meta);
                }
            }

            if (next == wilderness)
            {
                // all 3 weren't enough. we'll connect all 3 and enlrge wilderness:

                // update list:
                eraseFreeBlock(next);
                eraseFreeBlock(prev);
                meta = _mergeAndCopy(prev, meta, 1);
                meta = _mergeAndCopy(meta, next, 0);
                wilderness = meta;

                int addition = size - meta->size;
                void *ptr = sbrk(addition);
                if (ptr == (void *)(-1))
                {
                    return nullptr;
                }
                allocated_bytes += addition;

                wilderness->size += addition;
                wilderness->is_free = false;
                return PAYLOAD(wilderness);
            }
        }

        if (next != nullptr && next == wilderness)
        {
            eraseFreeBlock(next);
            // merge:
            meta = _mergeAndCopy(meta, next, 0);
            wilderness = meta;
            int addition = size - wilderness->size;
            void *ptr = sbrk(addition);
            if (ptr == (void *)(-1))
            {
                return nullptr;
            }
            allocated_bytes += addition;

            wilderness->size += addition;
            return PAYLOAD(wilderness);
        }

        // even all merging 3 adjacent didn't work :(
        // finding a block in the free_list or allocating new if necessary:
        MallocMetadata *sweet_spot = (MallocMetadata *)smalloc(size);
        if (meta != nullptr)
        {
            sfree(PAYLOAD(meta));
            return PAYLOAD(sweet_spot);
        }
        perror("no spot was found! problems with smalloc?");
        return nullptr;
    }
}

size_t _num_free_blocks()
{
    return free_list.getSize() + wilderness->is_free;
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
