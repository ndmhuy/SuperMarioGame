#pragma once

#include <cstddef>
#include <malloc/_malloc.h>
#include <new>

namespace nn {

// TODO: Implement cache-aligned allocator (128 bytes for M1)
template <typename T, std::size_t Alignment = 128>
class AlignedAllocator {
   public:
    using value_type = T;

    AlignedAllocator() noexcept = default;

    template <typename U>
    constexpr AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}

    template <typename U>
    struct rebind {
        using other = AlignedAllocator<U, Alignment>;
    };

    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (n > std::size_t(-1) / sizeof(T)) throw std::bad_alloc();

        void* ptr = nullptr;
        std::size_t bytes = n * sizeof(T);

        if (posix_memalign(&ptr, Alignment, bytes) != 0) throw std::bad_alloc();

        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t n) noexcept { std::free(p); }
};

template <typename T, typename U, std::size_t Alignment>
bool operator==(const AlignedAllocator<T, Alignment>&, const AlignedAllocator<U, Alignment>&) {
    return true;
}

template <typename T, typename U, std::size_t Alignment>
bool operator!=(const AlignedAllocator<T, Alignment>&, const AlignedAllocator<U, Alignment>&) {
    return false;
}

}  // namespace nn
