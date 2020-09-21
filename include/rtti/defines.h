#ifndef DEFINES_H
#define DEFINES_H

#define STRINGIZE_I(text) #text
#define STRINGIZE(text) STRINGIZE_I(text)
#define CONCAT_I(a,b) a##b
#define CONCAT(a,b) CONCAT_I(a,b)

#if defined(_MSC_VER)
    #define DISABLE_WARNINGS_PUSH __pragma(warning( push ))
    #define DISABLE_WARNINGS_POP __pragma(warning( pop ))
    #define DISABLE_WARNING(NUMBER) __pragma(warning( disable : NUMBER ))

    #define DISABLE_WARNING_UNUSED_PARAMETER DISABLE_WARNING(4100)
    #define DISABLE_WARNING_MISSING_OVERRIDE
#elif defined(__clang__)
    #define DO_PRAGMA(X) _Pragma(#X)
    #define DISABLE_WARNINGS_PUSH DO_PRAGMA(clang diagnostic push)
    #define DISABLE_WARNINGS_POP DO_PRAGMA(clang diagnostic pop)
    #define DISABLE_WARNING(NAME) DO_PRAGMA(clang diagnostic ignored #NAME)

    #define DISABLE_WARNING_UNKNOWN_WARNING DISABLE_WARNING(-Wunknown-warning-option)
    #define DISABLE_WARNING_UNUSED_PARAMETER DISABLE_WARNING(-Wunused-parameter)
    #define DISABLE_WARNING_MISSING_OVERRIDE DISABLE_WARNING(-Winconsistent-missing-override)
    #define DISABLE_WARNING_INIT_LIST_LIFETIME DISABLE_WARNING(-Winit-list-lifetime)
#elif defined(__GNUC__)
    #define DO_PRAGMA(X) _Pragma(#X)
    #define DISABLE_WARNINGS_PUSH DO_PRAGMA(GCC diagnostic push)
    #define DISABLE_WARNINGS_POP DO_PRAGMA(GCC diagnostic pop)
    #define DISABLE_WARNING(NAME) DO_PRAGMA(GCC diagnostic ignored #NAME)

    #define DISABLE_WARNING_UNKNOWN_WARNING DISABLE_WARNING(-Wunknown-warning-option)
    #define DISABLE_WARNING_UNUSED_PARAMETER DISABLE_WARNING(-Wunused-parameter)
    #define DISABLE_WARNING_MISSING_OVERRIDE DISABLE_WARNING(-Wsuggest-override)
    #define DISABLE_WARNING_INIT_LIST_LIFETIME DISABLE_WARNING(-Winit-list-lifetime)
#else
    #define DISABLE_WARNINGS_PUSH
    #define DISABLE_WARNINGS_POP

    #define DISABLE_WARNING_UNUSED_PARAMETER
    #define DISABLE_WARNING_MISSING_OVERRIDE
#endif

#define DECLARE_PRIVATE(Class) \
    inline Class##Private* d_func() { return reinterpret_cast<Class##Private *>(d_ptr.get()); } \
    inline Class##Private const* d_func() const { return reinterpret_cast<Class##Private const*>(d_ptr.get()); } \
    friend class rtti::Class##Private;

#define DECLARE_ACCESS_KEY(NAME) \
  class NAME  \
  { \
      NAME() {} \
      NAME(NAME const&) = default;

#define INVOKE_PROTECTED(OBJECT, METHOD, ...) (\
    ([&](auto &o) -> auto {\
        using T = std::remove_reference_t<decltype(o)>;\
        struct A: T\
        {\
            using T::METHOD;\
        };\
        using U = std::conditional_t<std::is_const_v<T>, A const*, A*>;\
        auto a = reinterpret_cast<U>(&o);\
        return a->METHOD(__VA_ARGS__);\
    })(OBJECT)\
)

#endif // DEFINES_H
