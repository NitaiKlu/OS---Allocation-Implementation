#include <iostream>
#include <unistd.h>
#include <cmath>

const long max_size = std::pow(10, 8);

void *smalloc(size_t size)
{
    if (size == 0 || size > max_size)
    {
        return nullptr;
    }
    void *ptr = sbrk(size);
    if (ptr == (void *)(-1))
    {
        return nullptr;
    }
    return ptr;
}