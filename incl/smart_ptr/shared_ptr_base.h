/**
 * @file shared_ptr_base.h
 * @author whoami (13003827890@163.com)
 * @brief
 * @version 0.1
 * @date 2024-12-14
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

#include <atomic>
#include <type_traits>

#include <smart_ptr/unique_ptr.h>

namespace tiny_std {

class SpCountedBase {
public:
    SpCountedBase() : use_cnt_(1), weak_cnt_(1) {}

    virtual ~SpCountedBase() {}

    virtual void Dispose() = 0;

    virtual void Destroy() {
        delete this;
    }

    virtual void* GetDeleter(const std::type_info&) = 0;

    void AddRefCopy() {
        use_cnt_++;
    }

    bool AddRefLock() {
        // if (use_cnt_.fetch_add(1) == 0)
        if (use_cnt_++ == 0)
            return false;
        return true;
    }

    void Release() {
        // if (use_cnt_.fetch_sub(1) == 1) {
        if (--use_cnt_ == 0) {
            ReleaseLastUse();
        }
    }

    void ReleaseLastUse() {
        Dispose();
        // if (weak_cnt_.fetch_sub(1) == 1) {
        if (--weak_cnt_ == 0) {
            Destroy();
        }
    }

    void ReleaseLastUseCold() {
        ReleaseLastUse();
    }

    void WeakAddRef() {
        weak_cnt_++;
    }

    void WeakRelease() {
        // if (weak_cnt_.fetch_sub(1) == 1) {
        if (--weak_cnt_ == 0) {
            Destroy();
        }
    }

    int GetUseCnt() const {
        return use_cnt_;
    }

private:
    SpCountedBase(SpCountedBase const&) = delete;
    SpCountedBase& operator=(SpCountedBase const&) = delete;

    std::atomic<int> use_cnt_;
    std::atomic<int> weak_cnt_;
};

template <typename Tp>
class SharedPtr;

template <typename Tp>
class WeakPtr;

template <typename Tp>
class weak_ptr;

template <typename Tp>
class EnableSharedFromThis;

template <typename Tp>
class enable_shared_from_this;

class WeakCount;

// TODO:
// class SpCountedDeleter {};

template <typename Ptr>
class SpCountedPtr final : public SpCountedBase {
public:
    explicit SpCountedPtr(Ptr p) : ptr_(p) {}

    void Dispose() override {
        delete ptr_;
    }

    void Destroy() override {
        delete this;
    }

    void* GetDeleter(const std::type_info&) override {
        return nullptr;
    }

    SpCountedPtr(const SpCountedPtr&) = delete;
    SpCountedPtr& operator=(const SpCountedPtr&) = delete;

private:
    Ptr ptr_;
};

template <>
inline void SpCountedPtr<nullptr_t>::Dispose() {}

struct SpArrayDelete {
    template <typename Yp>
    void operator()(Yp* p) const {
        delete[] p;
    }
};

class SharedCount {
public:
    SharedCount() : pi_(0) {}

    template <typename Ptr>
    explicit SharedCount(Ptr p) : pi_(0) {
        pi_ = new SpCountedPtr<Ptr>(p);
    }

    template <typename Ptr>
    SharedCount(Ptr p, std::false_type) : SharedCount(p) {}

    template <typename Ptr>
    SharedCount(Ptr p, std::true_type) : SharedCount(p, SpArrayDelete{}) {}

    template <typename Ptr, typename Deleter>
    SharedCount(Ptr p, Deleter d) : SharedCount(p, std::move(d)) {}

    // TODO: constructor for unique_ptr
    // template <typename Tp, typename Del>
    // explicit SharedCount(tiny_std::unique_ptr<Tp, Del>&& r) : pi_(0) {
    //     if (r.get() == nullptr)
    //         return;
    // }

    // template <typename Tp, typename Del>
    // explicit SharedCount(tiny_std::unique_ptr<Tp, Del>&& r)

    explicit SharedCount(const WeakCount& r);

    ~SharedCount() {
        if (pi_ != nullptr)
            pi_->Release();
    }

    SharedCount(const SharedCount& r) : pi_(r.pi_) {
        if (pi_ != nullptr) {
            pi_->AddRefCopy();
        }
    }

    SharedCount& operator=(const SharedCount& r) {
        SpCountedBase* tmp = r.pi_;
        if (tmp != pi_) {
            if (tmp != nullptr)
                tmp->AddRefCopy();
            if (pi_ != nullptr)
                pi_->Release();
            pi_ = tmp;
        }
        return *this;
    }

    void Swap(SharedCount& r) {
        SpCountedBase* tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    int GetUseCount() const {
        return pi_ ? pi_->GetUseCnt() : 0;
    }

    bool Unique() const {
        return GetUseCount() == 1;
    }

    void* GetDeleter(const std::type_info& ti) const {
        return pi_ ? pi_->GetDeleter(ti) : nullptr;
    }

    friend inline bool operator==(const SharedCount& a, const SharedCount& b) {
        return a.pi_ == b.pi_;
    }

private:
    friend class WeakCount;
    SpCountedBase* pi_;
};

class WeakCount {
public:
    WeakCount() : pi_(nullptr) {}

    WeakCount(const SharedCount& r) : pi_(r.pi_) {
        if (pi_ != nullptr)
            pi_->WeakAddRef();
    }

    WeakCount(const WeakCount& r) : pi_(r.pi_) {
        if (pi_ != nullptr)
            pi_->WeakAddRef();
    }

    WeakCount(WeakCount&& r) : pi_(r.pi_) {
        r.pi_ = nullptr;
    }

    ~WeakCount() {
        if (pi_ != nullptr)
            pi_->WeakRelease();
    }

    WeakCount& operator=(const SharedCount& r) {
        SpCountedBase* tmp = r.pi_;
        if (tmp != nullptr)
            tmp->WeakAddRef();
        if (r.pi_ != nullptr)
            r.pi_->WeakRelease();
        pi_ = tmp;
        return *this;
    }

    WeakCount& operator=(const WeakCount& r) {
        SpCountedBase* tmp = r.pi_;
        if (tmp != nullptr)
            tmp->WeakAddRef();
        if (r.pi_ != nullptr)
            r.pi_->WeakRelease();
        pi_ = tmp;
        return *this;
    }

    WeakCount& operator=(WeakCount&& r) {
        if (pi_ != nullptr)
            pi_->WeakRelease();
        pi_ = r.pi_;
        r.pi_ = nullptr;
        return *this;
    }

    void Swap(WeakCount& r) {
        SpCountedBase* tmp = r.pi_;
        r.pi_ = pi_;
        pi_ = tmp;
    }

    int GetUseCount() const {
        return pi_ != nullptr ? pi_->GetUseCnt() : 0;
    }

    friend inline bool operator==(const WeakCount& a, const WeakCount& b) {
        return a.pi_ == b.pi_;
    }

private:
    friend class SharedCount;
    SpCountedBase* pi_;
};

inline SharedCount::SharedCount(const WeakCount& r) : pi_(r.pi_) {
    if (pi_ && !pi_->AddRefLock())
        pi_ = nullptr;
}

template <typename Yp_ptr, typename Tp_ptr>
struct SpCompatibleWith : std::false_type {};

template <typename Yp, typename Tp>
struct SpCompatibleWith<Yp*, Tp*> : std::is_convertible<Yp*, Tp*>::type {};

template <typename Up, size_t Nm>
struct SpCompatibleWith<Up (*)[Nm], Up (*)[]> : std::true_type {};

template <typename Up, size_t Nm>
struct SpCompatibleWith<Up (*)[Nm], const Up (*)[]> : std::true_type {};

template <typename Up, size_t Nm>
struct SpCompatibleWith<Up (*)[Nm], volatile Up (*)[]> : std::true_type {};

template <typename Up, size_t Nm>
struct SpCompatibleWith<Up (*)[Nm], const volatile Up (*)[]> : std::true_type {};

template <typename Up, size_t Nm, typename Yp, typename = void>
struct SpIsConstructibleArrN : std::false_type {};

template <typename Up, size_t Nm, typename Yp>
struct SpIsConstructibleArrN<Up, Nm, Yp, std::__void_t<Yp[Nm]>> : std::is_convertible<Yp (*)[Nm], Up (*)[Nm]>::type {};

template <typename Up, typename Yp, typename = void>
struct SpIsConstructibleArr : std::false_type {};

template <typename Up, typename Yp>
struct SpIsConstructibleArr<Up, Yp, std::__void_t<Yp[]>> : std::is_convertible<Yp (*)[], Up (*)[]>::type {};

template <typename Tp, typename Yp>
struct SpIsConstructible;

template <typename Up, size_t Nm, typename Yp>
struct SpIsConstructible<Up[Nm], Yp> : SpIsConstructibleArrN<Up, Nm, Up>::type {};

template <typename Up, typename Yp>
struct SpIsConstructible<Up[], Yp> : SpIsConstructibleArr<Up, Yp>::type {};

template <typename Tp, typename Yp>
struct SpIsConstructible : std::is_convertible<Yp*, Tp*>::type {};

template <typename Tp, bool = std::is_array<Tp>::value, bool = std::is_void<Tp>::value>
class SharedPtrAccess {
public:
    using element_type = Tp;

    element_type& operator*() const {
        return *Get();
    }

    element_type* operator->() const {
        return Get();
    }

private:
    element_type* Get() const {
        return static_cast<const SharedPtr<Tp>*>(this)->Get();
    }
};

template <typename Tp>
class SharedPtrAccess<Tp, false, true> {
public:
    using element_type = Tp;

    element_type* operator->() const {
        auto ptr = static_cast<const SharedPtr<Tp>*>(this)->Get();
        return ptr;
    }
};

template <typename Tp>
class SharedPtrAccess<Tp, true, false> {
public:
    using element_type = typename std::remove_extent<Tp>::type;

    element_type& operator[](std::ptrdiff_t i) const {
        return Get()[i];
    }

private:
    element_type* Get() const {
        return static_cast<const SharedPtr<Tp>*>(this)->Get();
    }
};

template <typename Tp>
class SharedPtr : public SharedPtrAccess<Tp> {
public:
    using element_type = typename std::remove_extent<Tp>::type;

private:
    template <typename Yp>
    using SafeConv = typename std::enable_if<SpIsConstructible<Tp, Yp>::value>::type;

    template <typename Yp, typename Res = void>
    using Compatible = typename std::enable_if<SpCompatibleWith<Yp*, Tp*>::value, Res>::type;

    template <typename Yp>
    using Assignable = Compatible<Yp, SharedPtr&>;

    template <typename Yp, typename Del, typename Res = void, typename Ptr = typename unique_ptr<Yp, Del>::pointer>
    using UniqCompatible =
        std::__enable_if_t<std::__and_<SpCompatibleWith<Yp*, Tp*>, std::is_convertible<Ptr, element_type*>,
                                       std::is_move_constructible<Del>>::value,
                           Res>;

    template <typename Yp, typename Del>
    using UniqAssignable = UniqCompatible<Yp, Del, SharedPtr&>;

public:
    using weak_type = WeakPtr<Tp>;

    SharedPtr() : ptr_(0), ref_count_() {}

    template <typename Yp, typename = SafeConv<Yp>>
    explicit SharedPtr(Yp* p) : ptr_(p), ref_count_(p, typename std::is_array<Tp>::type()) {
        EnableSharedFromThisWith(p);
    }

    template <typename Yp, typename Deleter, typename = SafeConv<Yp>>
    SharedPtr(Yp* p, Deleter d) : ptr_(p), ref_count_(p, std::move(d)) {
        EnableSharedFromThisWith(p);
    }

    template <typename Deleter>
    SharedPtr(std::nullptr_t p, Deleter d) : ptr_(0), ref_count_(p, std::move(d)) {}

    template <typename Yp>
    SharedPtr(const SharedPtr<Yp>& r, element_type* p) : ptr_(p), ref_count_(r.ref_count_) {}

    template <typename Yp>
    SharedPtr(SharedPtr<Yp>&& r, element_type* p) : ptr_(p), ref_count_() {
        ref_count_.Swap(r.ref_count_);
        r.ptr_ = nullptr;
    }

    SharedPtr(const SharedPtr&) = default;
    SharedPtr& operator=(const SharedPtr&) = default;
    ~SharedPtr() = default;

    template <typename Yp, typename = Compatible<Yp>>
    SharedPtr(const SharedPtr<Yp>& r) : ptr_(r.ptr_), ref_count_(r.ref_count_) {}

    SharedPtr(SharedPtr&& r) : ptr_(r.ptr_), ref_count_() {
        ref_count_.Swap(r.ref_count_);
        r.ptr_ = nullptr;
    }

    template <typename Yp, typename = Compatible<Yp>>
    SharedPtr(SharedPtr<Yp>&& r) : ptr_(r.ptr_), ref_count_() {
        ref_count_.Swap(r.ref_count_);
        r.ptr_ = nullptr;
    }

    template <typename Yp, typename = Compatible<Yp>>
    explicit SharedPtr(const WeakPtr<Yp>& r) : ref_count_(r.ref_count) {
        ptr_ = r.ptr_;
    }

    // TODO: why ref_count_ in initialization list
    template <typename Yp, typename Del, typename = UniqCompatible<Yp, Del>>
    SharedPtr(unique_ptr<Yp, Del>&& r) : ptr_(r.get()), ref_count_() {
        auto raw = std::__to_address(r.get());
        ref_count_ = SharedCount(std::move(r));
        EnableSharedFromThisWith(raw);
    }

    SharedPtr(nullptr_t) : SharedPtr() {}

    template <typename Yp>
    Assignable<Yp> operator=(const SharedPtr<Yp>& r) {
        ptr_ = r.ptr_;
        ref_count_ = r.ref_count_;
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& r) {
        SharedPtr(std::move(r)).Swap(*this);
        return *this;
    }

    template <typename Yp, typename Del>
    UniqAssignable<Yp, Del> operator=(unique_ptr<Yp, Del>&& r) {
        SharedPtr(std::move(r)).Swap(*this);
        return *this;
    }

    void Reset() {
        SharedPtr().Swap(*this);
    }

    template <typename Yp>
    SafeConv<Yp> Reset(Yp* p) {
        SharedPtr(p).Swap(*this);
    }

    template <typename Yp, typename Deleter>
    SafeConv<Yp> Reset(Yp* p, Deleter d) {
        SharedPtr(p, std::move(d)).Swap(*this);
    }

    element_type* Get() const {
        return ptr_;
    }

    explicit operator bool() const {
        return ptr_ != nullptr;
    }

    bool Unique() const {
        return ref_count_.Unique();
    }

    int UseCount() const {
        return ref_count_.GetUseCount();
    }

    void Swap(SharedPtr<Tp>& other) {
        std::swap(ptr_, other.ptr_);
        ref_count_.Swap(other.ref_count_);
    }

protected:
    // TODO: allocate_shared

    SharedPtr(const WeakPtr<Tp>& r, std::nothrow_t) : ref_count_(r.ref_count_, std::nothrow) {
        ptr_ = ref_count_.GetUseCount() ? r.ptr_ : nullptr;
    }

    friend class WeakPtr<Tp>;

private:
    template <typename Yp>
    using esft_base_t = decltype(EnableSharedFromThisBase(std::declval<const SharedCount&>(), std::declval<Yp*>()));

    template <typename Yp, typename = void>
    struct HasEsftBase : std::false_type {};

    template <typename Yp>
    struct HasEsftBase<Yp, std::__void_t<esft_base_t<Yp>>> : std::__not_<std::is_array<Tp>> {};

    template <typename Yp, typename Yp2 = typename std::remove_cv<Yp>::type>
    typename std::enable_if<HasEsftBase<Yp2>::value>::type EnableSharedFromThisWith(Yp* p) {
        if (auto base = EnableSharedFromThisBase(ref_count_, p))
            base->WeakAssign(const_cast<Yp2*>(p), ref_count_);
    }

    template <typename Yp, typename Yp2 = typename std::remove_cv<Yp>::type>
    typename std::enable_if<!HasEsftBase<Yp2>::value>::type EnableSharedFromThisWith(Yp*) {}

private:
    element_type* ptr_;
    SharedCount ref_count_;
};

template <typename Tp1, typename Tp2>
inline bool operator==(const SharedPtr<Tp1>& a, const SharedPtr<Tp2>& b) {
    return a.Get() == a.Get();
}

template <typename Tp>
inline bool operator==(const SharedPtr<Tp>& a, nullptr_t) {
    return !a;
}

template <typename Tp>
inline bool operator==(nullptr_t, const SharedPtr<Tp>& a) {
    return !a;
}

template <typename Tp1, typename Tp2>
inline bool operator!=(const SharedPtr<Tp1>& a, const SharedPtr<Tp2>& b) {
    return a.Get() != b.Get();
}

template <typename Tp>
inline bool operator!=(const SharedPtr<Tp>& a, nullptr_t) {
    return static_cast<bool>(a);
}

template <typename Tp>
inline bool operator!=(nullptr_t, const SharedPtr<Tp>& a) {
    return static_cast<bool>(a);
}

template <typename Tp>
inline void swap(SharedPtr<Tp>& a, SharedPtr<Tp>& b) {
    a.Swap(b);
}

template <typename Tp, typename Tp1>
inline SharedPtr<Tp> static_pointer_cast(const SharedPtr<Tp1>& r) {
    using Sp = SharedPtr<Tp>;
    return Sp(r, static_cast<typename Sp::element_type*>(r.Get()));
}

template <typename Tp, typename Tp1>
inline SharedPtr<Tp> const_pointer_cast(const SharedPtr<Tp1>& r) {
    using Sp = SharedPtr<Tp>;
    return Sp(r, const_cast<typename Sp::element_type*>(r.Get()));
}

template <typename Tp, typename Tp1>
inline SharedPtr<Tp> dynamic_pointer_cast(const SharedPtr<Tp1>& r) {
    using Sp = SharedPtr<Tp>;
    if (auto* p = dynamic_cast<typename Sp::element_type*>(r.Get()))
        return Sp(r, p);
    return Sp();
}

template <typename Tp, typename Tp1>
inline SharedPtr<Tp> reinterpret_pointer_cast(const SharedPtr<Tp1>& r) {
    using Sp = SharedPtr<Tp>;
    return Sp(r, reinterpret_cast<typename Sp::element_type*>(r.Get()));
}

template <typename Tp>
class WeakPtr {
    template <typename Yp, typename Res = void>
    using Compatible = typename std::enable_if<SpCompatibleWith<Yp*, Tp*>::value, Res>::type;

    template <typename Yp>
    using Assignable = Compatible<Yp, WeakPtr&>;

public:
    using element_type = typename std::remove_extent<Tp>::type;

    WeakPtr() : ptr_(nullptr), ref_count_() {}

    WeakPtr(const WeakPtr&) = default;

    ~WeakPtr() = default;

    template <typename Yp, typename = Compatible<Yp>>
    WeakPtr(const WeakPtr<Yp>& r) : ref_count_(r.ref_count_) {
        ptr_ = r.Lock().Get();
    }

    template <typename Yp, typename = Compatible<Yp>>
    WeakPtr(const SharedPtr<Yp>& r) : ptr_(r.ptr_), ref_count_(r.ref_count_) {}

    WeakPtr(WeakPtr&& r) : ptr_(r.ptr_), ref_count_(std::move(r.ref_count_)) {
        r.ptr_ = nullptr;
    }

    template <typename Yp, typename = Compatible<Yp>>
    WeakPtr(WeakPtr<Yp>&& r) : ptr_(r.Lock().Get()), ref_count_(std::move(r.ref_count_)) {
        r.ptr_ = nullptr;
    }

    WeakPtr& operator=(const WeakPtr& r) = default;

    template <typename Yp>
    Assignable<Yp> operator=(const WeakPtr<Yp>& r) {
        ptr_ = r.Lock().Get();
        ref_count_ = r.ref_count_;
        return *this;
    }

    template <typename Yp>
    Assignable<Yp> operator=(const SharedPtr<Yp>& r) {
        ptr_ = r.ptr_;
        ref_count_ = r.ref_count_;
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& r) {
        WeakPtr(std::move(r)).Swap(*this);
        return *this;
    }

    template <typename Yp>
    Assignable<Yp> operator=(WeakPtr<Yp>&& r) {
        ptr_ = r.Lock().Get();
        ref_count_ = std::move(r.ref_count_);
        return *this;
    }

    SharedPtr<Tp> Lock() const {
        return SharedPtr<element_type>(*this, std::nothrow);
    }

    int UseCount() const {
        return ref_count_.GetUseCount();
    }

    bool Expired() const {
        return ref_count_.GetUseCount() == 0;
    }

    // template <typename Yp1>
    // bool owner_before(const SharedPtr<Yp1>& rhs) const {
    //     return ref_count_.Less(rhs.ref_count_);
    // }

    void Reset() {
        WeakPtr().swap(*this);
    }

    void Swap(WeakPtr& s) {
        std::swap(ptr_, s.ptr_);
        ref_count_.Swap(s.ref_count_);
    }

private:
    void Assign(Tp* ptr, const SharedCount& ref_count) {
        if (UseCount() == 0) {
            ptr_ = ptr;
            ref_count_ = ref_count;
        }
    }

    template <typename Tp1>
    friend class SharedPtr;

    // friend self ?
    template <typename Tp1>
    friend class WeakPtr;

    friend class EnableSharedFromThis<Tp>;
    friend class enable_shared_from_this<Tp>;

    element_type* ptr_;
    WeakCount ref_count_;
};

template <typename Tp>
inline void swap(WeakPtr<Tp>& a, WeakPtr<Tp>& b) {
    a.Swap(b);
}

template <typename Tp>
class EnableSharedFromThis {
    EnableSharedFromThis() {}

    EnableSharedFromThis(const EnableSharedFromThis&) {}

    EnableSharedFromThis& operator=(const EnableSharedFromThis&) {
        return *this;
    }

    ~EnableSharedFromThis() {}

public:
    SharedPtr<Tp> SharedFromThis() {
        return SharedPtr<Tp>(this->_weak_this_);
    }

    SharedPtr<const Tp>
    SharedFromThis() const {
        return SharedPtr<const Tp>(this->_weak_this_);
    }

    WeakPtr<Tp> WeakFromThis() {
        return this->_weak_this_;
    }

    WeakPtr<const Tp> WeakFromThis() const {
        return this->_weak_this_;
    }

private:
    template <typename Tp1>
    void WeakAssign(Tp* p, const SharedCount& n) const {
        _weak_this_.Assign(p, n);
    }

private:
    friend const EnableSharedFromThis* EnableSharedFromThisBase(const SharedCount&, const EnableSharedFromThis* p) {
        return p;
    }

    template<typename>
    friend class SharedPtr;

    // named to avoid repeating names
    mutable WeakPtr<Tp> _weak_this_;
};

// TODO: MakeShared

}  // namespace tiny_std
