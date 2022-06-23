// heyyyyyyyy that's my list
// it's got ton a cool stuff

#include <iostream>
#include <unistd.h>
#include <cmath>
#include <cstring>
#include <list>
#define DOUBLE 0
#define ADDRESS 1

typedef struct MallocMetadata MallocMetadata;

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

struct MetaDataList
{
    int size;
    CompareBy cmp;
    MallocMetadata *head;

    MetaDataList(int compare) : size(0), cmp(compare) {};
    ~MetaDataList();
    MallocMetadata *begin();
    MallocMetadata *end();
    void erase(MallocMetadata *to_delete);
    void push(MallocMetadata *to_add);

};
