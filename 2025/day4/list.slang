import memory;

namespace list {
    struct List<T> {
        Ptr<T> mem;
        size_t capacity;
        size_t length;

        __init(size_t initial_capacity) {
            this.mem = memory::allocate<T>(initial_capacity);
            this.capacity = initial_capacity;
            this.length = 0;
        }

        __subscript(size_t index) -> T {
            get { return this.mem[index]; }
            set { this.mem[index] = newValue; }
        }

        void dispose() {
            memory::free(this.mem);
        }

        [mutating]
        void append(T item) {
            let slot = this.length++;
            if (this.length > this.capacity) {
                this.capacity = this.capacity * 3 / 2 + 1;
                this.mem = memory::reallocate(this.mem, this.capacity);
            }

            this.mem[slot] = item;
        }
    }
}