#ifndef GLOBAL_H
#define GLOBAL_H

#if defined(BUILD_SHARED)
  #if defined (_WIN32) || defined (__CYGWIN__)
    #if defined(BUILD_DLL)
      #ifdef __GNUC__
        #define DLL_PUBLIC __attribute__ ((dllexport))
      #else
        #define DLL_PUBLIC __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
      #endif
    #else
      #ifdef __GNUC__
        #define DLL_PUBLIC __attribute__ ((dllimport))
      #else
        #define DLL_PUBLIC __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
      #endif
    #endif
    #define DLL_LOCAL
  #else
    #if __GNUC__ >= 4
      #define DLL_PUBLIC __attribute__ ((visibility ("default")))
      #define DLL_LOCAL  __attribute__ ((visibility ("hidden")))
    #else
      #define DLL_PUBLIC
      #define DLL_LOCAL
    #endif
  #endif
#else
  #define DLL_PUBLIC
  #define DLL_LOCAL
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
        struct: T\
        {\
            using T::METHOD;\
        } *a = reinterpret_cast<decltype(a)>(&o);\
        return a->METHOD(__VA_ARGS__);\
    })(OBJECT)\
)

#endif // GLOBAL_H
