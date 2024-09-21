#ifndef __C_SMART_PTRS_HPP_
#define __C_SMART_PTRS_HPP_

template <typename tp>
struct cuniqueptr {
    cuniqueptr(tp *p = NULL) : data(p) {}

    ~cuniqueptr() {
        if (data)
            free(data);
    }

    cuniqueptr(const cuniqueptr &) = delete;
    cuniqueptr& operator=(const cuniqueptr &) = delete;

    cuniqueptr(cuniqueptr &&other) : ptr(other.ptr) {
        other.data = NULL;
    }

    cuniqueptr& operator=(cuniqueptr &&other){
        if (this != &other) {
            free(ptr);

            ptr = other.ptr;
            other.ptr = NULL;
        }
        return *this;
    }

    tp &get() const {
        return *data;
    }

    tp *data;
};

#endif//__C_SMART_PTRS_HPP_