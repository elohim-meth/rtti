#include "test_dll.h"

#include <iostream>
#include <vector>

#include <debug.h>

int main(int argc, char* argv[])
{
    (void) argc; (void) argv;
    try {
        test_variant_1();

        register_rtti();


        auto lambda = [](const std::string &name, const rtti::variant &value)
        {
            std::cout << name << std::endl;
            (void) value;
        };

        auto g = rtti::MetaNamespace::global();
        std::cout << "namespace " << (g->isGlobal() ? "<global>" : "error") << std::endl;
        std::cout << "Attribute count: " << g->attributeCount() << std::endl;
        g->for_each_attribute(lambda);
        std::cout << std::endl;

        auto s = g->getNamespace("std");
        std::cout << "namespace " << (s->isGlobal() ? "error" : s->qualifiedName().c_str()) << std::endl;
        std::cout << "Attribute count: " << s->attributeCount() << std::endl;
        s->for_each_attribute(lambda);
        std::cout << std::endl;

        auto c = rtti::MetaClass::findByTypeId(rtti::metaTypeId<std::vector<int>>());
        if (c)
            std::cout << c->qualifiedName() << std::endl;

        auto t = g->getNamespace("test");
        c = t->getClass("TestBase1");
        if (c)
        {
//            auto e = c->getEnum("TestEnum");
//            if (e)
//            {
//                std::cout << e->qualifiedName() << std::endl;
//                e->for_each_element([](const std::string &name, const rtti::variant &value)
//                {
//                    std::cout << name << " = " << rtti::variant_cast<TestBase1::TestEnum>(value) << std::endl;
//                });
//            }

//            std::cout << std::endl;

//            e = c->getEnum("Color");
//            if (e)
//            {
//                std::cout << e->qualifiedName() << std::endl;
//                e->for_each_element([](const std::string &name, const rtti::variant &value)
//                {
//                    std::cout << name << " = " << static_cast<typename std::underlying_type<TestBase1::Color>::type>(
//                                     rtti::variant_cast<TestBase1::Color>(value)) << std::endl;
//                });
//            }

            auto dc = c->defaultConstructor();
            if (dc)
            {
                std::cout << "\ndefault\n";
                auto v = dc->invoke();
                auto cc = c->copyConstructor();
                if (cc)
                {
                    std::cout << "copy \n";
                    auto vc = cc->invoke(v);
                    auto mc = c->moveConstructor();
                    if (mc)
                    {
                        std::cout << "move \n";
                        auto vm = mc->invoke(std::move(vc));
                    }
                }
            }
        }

    } catch(const std::exception& e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        auto e = std::current_exception();
        std::cout << "e.what()" << std::endl;
    }

    return 0;
}
