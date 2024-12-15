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

class SpCountedDeleter {};

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

    template <typename Tp, typename Del>
    explicit SharedCount(tiny_std::unique_ptr<Tp, Del>&& r) : pi_(0) {
        if (r.get() == nullptr)
            return;
        
    }

private:
    SpCountedBase* pi_;
};

}  // namespace tiny_std
