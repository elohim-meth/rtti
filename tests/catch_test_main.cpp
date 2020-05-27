#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

extern void register_rtti();

int main(int argc, char* argv[])
{
    // global setup...
    register_rtti();

    auto result = Catch::Session().run(argc, argv);
    return result;
}
