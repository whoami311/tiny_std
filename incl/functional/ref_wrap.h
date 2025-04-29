/**
 * @file ref_wrap.h
 * @author whoami (13003827890@163.com)
 * @brief
 * @version 0.1
 * @date 2025-04-30
 *
 * @copyright Copyright (c) 2025
 *
 */

#include <functional>

namespace tiny_std {

/**
 *  Helper for defining adaptable unary function objects.
 *  @deprecated Deprecated in C++11, no longer in the standard since C++17.
 */
template <typename Arg, typename Result>
struct unary_function {
    /// @c argument_type is the type of the argument
    using argument_type = Arg;

    /// @c result_type is the return type
    using result_type = Result;
};

/**
 *  Helper for defining adaptable binary function objects.
 *  @deprecated Deprecated in C++11, no longer in the standard since C++17.
 */
template <typename Arg1, typename Arg2, typename Result>
struct binary_function {
    /// @c first_argument_type is the type of the first argument
    using first_argument_type = Arg1;

    /// @c second_argument_type is the type of the second argument
    using second_argument_type = Arg2;

    /// @c result_type is the return type
    using result_type = Result;
};
/** @}  */

/**
 * Derives from @c unary_function or @c binary_function, or perhaps
 * nothing, depending on the number of arguments provided. The
 * primary template is the basis case, which derives nothing.
 */
template <typename Res, typename... ArgTypes>
struct MaybeUnaryOrBinaryFunction {};

/// Derives from @c unary_function, as appropriate.
template <typename Res, typename T1>
struct MaybeUnaryOrBinaryFunction<Res, T1> : unary_function<T1, Res> {};

/// Derives from @c binary_function, as appropriate.
template <typename Res, typename T1, typename _T2>
struct MaybeUnaryOrBinaryFunction<Res, T1, _T2> : binary_function<T1, _T2, Res> {};

}  // namespace tiny_std