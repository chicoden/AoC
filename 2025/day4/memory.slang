namespace memory {
    namespace detail {
        void *malloc(size_t size) {
            __intrinsic_asm "malloc($0)";
        }

        void *realloc(void *ptr, size_t new_size) {
            __intrinsic_asm "realloc($0, $1)";
        }

        void free(void *ptr) {
            __intrinsic_asm "free($0)";
        }
    }

    Ptr<T> allocate<T>(size_t count) {
        return reinterpret<Ptr<T>>(detail::malloc(sizeof(T) * count));
    }

    Ptr<T> reallocate<T>(Ptr<T> mem, size_t new_count) {
        return reinterpret<Ptr<T>>(detail::realloc(reinterpret<void*>(mem), sizeof(T) * new_count));
    }

    void free<T>(Ptr<T> mem) {
        detail::free(reinterpret<void*>(mem));
    }
}
