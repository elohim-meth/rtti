#include "test_dll.h"

#include <iostream>
#include <vector>

#include <debug.h>

int main(int argc, char* argv[])
{
    (void) argc; (void) argv;
    try {
        register_rtti();
        auto lambda = [](const std::string &name, const rtti::variant &value)
        {
            std::cout << name << " = " << value.to<std::string>() << std::endl;
            return true;
        };

        auto gNS = rtti::MetaNamespace::global(); assert(gNS);
        //gNS->forceDeferredDefine(rtti::MetaContainer::ForceDeferred::Recursive);
        std::cout << "namespace " << gNS->name() << std::endl;
        std::cout << "Attribute count: " << gNS->attributeCount() << std::endl;
        gNS->for_each_attribute(lambda);
        std::cout << std::endl;

        auto stdNS = gNS->getNamespace("std"); assert(stdNS);
        std::cout << "namespace " << stdNS->qualifiedName() << std::endl;
        std::cout << "Attribute count: " << stdNS->attributeCount() << std::endl;
        stdNS->for_each_attribute(lambda);
        std::cout << std::endl;

        auto testNS = gNS->getNamespace("test"); assert(testNS);
        std::cout << "namespace " << testNS->qualifiedName() << std::endl;
        std::cout << "Attribute count: " << testNS->attributeCount() << std::endl;
        testNS->for_each_attribute(lambda);
        std::cout << std::endl;

        {
            auto prop = gNS->getProperty("global_string"); assert(prop);
            std::cout << prop->qualifiedName() << std::endl;
            prop->set(std::string{"Qwerty"});
            assert(prop->get().cvalue<std::string>() == "Qwerty");
            assert(prop->get().value<const std::string>() == "Qwerty");

            const auto v = prop->get();
            assert(v.value<std::string>() == "Qwerty");
            prop->set(std::string{"YouTube"});
            assert(v.value<std::string>() == "YouTube");
        }

        {
            auto prop = gNS->getProperty("global_readonly_string"); assert(prop);
            std::cout << prop->qualifiedName() << std::endl;
            const auto v = prop->get();
            assert(v.value<std::string>() == "Hello, World");
            try { prop->set(std::string{"Qwerty"}); assert(false);
            } catch (const rtti::runtime_error &e) { LOG_RED(e.what()); };
        }

        {
            auto itosM = gNS->getMethod("intToStr"); assert(itosM);
            {
                bool ok = false;
                auto r = itosM->invoke(123, ok);
                assert(r.value<std::string>() == "123" && ok);
            }

            {
                const bool ok = false;
                try { auto r = itosM->invoke(123, ok); assert(false);
                } catch (const rtti::runtime_error &e) { LOG_RED(e.what()); };
            }

            {
                try { auto r = itosM->invoke(123, false); assert(false);
                } catch (const rtti::runtime_error &e) { LOG_RED(e.what()); };
            }

            {
                rtti::variant ok = false;
                auto r = itosM->invoke(123, ok);
                assert(r.value<std::string>() == "123" && ok.value<bool>());
            }

            {
                const rtti::variant ok = false;
                try { auto r = itosM->invoke(123, ok); assert(false);
                } catch (const rtti::runtime_error &e) { LOG_RED(e.what()); };
            }

            {
                bool ok = false;
                rtti::variant vok = std::ref(ok);
                auto r = itosM->invoke(123, vok);
                assert(r.value<std::string>() == "123" && ok && vok.value<bool>());
            }

            {
                const bool ok = false;
                rtti::variant vok = std::ref(ok);
                try { auto r = itosM->invoke(123, vok); assert(false);
                } catch (const rtti::runtime_error &e) { LOG_RED(e.what()); };
            }

            {
                rtti::variant ok = false;
                try { auto r = itosM->invoke(123, std::move(ok)); assert(false);
                } catch (const rtti::runtime_error &e) { LOG_RED(e.what()); };
            }

        }

        test_cast_1();
        test_variant_1();

        std::printf("/n");
    } catch(const std::exception& e) {
        LOG_RED(e.what());
    } catch (...) {
        LOG_RED("Unknown exception!");
    }

    return 0;
}
