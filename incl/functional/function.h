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

#include <type_traits>

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

enum ManagerOperation {
    GET_TYPE_INFO,
    GET_FUNCTOR_PTR,
    CLONE_FUNCTOR,
    DESTROY_FUNCTOR
};

template <typename Signature>
class function;

/// Base class of all polymorphic function object wrappers.
class FunctionBase {
public:
    template <typename Functor>
    class BaseManager {
    protected:
        // Retrieve a pointer to the function object
        static Functor* GetPointer(const AnyData& source) noexcept {
            return source.Access<Functor*>();
        }

    private:
        // Construct a function object on the heap and store a pointer.
        template <typename Fn>
        static void Create(AnyData& dest, Fn&& f) {
            dest.Access<Functor*>() = new Functor(std::forward<Fn>(f));
        }

        // Destroy an object located on the heap.
        static void _M_destroy(AnyData& victim) {
            delete victim.Access<Functor*>();
        }

    public:
        static bool Manager(AnyData& dest, const AnyData& source, ManagerOperation op) {
            switch (op) {
                case GET_TYPE_INFO:
                    dest.Access<const std::type_info*>() = &typeid(Functor);
                    break;
                case GET_FUNCTOR_PTR:
                    dest.Access<Functor*>() = GetPointer(source);
                    break;
                case CLONE_FUNCTOR:
                    InitFunctor(dest, *const_cast<const Functor*>(GetPointer(source)));
                    break;
            }
            return false;
        }

        template <typename Fn>
        static void InitFunctor(AnyData& functor, Fn&& f) {
            Create(functor, std::forward<Fn>(f));
        }

        template <typename Signature>
        static bool NotEmptyFunction(const function<Signature>& f) noexcept {
            return static_cast<bool>(f);
        }

        template <typename Tp>
        static bool NotEmptyFunction(Tp* fp) noexcept {
            return fp != nullptr;
        }

        template <typename Class, typename Tp>
        static bool NotEmptyFunction(Tp Class::* mp) noexcept {
            return mp != nullptr;
        }

        template <typename Tp>
        static bool NotEmptyFunction(const Tp&) noexcept {
            return true;
        }
    };

    FunctionBase() = default;
    ~FunctionBase() {
        if (manager_)
            manager_(functor_, functor_, DESTROY_FUNCTOR);
    }

    bool Empty() const {
        return !manager_;
    }

    using ManagerType = bool (*)(AnyData&, const AnyData&, ManagerOperation);

    AnyData functor_{};
    ManagerType manager_{};
};

template <typename Signature, typename Functor>
class FunctionHandler;

template <typename Res, typename Functor, typename... ArgTypes>
class FunctionHandler<Res(ArgTypes...), Functor> : public FunctionBase::BaseManager<Functor> {
    using Base = FunctionBase::BaseManager<Functor>;

public:
    static bool Manager(AnyData& dest, const AnyData& source, ManagerOperation op) {
        switch (op) {
            case GET_TYPE_INFO:
                dest.Access<const std::type_info*>() = &typeid(Functor);
                break;
            case GET_FUNCTOR_PTR:
                dest.Access<Functor*>() = Base::GetPointer(source);
                break;
            default:
                Base::Manager(dest, source, op);
        }
        return false;
    }

    static Res Invoke(const AnyData& functor, ArgTypes&&... args) {
        return std::__invoke_r<Res>(*Base::GetPointer(functor), std::forward<ArgTypes>(args)...);
    }
};

// Specialization for invalid types
template <>
class FunctionHandler<void, void> {
public:
    static bool Manager(AnyData&, const AnyData&, ManagerOperation) {
        return false;
    }
};

// Avoids instantiating ill-formed specializations of _Function_handler
// in std::function<_Signature>::target<_Functor>().
// e.g. _Function_handler<Sig, void()> and _Function_handler<Sig, void>
// would be ill-formed.
template <typename Signature, typename Functor, bool valid = std::is_object<Functor>::value>
struct TargetHandler : FunctionHandler<Signature, typename std::remove_cv<Functor>::type> {};

template <typename Signature, typename Functor>
struct TargetHandler<Signature, Functor, false> : FunctionHandler<void, void> {};

/**
 *  @brief Polymorphic function wrapper.
 *  @ingroup functors
 *  @since C++11
 */
template <typename Res, typename... ArgTypes>
class function<Res(ArgTypes...)> : private FunctionBase {
    // Equivalent to std::decay_t except that it produces an invalid type
    // if the decayed type is the current specialization of std::function.
    template <typename Func, bool Self = std::is_same<std::__remove_cvref_t<Func>, function>::value>
    using Decay = typename std::enable_if<!Self, std::decay<Func>>::type;

    template <typename Func, typename DFunc = Decay<Func>,
    typename Res2 = std::invoke_result<DFunc&, ArgTypes...>>
    struct Callable : std::is_invocable<Res2, Res>::type {};
};
}  // namespace tiny_std