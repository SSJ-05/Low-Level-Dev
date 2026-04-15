// malloc package cpp file// 14.04.26// ZeroK

#include "zmalloc.hpp"
#include <sys/mman.h>

namespace zerok {

    // ***** ctor *****
        ZMalloc::ZMalloc() noexcept {
            void* mem = mmap (
                        nullptr,                            // kernel chooses addr
                        config::HEAP_INIT,                  // 16KB init heap
                        PROT_READ | PROT_WRITE,             // read + write
                        MAP_PRIVATE | MAP_ANONYMOUS,        // private, no backing file
                        -1,                                 // no fd
                        0                                   // no offset
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
        ZMalloc::~ZMalloc() noexcept {
            if (heap_.start) 
                munmap (heap_.start, heap_.capacity);
        }

        // adjust_size - aligned for 16B
        static std::uint64_t adjust_size (std::uint64_t size) noexcept {
            if (size == 0) return 0;

            std::uint64_t asize = align_up(size 
                                    + config::HEADER_SIZE
                                    + config::FOOTER_SIZE,
                                    config::ALIGNMENT);

            if (asize < config::MIN_BLOCK_SIZE)
                asize = config::MIN_BLOCK_SIZE;

            return asize;
        }


    // ***** zmalloc() *****
    [[nodiscard]]
    void* ZMalloc::zmalloc (std::uint64_t size) noexcept {
        if (size == 0) return nullptr;

        std::uint64_t asize = adjusted_size(size);
        void* bptr = find_first_fit(asize);

        if (!bptr) {
            extend_heap(asize);
            if (!bptr) return nullptr;
        }

        place_block(bptr, asize);

        return bptr;
    }

    // implicit list - linear scan of heap
    [[nodiscard]]
    void* ZMalloc::find_first_fit (std::uint64_t size) noexcept {
        std::uint8_t* bptr = heap_.start 
                             + config::HEADER_SIZE 
                             + config::FOOTER_SIZE;

        while (get_size(hdrptr(bptr)) >= 0) {
            if (!get_alloc(hdrptr(bptr)) && get_size(hdrptr(bptr)) >= size)
                return bptr;
            bptr = static_cast<std::uint8_t*>(next_block(bptr));
        }

        return nullptr;     // reach epilogue block - no fit found
    }

    // place_block - split blocks for better mem util
    [[nodiscard]]
    void ZMalloc::place_block (void* bptr, std::uint64_t asize) noexcept {
        std::uint64_t csize = get_size(hdrptr(bptr));

        if ((csize - asize) >= config::MIN_BLOCK_SIZE) {
            // allocated blocks
            __builtin_memcpy ( hdrptr(bptr),
                                &(const std::uint64_t&) { pack(asize, 1) },
                                config::HEADER_SIZE
                            );
            __builtin_memcpy ( ftrptr(bptr),
                                &(const std::uint64_t&) { pack(asize, 1) },
                                config::FOOTER_SIZE
                            );

            // remaining free blocks
            void* next = next_block(bptr);
            std::uint64_t remaining = csize - asize;

            __builtin_memcpy ( hdrptr(next),
                                &(const std::uint64_t&) { pack(remaining, 0) },
                                config::HEADER_SIZE
                            );
            __builtin_memcpy ( ftrptr(next),
                                &(const std::uint64_t&) { pack(remaining, 0) },
                                config::FOOTER_SIZE
                            );
        }
        else {
             __builtin_memcpy ( hdrptr(bptr),
                                &(const std::uint64_t&) { pack(csize, 1) },
                                config::HEADER_SIZE
                            );
            __builtin_memcpy ( ftrptr(bptr),
                                &(const std::uint64_t&) { pack(csize, 1) },
                                config::FOOTER_SIZE
                            );
        }
    }

    [[nodiscard]]
    void* ZMalloc::extend_heap (std::uint64_t size) noexcept {
        size = align_up(size, 4096);

        void* mem = mmap(
                    heap_.end,
                    size,
                    PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS,
                    -1, 0
        );

        if (mem == MAP_FAILED) return nullptr;

        void* bptr      =   heap_.end;
        heap_.end       +=  size;
        heap_.capacity  +=  size;
        
        __builtin_memcpy(
                    hdrptr(bptr),
                    &(const std::uint64_t&) { pack(size, 0) },
                    config::HEADER_SIZE
        );
        __builtin_memcpy(
                    ftrptr(bptr),
                    &(const std::uint64_t&) { pack(size, 0) },
                    config::FOOTER_SIZE 
        );

        return coalesce(bptr);
    }

    // coalesce with boundary tags
    [[nodiscard]]
    void* ZMalloc::coalesce (void* bptr) noexcept {
        bool prev_alloc     = get_alloc(hdrptr(prev_block(bptr)));
        bool next_alloc     = get_alloc(hdrptr(next_block(bptr)));
        std::uint64_t size  = get_size(hdrptr(bptr));
        
        // case 1: prev(alloc), next(alloc) - do nothing
        if (prev_alloc && next_alloc) return bptr;

        // case 2: prev(free), next(alloc) - merge prev
        else if (prev_alloc && !next_alloc) {
            size += get_size(hdrptr(next_block(bptr)));

            __builtin_memcpy(
                    hdrptr(bptr),
                    &(const std::uint64_t&) { pack(size, 0) },
                    config::HEADER_SIZE
            );
            __builtin_memcpy(
                    ftrptr(bptr),
                    &(const std::uint64_t&) { pack(size, 0) },
                    config::FOOTER_SIZE
            );
        }

        // case 3: prev(alloc), next(free) - merge next
        else if (!prev_alloc && next_alloc) {
            size += get_size(hdrptr(prev_block(bptr)));
            bptr = prev_block(bptr);
            
            __builtin_memcpy(
                    hdrptr(bptr),
                    &(const std::uint64_t&) { pack(size, 0) },
                    config::HEADER_SIZE
            );
            __builtin_memcpy(
                    ftrptr(bptr),
                    &(const std::uint64_t&) { pack(size, 0) },
                    config::FOOTER_SIZE
            );
        }

        // case 4: prev(free), next(free) - merge both
        else {
            size += get_size(hdrptr(prev_block(bptr)))
                    + get_size(hdrptr(next_block(bptr)));

            bptr = prev_block(bptr);

            __builtin_memcpy(
                    hdrptr(bptr),
                    &(const std::uint64_t&) { pack(size, 0) },
                    config::HEADER_SIZE
            );
            __builtin_memcpy(
                    ftrptr(bptr),
                    &(const std::uint64_t&) { pack(size, 0) },
                    config::FOOTER_SIZE
            );
        }
        
        return bptr;
    }

    // ***** zfree() *****
    [[nodiscard]]
    void ZMalloc::zfree (void* ptr) noexcept {
        if (!ptr) return;

        std::uint64_t size = get_size(hdrptr(ptr));
        __builtin_memcpy ( hdrptr(ptr),
                           &(const std::uint64_t&) { pack(size, 0) },
                           config::HEADER_SIZE
                          );
        __builtin_memcpy ( ftrptr(ptr),
                          &(const std::uint64_t&) { pack(size, 0) },
                          config::FOOTER_SIZE
                        );

       coalesce(ptr);
    }


    // ***** realloc() *****
    [[nodiscard]]
    void* ZMalloc::zrealloc (void* ptr, std::uint64_t size) noexcept {
        
    }
    
}   // namespace zerok
