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

/**
 *  Trait identifying "location-invariant" types, meaning that the
 *  address of the object (or any of its members) will not escape.
 *  Trivially copyable types are location-invariant and users can
 *  specialize this trait for other types.
 */
template <typename Tp>
struct IsLocationInvariant : std::is_trivially_copyable<Tp>::type {};

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
    static const size_t max_size_ = sizeof(NocopyTypes);
    static const size_t max_align_ = __alignof__(NocopyTypes);

    template <typename Functor>
    class BaseManager {
    protected:
        static const bool stored_locally_ =
            (IsLocationInvariant<Functor>::value && sizeof(Functor) <= max_size_ &&
             __alignof__(Functor) <= max_align_ && (max_align_ % __alignof__(Functor) == 0));

        using LocalStorage = std::integral_constant<bool, stored_locally_>;

        // Retrieve a pointer to the function object
        static Functor* GetPointer(const AnyData& source) noexcept {
            if constexpr (stored_locally_) {
                const Functor& f = source.Access<Functor>();
                return const_cast<Functor*>(std::__addressof(f));
            } else  // have stored a pointer
                return source.Access<Functor*>();
        }

    private:
        // Construct a location-invariant function object that fits within
        // an AnyData structure.
        template <typename Fn>
        static void Create(AnyData& dest, Fn&& f, std::true_type) {
            ::new (dest.Access()) Functor(std::forward<Fn>(f));
        }

        // Construct a function object on the heap and store a pointer.
        template <typename Fn>
        static void Create(AnyData& dest, Fn&& f, std::false_type) {
            dest.Access<Functor*>() = new Functor(std::forward<Fn>(f));
        }

        // Destroy an object stored in the internal buffer.
        static void Destroy(AnyData& victim, std::true_type) {
            victim.Access<Functor>().~Functor();
        }

        // Destroy an object located on the heap.
        static void Destroy(AnyData& victim, std::false_type) {
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
                case DESTROY_FUNCTOR:
                    Destroy(dest, LocalStorage());
                    break;
            }
            return false;
        }

        template <typename Fn>
        static void InitFunctor(AnyData& functor, Fn&& f) noexcept(
            std::__and_<LocalStorage, std::is_nothrow_constructible<Functor, Fn>>::value) {
            Create(functor, std::forward<Fn>(f), LocalStorage());
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

    template <typename Fn>
    static constexpr bool NoThrowInit() noexcept {
        return std::__and_<typename Base::LocalStorage, std::is_nothrow_constructible<Functor, Fn>>::value;
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
// in std::function<Signature>::target<Functor>().
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

    template <typename Func, typename DFunc = Decay<Func>, typename Res2 = std::invoke_result<DFunc&, ArgTypes...>>
    struct Callable : std::is_invocable<Res2, Res>::type {};

    template <typename Cond, typename Tp = void>
    using Requires = std::__enable_if_t<Cond::value, Tp>;

    template <typename Functor>
    using Handler = FunctionHandler<Res(ArgTypes...), std::__decay_t<Functor>>;

public:
    using result_type = Res;

    // [3.7.2.1] construct/copy/destroy

    /**
     *  @brief Default construct creates an empty function call wrapper.
     *  @post `!(bool)*this`
     */
    function() noexcept : FunctionBase() {}

    /**
     *  @brief Creates an empty function call wrapper.
     *  @post @c !(bool)*this
     */
    function(nullptr_t) noexcept : FunctionBase() {}

    /**
     *  @brief %Function copy constructor.
     *  @param x A %function object with identical call signature.
     *  @post `bool(*this) == bool(x)`
     *
     *  The newly-created %function contains a copy of the target of
     *  `x` (if it has one).
     */
    function(const function& x) : FunctionBase() {
        if (static_cast<bool>(x)) {
            x.Manager(functor_, x.functor_, CLONE_FUNCTOR);
            invoker_ = x.invoker_;
            manager_ = x.manager_;
        }
    }

    /**
     *  @brief %Function move constructor.
     *  @param x A %function object rvalue with identical call signature.
     *
     *  The newly-created %function contains the target of `x`
     *  (if it has one).
     */
    function(function&& x) noexcept : FunctionBase(), invoker_(x.invoker_) {
        if (static_cast<bool>(x)) {
            functor_ = x.functor_;
            manager_ = x.manager_;
            x.manager_ = nullptr;
            x.invoker_ = nullptr;
        }
    }

    /**
     *  @brief Builds a %function that targets a copy of the incoming
     *  function object.
     *  @param f A %function object that is callable with parameters of
     *  type `ArgTypes...` and returns a value convertible to `Res`.
     *
     *  The newly-created %function object will target a copy of
     *  `f`. If `f` is `reference_wrapper<F>`, then this function
     *  object will contain a reference to the function object `f.get()`.
     *  If `f` is a null function pointer, null pointer-to-member, or
     *  empty `std::function`, the newly-created object will be empty.
     *
     *  If `f` is a non-null function pointer or an object of type
     *  `reference_wrapper<F>`, this function will not throw.
     */
    // 2774. std::function construction vs assignment
    template <typename Functor, typename Constraints = Requires<Callable<Functor>>>
    function(Functor&& f) noexcept(Handler<Functor>::template NothrowInit<Functor>()) : FunctionBase() {
        static_assert(std::is_copy_constructible<std::__decay_t<Functor>>::value,
                      "std::function target must be copy-constructible");
        static_assert(std::is_constructible<std::__decay_t<Functor>, Functor>::value,
                      "std::function target must be constructible from the constructor argument");

        using MyHandler = Handler<Functor>;

        if (MyHandler::NotEmptyFunction(f)) {
            MyHandler::InitFunctor(functor_, std::forward<Functor>(f));
            invoker_ = &MyHandler::invoke_;
            manager_ = &MyHandler::manager_;
        }
    }

    /**
     *  @brief Function assignment operator.
     *  @param x A %function with identical call signature.
     *  @post `(bool)*this == (bool)x`
     *  @returns `*this`
     *
     *  The target of `x` is copied to `*this`. If `x` has no
     *  target, then `*this` will be empty.
     *
     *  If `x` targets a function pointer or a reference to a function
     *  object, then this operation will not throw an exception.
     */
    function& operator=(const function& x) {
        function(x).swap(*this);
        return *this;
    }

    /**
     *  @brief Function move-assignment operator.
     *  @param x A %function rvalue with identical call signature.
     *  @returns `*this`
     *
     *  The target of `x` is moved to `*this`. If `x` has no
     *  target, then `*this` will be empty.
     *
     *  If `x` targets a function pointer or a reference to a function
     *  object, then this operation will not throw an exception.
     */
    function& operator=(function&& x) noexcept {
        function(std::move(x)).swap(*this);
        return *this;
    }

    /**
     *  @brief Function assignment to empty.
     *  @post `!(bool)*this`
     *  @returns `*this`
     *
     *  The target of `*this` is deallocated, leaving it empty.
     */
    function& operator=(nullptr_t) noexcept {
        if (manager_) {
            manager_(functor_, functor_, DESTROY_FUNCTOR);
            manager_ = nullptr;
            invoker_ = nullptr;
        }
        return *this;
    }

    /**
     *  @brief Function assignment to a new target.
     *  @param f  A function object that is callable with parameters of
     *              type  `ArgTypes...` and returns a value convertible
     *              to `Res`.
     *  @return `*this`
     *  @since C++11
     *
     *  This function object wrapper will target a copy of `f`. If `f`
     *  is `reference_wrapper<F>`, then this function object will contain
     *  a reference to the function object `f.get()`. If `f` is a null
     *  function pointer or null pointer-to-member, this object will be
     *  empty.
     *
     *  If `f` is a non-null function pointer or an object of type
     *  `reference_wrapper<F>`, this function will not throw.
     */
    template <typename Functor>
    Requires<Callable<Functor>, function&> operator=(Functor&& f) noexcept(
        Handler<Functor>::template NothrowInit<Functor>()) {
        function(std::forward<Functor>(f)).swap(*this);
        return *this;
    }

    /// @overload
    template <typename Functor>
    function& operator=(std::reference_wrapper<Functor> f) noexcept {
        function(f).swap(*this);
        return *this;
    }

    // [3.7.2.2] function modifiers

    /**
     *  @brief Swap the targets of two %function objects.
     *  @param x A %function with identical call signature.
     *
     *  Swap the targets of `this` function object and `f`.
     *  This function will not throw exceptions.
     */
    void swap(function& x) noexcept {
        std::swap(functor_, x.functor_);
        std::swap(manager_, x.manager_);
        std::swap(invoker_, x.invoker_);
    }

    // [3.7.2.3] function capacity

    /**
     *  @brief Determine if the %function wrapper has a target.
     *
     *  @return `true` when this function object contains a target,
     *  or `false` when it is empty.
     *
     *  This function will not throw exceptions.
     */
    explicit operator bool() const noexcept {
        return !Empty();
    }

    // [3.7.2.4] function invocation

    /**
     *  @brief Invokes the function targeted by `*this`.
     *  @returns the result of the target.
     *
     *  The function call operator invokes the target function object
     *  stored by `this`.
     */
    Res operator()(ArgTypes... args) const {
        return invoker_(functor_, std::forward<ArgTypes>(args)...);
    }

    // [3.7.2.5] function target access
    /**
     *  @brief Determine the type of the target of this function object
     *  wrapper.
     *
     *  @returns the type identifier of the target function object, or
     *  `typeid(void)` if `!(bool)*this`.
     *
     *  This function will not throw exceptions.
     */
    const std::type_info& target_type() const noexcept {
        if (manager_) {
            AnyData typeinfo_result;
            manager_(typeinfo_result, functor_, GET_TYPE_INFO);
            if (auto ti = typeinfo_result.Access<const std::type_info*>())
                return *ti;
        }
        return typeid(void);
    }

    /**
     *  @brief Access the stored target function object.
     *
     *  @return Returns a pointer to the stored target function object,
     *  if `typeid(Functor).equals(target_type())`; otherwise, a null
     *  pointer.
     *
     * This function does not throw exceptions.
     *
     * @{
     */
    template <typename Functor>
    Functor* target() noexcept {
        const function* const_this = this;
        const Functor* func = const_this->template target<Functor>();
        // If is_function_v<Functor> is true then const_cast<Functor*>
        // would be ill-formed, so use *const_cast<Functor**> instead.
        return *const_cast<Functor**>(&func);
    }

    template <typename Functor>
    const Functor* target() const noexcept {
        if constexpr (std::is_object<Functor>::value) {
            // For C++11 and C++14 if-constexpr is not used above, so
            // TargetHandler avoids ill-formed _Function_handler types.
            using Handler = TargetHandler<Res(ArgTypes...), Functor>;

            if (manager_ == &Handler::manager_ || (manager_ && typeid(Functor) == target_type())) {
                AnyData ptr;
                manager_(ptr, functor_, GET_FUNCTOR_PTR);
                return ptr.Access<const Functor*>();
            }
        }
        return nullptr;
    }
    /// @}

private:
    using InvokerType = Res (*)(const AnyData&, ArgTypes&&...);
    InvokerType invoker_ = nullptr;
};

template <typename>
struct FunctionGuideHelper {};

template <typename Res, typename Tp, bool Nx, typename... Args>
struct FunctionGuideHelper<Res (Tp::*)(Args...) noexcept(Nx)> {
    using type = Res(Args...);
};

template <typename Res, typename Tp, bool Nx, typename... Args>
struct FunctionGuideHelper<Res (Tp::*)(Args...) & noexcept(Nx)> {
    using type = Res(Args...);
};

template <typename Res, typename Tp, bool Nx, typename... Args>
struct FunctionGuideHelper<Res (Tp::*)(Args...) const noexcept(Nx)> {
    using type = Res(Args...);
};

template <typename Res, typename Tp, bool Nx, typename... Args>
struct FunctionGuideHelper<Res (Tp::*)(Args...) const & noexcept(Nx)> {
    using type = Res(Args...);
};

template <typename Fn, typename Op>
using FunctionGuide = typename FunctionGuideHelper<Op>::type;

template <typename Res, typename... ArgTypes>
function(Res (*)(ArgTypes...)) -> function<Res(ArgTypes...)>;

template <typename Fn, typename Signature = FunctionGuide<Fn, decltype(&Fn::operator())>>
function(Fn) -> function<Signature>;

// [20.7.15.2.6] null pointer comparisons

/**
 *  @brief Test whether a polymorphic function object wrapper is empty.
 *  @returns `true` if the wrapper has no target, `false` otherwise
 *
 *  This function will not throw exceptions.
 */
template <typename Res, typename... Args>
inline bool operator==(const function<Res(Args...)>& f, nullptr_t) noexcept {
    return !static_cast<bool>(f);
}

/// @overload
template <typename Res, typename... Args>
inline bool operator==(nullptr_t, const function<Res(Args...)>& f) noexcept {
    return !static_cast<bool>(f);
}

/**
 *  @brief Test whether a polymorphic function object wrapper is non-empty.
 *  @returns `false` if the wrapper has no target, `true` otherwise
 *
 *  This function will not throw exceptions.
 */
template <typename Res, typename... Args>
inline bool operator!=(const function<Res(Args...)>& f, nullptr_t) noexcept {
    return static_cast<bool>(f);
}

/// @overload
template <typename Res, typename... Args>
inline bool operator!=(nullptr_t, const function<Res(Args...)>& f) noexcept {
    return static_cast<bool>(f);
}

// [20.7.15.2.7] specialized algorithms

/**
 *  @brief Swap the targets of two polymorphic function object wrappers.
 *
 *  This function will not throw exceptions.
 */
template <typename Res, typename... Args>
inline void swap(function<Res(Args...)>& x, function<Res(Args...)>& y) noexcept {
    x.swap(y);
}

}  // namespace tiny_std
