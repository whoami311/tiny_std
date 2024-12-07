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
    uniq_ptr_impl(pointer ptr) : ptr_(ptr) {}

    template<typename Del>
    uniq_ptr_impl(pointer ptr, Del&& deleter) : ptr_(ptr), deleter_(std::forward<Del>(deleter)) {}

    uniq_ptr_impl(uniq_ptr_impl&& u) noexcept : ptr_(u.ptr_), deleter_(u.deleter_) {
        u.ptr_ = nullptr;
    }

    uniq_ptr_impl& operator=(uniq_ptr_impl&& u) noexcept {
        Reset(u.Release());
        deleter_ = std::forward<D>(u.deleter_());
        return this;
    }

public:
    pointer& GetPtr() {
        return ptr_;
    }

    pointer GetPtr() const {
        return ptr_;
    }

    D& GetDeleter() {
        return deleter_;
    }

    D GetDeleter() const {
        return deleter_;
    }

    void Reset(pointer ptr) {
        const pointer old_ptr = ptr_;
        ptr_ = ptr;
        if (old_ptr) {
            deleter_(old_ptr);
        }
    }

    pointer Release() {
        pointer t = ptr_;
        ptr_ = nullptr;
        return t;
    }

    void swap(uniq_ptr_impl& rhs) {
        std::swap(ptr_, rhs.ptr_);
        std::swap(deleter_, rhs.deleter_);
    }

private:
    pointer ptr_;
    D deleter_;
};

template <typename T, typename D = def_delete<T>>
class unique_ptr {
public:
    using pointer = typename uniq_ptr_impl<T, D>::pointer;
    using element_type = T;
    using deleter_type = D;

public:
    unique_ptr() : impl_() {}

    explicit unique_ptr(pointer ptr) : impl_(ptr) {}

    unique_ptr(pointer ptr, const deleter_type& deleter) : impl_(ptr, deleter) {}

    unique_ptr(nullptr_t) : impl_() {}

    unique_ptr(unique_ptr&& ptr) = default;

    unique_ptr(const unique_ptr&) = delete;

    ~unique_ptr() {
        auto& ptr = impl_.GetPtr();
        if (ptr != nullptr)
            impl_.GetDeleter()(ptr);
        ptr = nullptr;
    }

    unique_ptr& operator=(unique_ptr&&) = default;

    unique_ptr& operator=(const unique_ptr&) = delete;

    unique_ptr& operator=(nullptr_t) {
        reset();
        return *this;
    }

    element_type operator*() const {
        return *get();
    }

    pointer operator->() const {
        return get();
    }

    pointer get() const {
        return impl_.GetPtr();
    }

    deleter_type& get_deleter() {
        return impl_.GetDeleter();
    }

    const deleter_type& get_deleter() const {
        return impl_.GetDeleter();
    }

    operator bool() const {
        return !(get() == pointer());
    }

    // Modifiers

    pointer release() {
        return impl_.Release();
    }

    void reset(pointer ptr = pointer()) {
        impl_.Reset(ptr);
    }

    void swap(unique_ptr& u) {
        impl_.swap(u.impl_);
    }

private:
    uniq_ptr_impl<T, D> impl_;
};

template<typename T, typename D>
class unique_ptr<T[], D> {
public:
    using pointer = typename uniq_ptr_impl<T, D>::pointer;
    using element_type = T;
    using deleter_type = D;

public:
    unique_ptr() {}

    

private:
    uniq_ptr_impl<T, D> impl_;
};

}  // namespace tiny_std
