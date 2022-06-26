tests for part 3:

- testing challenge 1-3, and realloc for common sizes (not mmap).

- all tests SHOULD support varient   metadata sizes, except test_challenge_1,
  which has a problem with a big (70 bytes + I think) size of metadata.

- all given sizes to srealloc\sfree\smalloc are multiples of 8 bytes, so even if an alignment isnt defined in your implementation,
  the tests still should work properly.

- put your malloc_3.cpp file and run make, then ./test.

- these are not stress tests so it should run really quickly - if one of the tests stuck for more than 1 sec - something is wrong.

- if you want to run a single test, comment the for loop at the main function, and add the test function you want.