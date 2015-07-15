#include "test_dll.h"

#include <iostream>
#include <vector>

#include <debug.h>

//void test_register()
//{
//    rtti::global_define()
//        ._attribute("Attribute 1", 100)
//        ._attribute("Attribute 1", 200)
//        ._namespace("std")._lazy([](rtti::meta_define<void> define)
//            {
//                define
//                    ._attribute("Attribute 2", 300)
//                    ._class<std::string>("string")
//                    ._end()
//                ;
//            })
//        ._end()
//        ._attribute("Attribute 3", std::string{"Hello, World!"})
//        ._class<TestBase1>("TestBase1")
//            ._enum<TestBase1::TestEnum>("TestEnum")
//                ._element("te1", TestBase1::te1)
//                ._element("te2", TestBase1::te2)
//            ._enum<TestBase1::Color>("Color")
//                ._element("Red", TestBase1::Color::Red)
//                ._element("Green", TestBase1::Color::Green)
//                ._element("Blue", TestBase1::Color::Blue)
//            ._constructor<int, const std::string&>()
//        ._end()
//        ._class<TestBase2>("TestBase2")
//        ._end()
//        ._class<TestDerived>("TestDerived")
//            ._base<TestBase1, TestBase2>()
//        ._end()
//    ;
//}




int main(int argc, char* argv[])
{
    (void) argc; (void) argv;
    try {
        test_variant_1();

//        test_register();
        register_std_vector<int>();
//        register_std_vector<std::string>();
//        register_std_vector<const char*>();

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

        c = g->getClass("TestBase1");
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
