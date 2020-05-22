#include "test_register.h"
#include "test_cast.h"
#include "test_variant.h"

#include <iostream>
#include <vector>

#include "debug.h"

int main(int argc, char* argv[])
{
    (void) argc; (void) argv;
    try {
        register_rtti();
        test_rtti_1();

        auto lambda = [](std::string_view const &name, const rtti::variant &value)
        {
            std::cout << name << " = " << value.to<std::string>() << std::endl;
            return true;
        };

        auto nsGlobal = rtti::MetaNamespace::global(); assert(nsGlobal);
        //gNS->forceDeferredDefine(rtti::MetaContainer::ForceDeferred::Recursive);
        std::cout << "namespace " << nsGlobal->name() << std::endl;
        std::cout << "Attribute count: " << nsGlobal->attributeCount() << std::endl;
        nsGlobal->for_each_attribute(lambda);
        std::cout << std::endl;

        auto nsStd = nsGlobal->getNamespace("std"); assert(nsStd);
        std::cout << "namespace " << nsStd->qualifiedName() << std::endl;
        std::cout << "Attribute count: " << nsStd->attributeCount() << std::endl;
        nsStd->for_each_attribute(lambda);
        std::cout << std::endl;

        auto nsTest = nsGlobal->getNamespace("test"); assert(nsTest);
        std::cout << "namespace " << nsTest->qualifiedName() << std::endl;
        std::cout << "Attribute count: " << nsTest->attributeCount() << std::endl;
        nsTest->for_each_attribute(lambda);
        std::cout << std::endl;

        {
            auto prop = nsGlobal->getProperty("global_string"); assert(prop);
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
            auto prop = nsGlobal->getProperty("global_readonly_string"); assert(prop);
            std::cout << prop->qualifiedName() << std::endl;
            const auto v = prop->get();
            assert(v.value<std::string>() == "Hello, World");
            try { prop->set(std::string{"Qwerty"}); assert(false);
            } catch (const rtti::runtime_error &e) { LOG_RED(e.what()); };
        }

        {
            auto itosM = nsGlobal->getMethod("intToStr"); assert(itosM);
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
                rtti::variant const ok = false;
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

        std::printf("\n");
    } catch(const std::exception& e) {
        LOG_RED(e.what());
    } catch (...) {
        LOG_RED("Unknown exception!");
    }

    return 0;
}
