#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

extern void register_global_ns();
extern void register_std_ns();

int main(int argc, char* argv[])
{
    // global setup...
    register_global_ns();
    register_std_ns();

    auto result = doctest::Context(argc, argv).run();
    return result;
}
