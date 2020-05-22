#ifndef DEBUG_H
#define DEBUG_H

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

#ifdef DEBUG
#include<cstdio>
#define PRINT_PRETTY_FUNC \
    std::printf("%s\n", __PRETTY_FUNCTION__); fflush(stdout);
#define PRINT_TYPE_NAME(V) \
    std::printf("%s\n", type_name<decltype(V)>().c_str()); fflush(stdout);
#define LOG(X) std::printf("%s\n", X); fflush(stdout);
#define LOG_RED(X) std::printf(COLOR_RED "%s\n" COLOR_RESET, X); fflush(stdout);
#else
#define PRINT_PRETTY_FUNC
#define PRINT_TYPE_NAME(V)
#define LOG
#define LOG_RED
#endif



#endif // DEBUG_H

