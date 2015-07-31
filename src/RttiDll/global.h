#ifndef GLOBAL_H
#define GLOBAL_H

#ifndef __GNUC__
static_assert(false, "GCC is the only supported compiler");
#else
  #if __GNUC__ < 4 && __GNUC_MINOR__ < 9 && __GNUC_PATCHLEVEL__ < 2
    static_assert(false, "GCC 4.9.2 is minimum compiler version");
  #endif
#endif

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

#endif // GLOBAL_H
