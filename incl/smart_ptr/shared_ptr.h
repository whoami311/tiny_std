/**
 * @file shared_ptr.h
 * @author whoami (13003827890@163.com)
 * @brief
 * @version 0.1
 * @date 2024-12-21
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include "smart_ptr/shared_ptr_base.h"

namespace tiny_std {

template <typename Del, typename Tp>
inline Del* get_deleter(const SharedPtr<Tp>& p) {
    return static_cast<Del*>(p.GetDeleter(typeid(Del)));
}

template <typename Tp>
using NonArray = std::__enable_if_t<!std::is_array<Tp>::value, Tp>;

template <typename Tp>
class shared_ptr : public SharedPtr<Tp> {
    template <typename... Args>
    using Constructible = typename std::enable_if<std::is_constructible<SharedPtr<Tp>, Args...>::value>::type;

    template <typename Arg>
    using Assignable = typename std::enable_if<std::is_assignable<SharedPtr<Tp>&, Arg>::value, shared_ptr&>::type;

public:
    using element_type = typename SharedPtr<Tp>::element_type;

    using weak_type = weak_ptr<Tp>;

    shared_ptr() : SharedPtr<Tp>() {}

    shared_ptr(const shared_ptr&) = default;

    template <typename Yp, typename = Constructible<Yp*>>
    explicit shared_ptr(Yp* p) : SharedPtr<Tp>(p) {}

    template <typename Yp, typename Deleter, typename = Constructible<Yp*, Deleter>>
    shared_ptr(Yp* p, Deleter d) : SharedPtr<Tp>(p, std::move(d)) {}

    template <typename Deleter>
    shared_ptr(nullptr_t p, Deleter d) : SharedPtr<Tp>(p, std::move(d)) {}

    template <typename Yp>
    shared_ptr(const shared_ptr<Yp>& r, element_type* p) : SharedPtr<Tp>(r, p) {}

    template <typename Yp>
    shared_ptr(shared_ptr<Yp>&& r, element_type* p) : SharedPtr<Tp>(std::move(r), p) {}

    template <typename Yp, typename = Constructible<const shared_ptr<Yp>&>>
    shared_ptr(const shared_ptr<Yp>& r) : SharedPtr<Tp>(r) {}

    shared_ptr(shared_ptr&& r) : SharedPtr<Tp>(std::move(r)) {}

    template <typename Yp, typename = Constructible<shared_ptr<Yp>>>
    shared_ptr(shared_ptr<Yp>&& r) : SharedPtr<Tp>(std::move(r)) {}

    template <typename Yp, typename = Constructible<const weak_ptr<Yp>&>>
    explicit shared_ptr(const weak_ptr<Yp>& r) : SharedPtr<Tp>(r) {}

    template <typename Yp, typename Del, typename = Constructible<unique_ptr<Yp, Del>>>
    shared_ptr(unique_ptr<Yp, Del>&& r) : SharedPtr<Tp>(std::move(r)) {}

    shared_ptr(nullptr_t) : shared_ptr() {}

    shared_ptr& operator=(const shared_ptr&) = default;

    template <typename Yp>
    Assignable<const shared_ptr<Yp>&> operator=(const shared_ptr<Yp>& r) {
        this->SharedPtr<Tp>::operator=(r);
        return *this;
    }

    shared_ptr& operator=(shared_ptr&& r) {
        this->SharedPtr<Tp>::operator=(std::move(r));
        return *this;
    }

    template <class Yp>
    Assignable<shared_ptr<Yp>> operator=(shared_ptr<Yp>&& r) {
        this->SharedPtr<Tp>::operator=(std::move(r));
        return *this;
    }

    template <typename Yp, typename Del>
    Assignable<unique_ptr<Yp, Del>> operator=(unique_ptr<Yp, Del>&& r) {
        this->SharedPtr<Tp>::operator=(std::move(r));
        return *this;
    }

private:
    // TODO: make_shared

    shared_ptr(const weak_ptr<Tp>& r, std::nothrow_t) : SharedPtr<Tp>(r, std::nothrow) {}

private:
    friend class weak_ptr<Tp>;
};

template <typename Tp, typename Up>
inline bool operator==(const shared_ptr<Tp>& a, const shared_ptr<Up>& b) {
    return a.get() == b.get();
}

template <typename Tp>
inline bool operator==(const shared_ptr<Tp>& a, nullptr_t) {
    return !a;
}

template <typename Tp>
inline bool operator==(nullptr_t, const shared_ptr<Tp>& a) {
    return !a;
}

template <typename Tp, typename Up>
inline bool operator!=(const shared_ptr<Tp>& a, const shared_ptr<Up>& b) {
    return a.get() != b.get();
}

template <typename Tp>
inline bool operator!=(const shared_ptr<Tp>& a, nullptr_t) {
    return static_cast<bool>(a);
}

template <typename Tp>
inline bool operator!=(nullptr_t, const shared_ptr<Tp>& a) {
    return static_cast<bool>(a);
}

template <typename Tp>
inline void swap(shared_ptr<Tp>& a, shared_ptr<Tp>& b) {
    a.Swap();
}

template <typename Tp, typename Up>
inline shared_ptr<Tp> static_pointer_cast(const shared_ptr<Up>& r) {
    using Sp = shared_ptr<Tp>;
    return Sp(r, static_cast<typename Sp::element_type*>(r.get()));
}

template <typename Tp, typename Up>
inline shared_ptr<Tp> const_pointer_cast(const shared_ptr<Up>& r) {
    using Sp = shared_ptr<Tp>;
    return Sp(r, const_cast<typename Sp::element_type*>(r.get()));
}

template <typename Tp, typename Up>
inline shared_ptr<Tp> dynamic_pointer_cast(const shared_ptr<Up>& r) {
    using Sp = shared_ptr<Tp>;
    if (auto* p = dynamic_cast<typename Sp::element_type*>(r.get()))
        return Sp(r, p);
    return Sp();
}

template <typename Tp, typename Up>
inline shared_ptr<Tp> reinterpret_pointer_cast(const shared_ptr<Up>& r) {
    using Sp = shared_ptr<Tp>;
    return Sp(r, reinterpret_cast<typename Sp::element_type*>(r.get()));
}

template <typename Tp>
class weak_ptr : public WeakPtr<Tp> {
private:
    template <typename Arg>
    using Constructible = typename std::enable_if<std::is_constructible<WeakPtr<Tp>, Arg>::value>::type;

    template <typename Arg>
    using Assignable = typename std::enable_if<std::is_assignable<WeakPtr<Tp>&, Arg>::value, weak_ptr&>::type;

public:
    weak_ptr() = default;

    template <typename Yp, typename = Constructible<const shared_ptr<Yp>&>>
    weak_ptr(const shared_ptr<Yp>& r) : WeakPtr<Tp>(r) {}

    weak_ptr(const weak_ptr&) = default;

    template <typename Yp, typename = Constructible<const weak_ptr<Yp>&>>
    weak_ptr(const weak_ptr<Yp>& r) : WeakPtr<Tp>(r) {}

    weak_ptr(weak_ptr&&) = default;

    template <typename Yp, typename = Constructible<weak_ptr<Yp>>>
    weak_ptr(weak_ptr<Yp>&& r) : WeakPtr<Tp>(std::move(r)) {}

    weak_ptr& operator=(const weak_ptr& r) = default;

    template <typename Yp>
    Assignable<const weak_ptr<Yp>&> operator=(const weak_ptr<Yp>& r) {
        this->WeakPtr<Tp>::operator=(r);
        return *this;
    }

    template <typename Yp>
    Assignable<const shared_ptr<Yp>&> operator=(const shared_ptr<Yp>& r) {
        this->WeakPtr<Tp>::operator=(r);
        return *this;
    }

    weak_ptr& operator=(weak_ptr&& r) = default;

    template <typename Yp>
    Assignable<weak_ptr<Yp>> operator=(weak_ptr<Yp>&& r) {
        this->WeakPtr<Tp>::operator=(std::move(r));
        return *this;
    }

    shared_ptr<Tp> lock() const {
        return shared_ptr<Tp>(*this, std::nothrow);
    }
};

template <typename Tp>
inline void swap(weak_ptr<Tp>& a, weak_ptr<Tp>& b) {
    a.swap(b);
}

template <typename Tp>
class enable_shared_from_this {
protected:
    enable_shared_from_this() {}

    enable_shared_from_this(const enable_shared_from_this&) {}

    enable_shared_from_this& operator=(const enable_shared_from_this&) {
        return *this;
    }

    ~enable_shared_from_this() {}

public:
    shared_ptr<Tp> shared_from_this() {
        return shared_ptr<Tp>(this->_weak_this_);
    }

    shared_ptr<const Tp> shared_from_this() const {
        return shared_ptr<const Tp>(this->_weak_this_);
    }

    weak_ptr<Tp> weak_from_this() {
        return this->_weak_this_;
    }

    weak_ptr<const Tp> weak_from_this() const {
        return this->_weak_this_;
    }

private:
    template <typename Tp1>
    void WeakAssign(Tp1* p, const SharedCount& n) const {
        _weak_this_.Assign(p, n);
    }

    friend const enable_shared_from_this* EnableSharedFromThisBase(const SharedCount&,
                                                                   const enable_shared_from_this* p) {
        return p;
    }

    template <typename>
    friend class SharedPtr;

    mutable weak_ptr<Tp> _weak_this_;
};

// TODO: make_shared

}  // namespace tiny_std
