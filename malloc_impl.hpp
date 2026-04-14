// malloc package implementation// 12.04.26// ZeroK

#pragma once

#include <cstdlib>
#include <cstddef>
#include <cstdint>
#include <cassert>
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

        HeapStats()                             = default;
        HeapStats (const HeapStats&)            = delete;
        HeapStats& operator=(const HeapStats&)  = delete;
        HeapStats (HeapStats&&)                 = delete;
        HeapStats& operator=(HeapStats&&)       = delete;
    };


    // alignment
    [[nodiscard]]
    constexpr std::uint64_t align_up (std::uint64_t size, std::uint64_t alignment) noexcept {
        return (size + alignment - 1) & ~(alignment - 1);     
    }


    // pack/unpack
    [[nodiscard]]
    constexpr std::uint64_t pack (std::uint64_t size, std::uint64_t alloc) noexcept {
        return (size | alloc);
    }


    // getters for size and alloc
    [[nodiscard]]
    inline constexpr std::uint64_t get_size (void* p) noexcept {
        std::uint64_t val;
        __builtin_memcpy(&val, p, sizeof(val));
        return val & ~std::uint64_t{0xF};
    }

    [[nodiscard]]
    inline constexpr std::uint64_t get_alloc (void* p) noexcept {
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

        [[nodiscard]] void* zmalloc (std::uint64_t size) noexcept;
                      void zfree(void* ptr) noexcept;
        [[nodiscard]] void* zrealloc (void* ptr, std::uint64_t size) noexcept;


        // ***** ctor *****
        ZMalloc() noexcept {
            void* mem = mmap (
                            nullptr,                            // kernel chooses addr
                            config::HEAP_INIT,                  // 16KB init heap
                            PROT_READ | PROT_WRITE,             // read + write
                            MAP_PRIVATE | MAP_ANONYMOUS,        // private, no backing file
                            -1,                                 // no fd
                             0                                  // no offset
                    );

            if (mem == MAP_FAILED) {
                heap_.start = nullptr;
                heap_.end   = nullptr;
                return;
            }

            heap_.start     = static_cast<std::uint8_t*>(mem);
            heap_.end       = heap_.start + config::HEAP_INIT;
            heap_.capacity  = config::HEAP_INIT;

            // ***** prologue block *****
            std::uint8_t* prologue      = heap_.start;
            std::uint64_t prologue_size = config::HEADER_SIZE + config::FOOTER_SIZE;

            __builtin_memcpy (  prologue,
                                &(const std::uint64_t&) { pack(prologue_size, 1) },
                                config::HEADER_SIZE
                             );

            __builtin_memcpy (  prologue + config::HEADER_SIZE,
                                &(const std::uint64_t&) { pack(prologue_size, 1) },
                                config::FOOTER_SIZE
                             );

            // ***** set first free block *****
            std::uint8_t* first_bptr = prologue + prologue_size;
            std::uint64_t free_size  = config::HEAP_INIT
                                        - prologue_size
                                        - config::HEADER_SIZE;      // EPILOGUE BLOCK

            __builtin_memcpy (  hdrptr(first_bptr),
                                &(const std::uint64_t&) { pack(free_size, 0) },
                                config::HEADER_SIZE
                             );

            __builtin_memcpy (  ftrptr(first_bptr),
                                &(const std::uint64_t&) { pack(free_size, 0) },
                                config::FOOTER_SIZE
                             );

            // ***** set epilogue block *****
            std::uint8_t* epilogue = heap_.end - config::HEADER_SIZE;

            __builtin_memcpy (  epilogue,
                                &(const std::uint64_t&) { pack(0, 1) },
                                config::HEADER_SIZE
                             );

            heap_.used = prologue_size + config::HEADER_SIZE;   

        }   // ZMalloc ctor


        // ***** dtor *****
        ~ZMalloc() noexcept {
            if (heap_.start) 
                munmap (heap_.start, heap_.capacity);
        }

    }; // class ZMalloc


} // namespace zerok
