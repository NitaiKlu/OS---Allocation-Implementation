#include "list.h"

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
