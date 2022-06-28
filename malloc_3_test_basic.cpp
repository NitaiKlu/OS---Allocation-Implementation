#include "tests/my_stdlib.h"
#include <catch2/catch_test_macros.hpp>

#include <unistd.h>

#define MAX_ALLOCATION_SIZE (1e8)
#define MMAP_THRESHOLD (128 * 1024)
#define MIN_SPLIT_SIZE (128)

static inline size_t aligned_size(size_t size)
{
    return (size % 8) ? (size & (size_t)(-8)) + 8 : size;
}

class Stats
{
public:
    size_t num_free_blocks;
    size_t num_free_bytes;
    size_t num_allocated_blocks;
    size_t num_allocated_bytes;
    size_t num_meta_bytes;
    size_t size_meta_data;

    Stats();
    Stats(size_t a, size_t b, size_t c, size_t d, size_t e, size_t f);
    void set();
    };
Stats::Stats()
{
    num_free_blocks = 0;
    num_free_bytes = 0;
    num_allocated_blocks = 0;
    num_allocated_bytes = 0;
    num_meta_bytes = 0;
    size_meta_data = 0;
}
Stats::Stats(size_t a, size_t b, size_t c, size_t d, size_t e, size_t f)
{
    num_free_blocks = a;
    num_free_bytes = b;
    num_allocated_blocks = c;
    num_allocated_bytes = d;
    num_meta_bytes = e;
    size_meta_data = f;
};

void Stats::set()
{
    num_free_blocks = _num_free_blocks();
    num_free_bytes = _num_free_bytes();
    num_allocated_blocks = _num_allocated_blocks();
    num_allocated_bytes = _num_allocated_bytes();
    num_meta_bytes = _num_meta_data_bytes();
    size_meta_data = _size_meta_data();
};


#define verify_blocks(allocated_blocks, allocated_bytes, free_blocks, free_bytes) \
    do                                                                            \
    {                                                                             \
        REQUIRE(_num_allocated_blocks() == allocated_blocks);                                                          \
        REQUIRE(_num_allocated_bytes() == aligned_size(allocated_bytes));                                              \
        REQUIRE(_num_free_blocks() == free_blocks);                                                                    \
        REQUIRE(_num_free_bytes() == aligned_size(free_bytes));                                                        \
        REQUIRE(_num_meta_data_bytes() == aligned_size(_size_meta_data() * allocated_blocks));                         \
    } while (0)


#define verify_size(base)                                                                             \
    do                                                                                                \
    {                                                                                                 \
        void *after = sbrk(0);                                                                        \
        REQUIRE(_num_allocated_bytes() + aligned_size(_size_meta_data() * _num_allocated_blocks()) == \
                (size_t)after - (size_t)base);                                                        \
    } while (0)

#define verify_size_with_large_blocks(base, diff)      \
    do                                                 \
    {                                                  \
        void *after = sbrk(0);                         \
        REQUIRE(diff == (size_t)after - (size_t)base); \
    } while (0)


TEST_CASE("Alignment MMAP", "[malloc3]")
{
    // verify_blocks(0, 0, 0, 0);
    void *base = sbrk(0);
    Stats s;
    REQUIRE(_size_meta_data() % 8 == 0);
    REQUIRE(_num_allocated_bytes() % 8 == 0);
    REQUIRE(_num_free_bytes() % 8 == 0);
    s.set();
    char *a = (char *)smalloc(MMAP_THRESHOLD);
    s.set();
    REQUIRE(a != nullptr);
    REQUIRE((size_t)a % 8 == 0);
    verify_blocks(1, MMAP_THRESHOLD, 0, 0);
    verify_size_with_large_blocks(base, 0);
    REQUIRE(_size_meta_data() % 8 == 0);
    REQUIRE(_num_allocated_bytes() % 8 == 0);
    REQUIRE(_num_free_bytes() % 8 == 0);

    sfree(a);
    verify_blocks(0, 0, 0, 0);
    verify_size(base);
    REQUIRE(_size_meta_data() % 8 == 0);
    REQUIRE(_num_allocated_bytes() % 8 == 0);
    REQUIRE(_num_free_bytes() % 8 == 0);
}
