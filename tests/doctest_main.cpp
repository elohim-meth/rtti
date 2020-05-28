#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

extern void register_rtti();

int main(int argc, char* argv[])
{
    // global setup...
    register_rtti();

    auto result = doctest::Context(argc, argv).run();
    return result;
}
