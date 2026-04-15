// malloc package implementation// 12.04.26// ZeroK

#pragma once

#include <cstdint>

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

        HeapStats()                             = default;
        HeapStats (const HeapStats&)            = delete;
        HeapStats& operator=(const HeapStats&)  = delete;
        HeapStats (HeapStats&&)                 = delete;
        HeapStats& operator=(HeapStats&&)       = delete;
    };


    // alignment
    [[nodiscard]]
    inline constexpr std::uint64_t align_up (std::uint64_t size, std::uint64_t alignment) noexcept {
        return (size + alignment - 1) & ~(alignment - 1);     
    }


    // pack/unpack
    [[nodiscard]]
    inline constexpr std::uint64_t pack (std::uint64_t size, std::uint64_t alloc) noexcept {
        return (size | alloc);
    }


    // getters for size and alloc
    [[nodiscard]]
    inline std::uint64_t get_size (void* p) noexcept {
        std::uint64_t val;
        __builtin_memcpy(&val, p, sizeof(val));
        return val & ~std::uint64_t{0xF};
    }

    [[nodiscard]]
    inline std::uint64_t get_alloc (void* p) noexcept {
        std::uint64_t val;
        __builtin_memcpy(&val, p, sizeof(val));
        return val & std::uint64_t{0x1};
    }


    // header/footer
    [[nodiscard]]
    inline void* hdrptr (void* bptr) noexcept {
        return static_cast<std::uint8_t*>(bptr) - config::HEADER_SIZE;
     }

    [[nodiscard]] 
    inline void* ftrptr (void* bptr) noexcept {
            return static_cast<std::uint8_t*>(bptr) 
                + get_size(hdrptr(bptr))
                - config::HEADER_SIZE 
                - config::FOOTER_SIZE;
     }


    // next/prev block
    [[nodiscard]]
    inline void* next_block (void* bptr) noexcept {
            return static_cast<std::uint8_t*>(bptr) 
                + get_size(hdrptr(bptr));
    }

    [[nodiscard]] 
    inline void* prev_block (void* bptr) noexcept {
            void* prev_ftr = static_cast<std::uint8_t*>(bptr) 
                               - config::HEADER_SIZE 

            return static_cast<std::uint8_t*>(bptr)
                     - get_size(prev_ftr);
    }


    // single instance malloc class
    class ZMalloc {
        HeapStats heap_;

        void* extend_heap (std::uint64_t size) noexcept;
        void* find_first_fit (std::uint64_t size) noexcept;
        void  place_block (void* bptr, std::uint64_t size) noexcept;
        void* coalesce (void* bptr) noexcept;

    public:
        ZMalloc() noexcept;
        ~ZMalloc() noexcept;

        [[nodiscard]] void* zmalloc (std::uint64_t size) noexcept;
        void zfree(void* ptr) noexcept;
        [[nodiscard]] void* zrealloc (void* ptr, std::uint64_t size) noexcept;
    };  // class ZMalloc

} // namespace zerok
