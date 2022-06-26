#ifndef _LIST_H
#define _LIST_H

#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstring>
#define DOUBLE 0
#define ADDRESS 1

struct MallocMetadata;

struct MallocTip
{
    MallocMetadata *front;
    MallocTip(MallocMetadata *front) : front(front){};
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
        MallocTip *tip = (MallocTip *)(this + this->size - sizeof(MallocTip));
        *tip = MallocTip(this);
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

#endif
