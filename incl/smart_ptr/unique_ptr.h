/**
 * @file unique_ptr.h
 * @author whoami (13003827890@163.com)
 * @brief
 * @version 0.1
 * @date 2024-12-02
 *
 * @copyright Copyright (c) 2024
 *
 */

namespace tiny_std {

template <typename T>
class def_delete {
public:
    def_delete() = default;

    void operator()(T* ptr) const {
        delete ptr;
    }
};

template <typename T>
class def_delete<T[]> {
    def_delete() = default;

    void operator()(T* ptr) const {
        delete[] ptr;
    }
};

template <typename T, typename D>
class uniq_ptr_impl {
public:
    using pointer = T*;

public:
    uniq_ptr_impl() = default;

private:
    pointer ptr_;
    D deleter_;
};

template <typename T, typename D = def_delete<T>>
class unique_ptr {
public:
    using pointer = T*;
    using element = T;

public:
    unique_ptr() : impl_() {}
    unique_ptr(pointer p) : impl_(p) {}

private:
    uniq_ptr_impl<T, D> impl_;
};

}  // namespace tiny_std
