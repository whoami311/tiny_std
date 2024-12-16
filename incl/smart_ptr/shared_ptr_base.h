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

template<typename Tp>
class SharedPtr;

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
};

}  // namespace tiny_std
