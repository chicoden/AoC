#pragma once

size_t round_up(size_t offset, size_t align) {
    return (offset + align - 1) / align * align;
}

class StructBuilder {
private:
    size_t size;

public:
    StructBuilder() : size(0) {};
    ~StructBuilder() {};

    template<typename T>
    size_t add(size_t count) {
        size_t offset = round_up(this->size, alignof(T));
        this->size = offset + count * sizeof(T);
        return offset;
    }

    size_t total_size() const {
        return this->size;
    }
};
