#include "list.h"

using std::cout;
using std::endl;

int main()
{
    int array_size = 15;
    MallocMetadata meta_array[array_size];
    cout << endl << "[1] Fixed size test" << endl;
    MetaDataList list1;
    for (int i = 0; i < array_size; i++)
    {
        meta_array[i].size = 50;
        list1.push(&(meta_array[i]));
    }
    for (auto it = list1.begin(); it != list1.end(); it = it->next)
    {
        cout << "Addr: " << it << ", Size: " << it->size << endl;
    }

    cout << endl << "[2] Size linear with addres test" << endl;
    MetaDataList list2;
    for (int i = 0; i < array_size; i++)
    {
        meta_array[i].size = 50 * i;
        list2.push(&(meta_array[i]));
    }
    for (auto it = list2.begin(); it != list2.end(); it = it->next)
    {
        cout << "Addr: " << it << ", Size: " << it->size << endl;
    }
    
    cout << endl << "[3] Size anti-linear with addres test" << endl;
    MetaDataList list3;
    for (int i = 0; i < array_size; i++)
    {
        meta_array[i].size = 50 * (array_size-i);
        list3.push(&(meta_array[i]));
    }
    for (auto it = list3.begin(); it != list3.end(); it = it->next)
    {
        cout << "Addr: " << it << ", Size: " << it->size << endl;
    }

    cout << endl << "[4] Delete test (on list 3)" << endl;
    for (int i = 0; i < array_size; i++)
    {
        if (i % 3 == 0) list3.erase(&(meta_array[i]));
    }
    for (auto it = list3.begin(); it != list3.end(); it = it->next)
    {
        cout << "Addr: " << it << ", Size: " << it->size << endl;
    }

    cout << endl << "[5] Re-add test (on list 3)" << endl;
    int extra_array_size = 0.5 * array_size;
    MallocMetadata extra_meta_array[extra_array_size];
    for (int i = 0; i < extra_array_size; i++)
    {
        extra_meta_array[i].size = (i % 2) ? 111 * i : 101 * i;
        list3.push(&(extra_meta_array[i]));
    }
    for (auto it = list3.begin(); it != list3.end(); it = it->next)
    {
        cout << "Addr: " << it << ", Size: " << it->size << endl;
    }
    
    cout << endl << "[6] Find test (on list 3)" << endl;
    for (int i = 0; i < array_size; i++)
    {
        if (list3.find(&(meta_array[i])))
        {
            cout << &(meta_array[i]) << " with size: ";
            cout << meta_array[i].size << " is in the list" << endl;
        }
    }
    for (int i = 0; i < extra_array_size; i++)
    {
        if (list3.find(&(extra_meta_array[i])))
        {
            cout << &(extra_meta_array[i]) << " with size: ";
            cout << extra_meta_array[i].size << " is in the list" << endl;
        }
    }

    return 0;
}