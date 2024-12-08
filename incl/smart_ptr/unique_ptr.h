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

#include <type_traits>

namespace tiny_std {

// TODO: constexpr function
// TODO: noexcept

template <typename Tp>
class def_delete {
public:
    def_delete() = default;

    template <typename Up, typename = std::_Require<std::is_convertible<Up*, Tp*>>>
    def_delete(const def_delete<Up>&) {}

    void operator()(Tp* ptr) const {
        delete ptr;
    }
};

template <typename Tp>
class def_delete<Tp[]> {
    def_delete() = default;

    template <typename Up, typename = std::_Require<std::is_convertible<Up (*)[], Tp (*)[]>>>
    def_delete(const def_delete<Up[]>&) {}

    // void operator()(Tp* ptr) const {
    //     delete[] ptr;
    // }
    template <typename Up>
    typename std::enable_if<std::is_convertible<Up (*)[], Tp (*)[]>::value>::type operator()(Up* ptr) const {
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

    template <typename Del>
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

private:
    template <typename Up, typename Ep>
    using safe_conversion_up =
        std::__and_<std::is_convertible<typename unique_ptr<Up, Ep>::pointer, pointer>, std::__not_<std::is_array<Up>>>;

public:
    unique_ptr() : impl_() {}

    explicit unique_ptr(pointer ptr) : impl_(ptr) {}

    unique_ptr(pointer ptr, const deleter_type& deleter) : impl_(ptr, deleter) {}

    unique_ptr(nullptr_t) : impl_() {}

    unique_ptr(unique_ptr&& ptr) = default;

    template <typename Up, typename Ep,
              typename = std::_Require<safe_conversion_up<Up, Ep>,
                                       typename std::conditional<std::is_reference<D>::value, std::is_same<Ep, D>,
                                                                 std::is_convertible<Ep, D>>::type>>
    unique_ptr(unique_ptr<Up, Ep>&& u) noexcept : impl_(u.release(), std::forward<Ep>(u.get_deleter())) {}

    unique_ptr(const unique_ptr&) = delete;

    ~unique_ptr() {
        auto& ptr = impl_.GetPtr();
        if (ptr != nullptr)
            // impl_.GetDeleter()(std::move(ptr));
            impl_.GetDeleter()(ptr);
        ptr = nullptr;
    }

    unique_ptr& operator=(unique_ptr&&) = default;

    template <typename Up, typename Ep>
    typename std::enable_if<std::__and_<safe_conversion_up<Up, Ep>, std::is_assignable<deleter_type&, Ep&&>>::value,
                            unique_ptr&>::type
    operator=(unique_ptr<Up, Ep>&& u) noexcept {
        reset(u.release());
        get_deleter() = std::forward<Ep>(u.get_deleter());
        return *this;
    }

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

template <typename T, typename D>
class unique_ptr<T[], D> {
public:
    using pointer = typename uniq_ptr_impl<T, D>::pointer;
    using element_type = T;
    using deleter_type = D;

    template <typename Up, typename Ep, typename UPtr = unique_ptr<Up, Ep>,
              typename Up_pointer = typename UPtr::pointer, typename Up_element_type = typename UPtr::element_type>
    using safe_conversion_up =
        std::__and_<std::is_array<Up>, std::is_same<pointer, element_type*>, std::is_same<Up_pointer, Up_element_type*>,
                    std::is_convertible<Up_element_type (*)[], element_type (*)[]>>;

    template <typename Up>
    using safe_conversion_raw = std::__and_<
        std::__or_<std::__or_<std::is_same<Up, pointer>, std::is_same<Up, nullptr_t>>,
                   std::__and_<std::is_pointer<Up>, std::is_same<pointer, element_type*>,
                               std::is_convertible<typename std::remove_pointer<Up>::type (*)[], element_type (*)[]>>>>;

public:
    unique_ptr() {}

    template <typename Up, typename = typename std::enable_if<safe_conversion_raw<Up>::value, bool>::type>
    explicit unique_ptr(Up p) : impl_(p) {}

    template <typename Up, typename Del = deleter_type,
              typename = std::_Require<safe_conversion_raw<Up>, std::is_copy_assignable<Del>>>
    unique_ptr(Up p, const deleter_type& d) : impl_(p, d) {}

    template <typename Up, typename Del = deleter_type,
              typename = std::_Require<safe_conversion_raw<Up>, std::is_move_constructible<Del>>>
    unique_ptr(Up p, std::__enable_if_t<!std::is_lvalue_reference<Del>::value, Del&&> d) noexcept
        : impl_(std::move(p), std::move(d)) {}

    template <typename Up, typename Del = deleter_type, typename DelUnRef = typename std::remove_reference<Del>::type,
              typename = std::_Require<safe_conversion_raw<Up>>>
    unique_ptr(Up, std::__enable_if_t<std::is_lvalue_reference<Del>::value, DelUnRef&&>) = delete;

    unique_ptr(const unique_ptr&) = delete;

    unique_ptr(unique_ptr&&) = default;

    template <typename Del = D>
    unique_ptr(nullptr_t) : impl_() {}

    template <typename Up, typename Ep,
              typename = std::_Require<safe_conversion_up<Up, Ep>,
                                       typename std::conditional<std::is_reference<D>::value, std::is_same<Ep, D>,
                                                                 std::is_convertible<Ep, D>>::type>>
    unique_ptr(unique_ptr<Up, Ep>&& u) noexcept : impl_(u.release(), std::forward<Ep>(u.get_deleter())) {}

    ~unique_ptr() {
        auto& ptr = impl_.GetPtr();
        if (ptr != nullptr)
            get_deleter()(ptr);
        ptr = nullptr;
    }

    unique_ptr& operator=(const unique_ptr&) = delete;

    unique_ptr& operator=(unique_ptr&&) = default;

    template <typename Up, typename Ep>
    typename std::enable_if<std::__and_<safe_conversion_up<Up, Ep>, std::is_assignable<deleter_type&, Ep&&>>::value,
                            unique_ptr&>::type
    operator=(unique_ptr<Up, Ep>&& u) noexcept {
        reset(u.release());
        get_deleter() = std::forward<Ep>(u.get_deleter());
        return *this;
    }

    unique_ptr& operator=(nullptr_t) {
        reset();
        return *this;
    }

    typename std::add_lvalue_reference<element_type>::type operator[](size_t i) const {
        return get()[i];
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

    explicit operator bool() const {
        return !get() == pointer();
    }

    pointer release() {
        return impl_.Release();
    }

    template <typename Up,
              typename = std::_Require<std::__or_<
                  std::is_same<Up, pointer>,
                  std::__and_<std::is_same<pointer, element_type*>, std::is_pointer<Up>,
                              std::is_convertible<typename std::remove_pointer<Up>::type (*)[], element_type (*)[]>>>>>
    void reset(Up p) {
        impl_.Reset(std::move(p));
    }

    void reset(nullptr_t = nullptr) {
        reset(pointer());
    }

    void swap(unique_ptr& u) {
        impl_.swap(u.impl_);
    }

private:
    uniq_ptr_impl<T, D> impl_;
};

}  // namespace tiny_std
