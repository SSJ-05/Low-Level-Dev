// malloc package implementation// 12.04.26// ZeroK

#pragma once

#include <cstdint>
#include <cstddef>

namespace zerok {

/*
## block layout (implicit list)

** allocated block:
[ header | payload | footer ]

** free block (future explicit list):
[ header | next_ptr | prev_ptr | unused payload space | footer ]

** header/footer format:
size | alloc_bit

** lowest bit = allocation flag
** remaining bits = block size


## block layout (explicit list)

[prologue]
[free block] -> linked list node
[allocated block]
[free block] -> linked list node
[epilogue]

** traversal becomes
free_list_head <-> free block <-> free block
*/



    // ***** constants *****
    namespace config {
        constexpr std::uint64_t ALIGNMENT       { 16 };
        constexpr std::uint64_t HEADER_SIZE     { sizeof(std::uint64_t) };
        constexpr std::uint64_t FOOTER_SIZE     { sizeof(std::uint64_t) };
        constexpr std::uint64_t MIN_BLOCK_SIZE  { 
                                                      (
                                                        HEADER_SIZE         // header
                                                        + sizeof(void*)     // next
                                                        + sizeof(void*)     // prev
                                                        + FOOTER_SIZE       // footer
                                                        + ALIGNMENT - 1 
                                                      ) & ~(ALIGNMENT - 1)
                                                };
        
        constexpr std::uint64_t HEAP_INIT       { 4 * 4096 };     // 16KB initial size of Heap
        constexpr std::uint64_t HEAP_MAX        { 1ull << 30 };   // 1GB size limit of Heap
        
        constexpr std::uint64_t ALIGN_MASK      { ALIGNMENT - 1 };
        constexpr std::uint64_t SIZE_MASK       { ~ALIGN_MASK };
        constexpr std::uint64_t ALLOC_MASK      { 0x1 };
                                                 
    }   // namespace config


    // ***** Heap Stats struct *****
    struct HeapStats {
        std::byte* start        {};
        std::byte* end          {};
        std::uint64_t capacity  {};

        HeapStats()                             = default;
        HeapStats (const HeapStats&)            = delete;
        HeapStats& operator=(const HeapStats&)  = delete;
        HeapStats (HeapStats&&)                 = delete;
        HeapStats& operator=(HeapStats&&)       = delete;
    };


    // ***** Free block struct *****
    // for explicit lists - chain only free blocks
    struct Freeblock {
        Freeblock* prev;
        Freeblock* next;
    };


    // ***** helper funcs *****
    namespace detail {

    // alignment
    inline constexpr std::uint64_t align_up (std::uint64_t size, std::uint64_t alignment) noexcept {
        return (size + alignment - 1) & ~(alignment - 1);     
    }

    // lower bit stores alloc flag
    inline constexpr std::uint64_t pack (std::uint64_t size, std::uint64_t alloc) noexcept {
        return (size | alloc);
    }

    // getters for size and alloc
    [[nodiscard]]
    inline std::uint64_t get_size (void* p) noexcept {
        std::uint64_t val;
        __builtin_memcpy(&val, p, sizeof(val));
        return val & config::SIZE_MASK;
    }

    [[nodiscard]]
    inline bool is_alloc (void* p) noexcept {
        std::uint64_t val;
        __builtin_memcpy(&val, p, sizeof(val));
        return val & config::ALLOC_MASK;
    }

    // header/footer - ptrs to payload region
    inline void* header_ptr (void* bptr) noexcept {
        return static_cast<std::byte*>(bptr) - config::HEADER_SIZE;
    }

    inline void* footer_ptr (void* bptr) noexcept {
            return static_cast<std::byte*>(bptr) 
                + get_size(header_ptr(bptr))
                - config::HEADER_SIZE 
                - config::FOOTER_SIZE;
    }

    // next/prev block
    inline void* next_block (void* bptr) noexcept {
            return static_cast<std::byte*>(bptr) 
                + get_size(header_ptr(bptr));
    }

    // safe: prologue block always allocated
    inline void* prev_block (void* bptr) noexcept {
            void* prev_ftr = static_cast<std::byte*>(bptr) 
                               - config::HEADER_SIZE; 

            return static_cast<std::byte*>(bptr)
                     - get_size(prev_ftr);
    }


    // helper funcs to cast b/w payload ptr and Freeblock node
    inline Freeblock* as_free_block (void* payload) noexcept {
        return static_cast<Freeblock*> (payload);
    }

    inline void* as_payload (Freeblock* node) noexcept {
        return static_cast<void*> (node);
    }


    } // namespace detail

 

    // ***** class ZMalloc *****
    class ZMalloc {
        HeapStats heap_;

        void* extend_heap (std::uint64_t size)              noexcept;
        void* find_first_fit (std::uint64_t size)           noexcept;
        void* place_block (void* bptr, std::uint64_t size)  noexcept;
        void* coalesce (void* bptr)                         noexcept;

    public:
        ZMalloc()  noexcept;
        ~ZMalloc() noexcept;

        [[nodiscard]] void* zmalloc (std::uint64_t size)             noexcept;
                      void  zfree (void* ptr)                        noexcept;
        [[nodiscard]] void* zrealloc (void* ptr, std::uint64_t size) noexcept;

    };  // class ZMalloc

} // namespace zerok
