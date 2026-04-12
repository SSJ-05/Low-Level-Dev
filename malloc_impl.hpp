// malloc package implementation// 12.04.26// ZeroK

/*
NOTEs:
    - need error handling free func that returns explicit error messages when things go wrong
    - no undefined behavior on negative increment
    - padding alignment to 64 bit
    - external fragmentation problem : heuristics that maintain small number of large free blocks rather large number of small free blocks
    - NEEDS TO BE A BALANCE B/W THROUGHPUT AND MEMORY UTILIZATION :                   _
      -- keep track of free blocks                                                     |
      -- which free block to choose based on size of new allocation request            |    DECISIONS ... DECISIONS ...
      -- what to do with remaining free block after placement                          |
      -- what to do with block that is recently freed                                 _|
   
    - implicit free lists : find free blocks by walking entire heap sequentially
        -- header contains (size + allocated bit) 
        -- footer contains the same of previous block
    - explicit free lists - doubly linked lists - only free blocks linked together - ordering of lists LIFO
    - intrusive lists - obj itself contain list ptrs
    - segregated lists - multiple free lists categorized by size class

    - PLACEMENT POLICY : first fit, next fit, best fit
    - SPLITTING POLICY : should work with Placement Policy to utilize free blocks

    - Getting additional heap memory from kernel ( sbrk() ) - swap space or zram
    - COALESCING POLICY : immediate (thrashing) or deferred (preferred)

    - PROLOGUE AND EPILOGUE BLOCKS : trick to eliminate edge cases
*/

#pragma once


#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

namespace zerok {

    class zmalloc;
    class zfree;

    class zmalloc {

    };  // class zmalloc

    class zfree {

    };  // class zfree

} // namespace zerok
