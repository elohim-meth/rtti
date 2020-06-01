#ifndef FINALLY_H
#define FINALLY_H

#include <rtti/defines.h>

#include <type_traits>
#include <utility>

namespace internal {

template<typename F>
class scope_guard final
{
public:
    scope_guard() = delete;
    scope_guard(scope_guard const&) = delete;
    scope_guard& operator=(scope_guard const&) = delete;
    scope_guard(scope_guard &&) noexcept = default;
    scope_guard& operator=(scope_guard &&) noexcept = default;
    explicit scope_guard(F const &function)
        noexcept(std::is_nothrow_copy_constructible<F>::value)
        : m_function{function}
    {}
    explicit scope_guard(F &&function)
        noexcept(std::is_nothrow_move_constructible<F>::value)
        : m_function{std::move(function)}
    {}
    ~scope_guard() noexcept
    {
        if (!m_dismiss)
            m_function();
    }
    void dismiss() noexcept
    { m_dismiss = true; }
    void* operator new(std::size_t) = delete;
private:
    bool m_dismiss = false;
    F m_function;
};

struct scope_guard_constructor
{
    template<typename F>
    using scope_guard_t = scope_guard<typename std::decay<F>::type>;

    template<typename F>
    scope_guard_t<F> operator<<(F &&function) noexcept
    {
        return scope_guard_t<F>(std::forward<F>(function));
    }
};

} // internal

#define FINALLY_EX(NAME) \
    auto NAME = ::internal::scope_guard_constructor() << [&]() noexcept

#define FINALLY \
    FINALLY_EX(CONCAT(_scopeGuard, __LINE__))

#endif // FINALLY_H
