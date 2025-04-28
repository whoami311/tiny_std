/**
 * @file function.h
 * @author whoami (13003827890@163.com)
 * @brief
 * @version 0.1
 * @date 2025-04-28
 *
 * @copyright Copyright (c) 2025
 *
 */

namespace tiny_std {

class UndefinedClass;

union NocopyTypes {
    void* object_;
    const void* const_object_;
    void (*function_pointer_)();
    void (UndefinedClass::*member_pointer_)();
};

union AnyData {
    void* Access() noexcept {
        return &pod_data_[0];
    }

    const void* Access() const noexcept {
        return &pod_data_[0];
    }

    template <typename Tp>
    Tp& Access() noexcept {
        return *static_cast<Tp*>(Access());
    }

    template <typename Tp>
    const Tp& Access() const noexcept {
        return *static_cast<const Tp*>(Access());
    }

    NocopyTypes unused_;
    char pod_data_[sizeof(NocopyTypes)];
};

}  // namespace tiny_std