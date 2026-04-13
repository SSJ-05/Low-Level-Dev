// malloc package implementation// 12.04.26// ZeroK



#pragma once


#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <sys/mman.h>
#include <unistd.h>

namespace zerok {


    // ***** constants *****
    namespace config {
        constexpr std::uint64_t ALIGNMENT       { 16 };
        constexpr std::uint64_t HEADER_SIZE     { sizeof(std::uint64_t) };
        constexpr std::uint64_t FOOTER_SIZE     { sizeof(std::uint64_t) };
        constexpr std::uint64_t MIN_BLOCK_SIZE  { ALIGNMENT + HEADER_SIZE + FOOTER_SIZE };
        constexpr std::uint64_t HEAP_INIT       { 4 * 4096 };     // 16KB initial size of Heap
        constexpr std::uint64_t HEAP_MAX        { 1ull << 30 };   // 1GB size limit of Heap
    }   // namespace config


    // ***** Heap Stats struct *****
    struct HeapStats {
        std::uint8_t* start     { nullptr };
        std::uint8_t* end       { nullptr };
        std::uint64_t capacity  { 0 };
        std::uint64_t used      { 0 };

        HeapStats                               = default;
        HeapStats (const HeapStats&)            = delete;
        HeapStats& operator=(const HeapStats&)  = delete;
        HeapStats (HeapStats&&)                 = delete;
        HeapStats& operator=(HeapStats&&)       = delete;
    };    // struct HeapStats

    class zmalloc;
    class zfree;

    class zmalloc {

    };  // class zmalloc

    class zfree {

    };  // class zfree

} // namespace zerok
