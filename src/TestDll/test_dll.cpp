#include "test_dll.h"

#include <iostream>
#include <vector>

#include <debug.h>

int main(int argc, char* argv[])
{
    (void) argc; (void) argv;
    try {
//        using ttt = const void *;
//        std::cout << type_name<ttt>() << rtti::MetaType(rtti::metaTypeId<ttt>()).typeName();
//        return 0;

        test_variant_1();
        test_cast_1();

        register_rtti();

        auto lambda = [](const std::string &name, const rtti::variant &value)
        {
            std::cout << name << " = " << value.to<std::string>() << std::endl;
            return true;
        };

        auto gn = rtti::MetaNamespace::global();
        std::cout << "namespace " << gn->name() << std::endl;
        std::cout << "Attribute count: " << gn->attributeCount() << std::endl;
        gn->for_each_attribute(lambda);
        std::cout << std::endl;

        auto sn = gn->getNamespace("std");
        std::cout << "namespace " << sn->qualifiedName() << std::endl;
        std::cout << "Attribute count: " << sn->attributeCount() << std::endl;
        sn->for_each_attribute(lambda);
        std::cout << std::endl;

        auto tn = gn->getNamespace("test");
        std::cout << "namespace " << tn->qualifiedName() << std::endl;
        std::cout << "Attribute count: " << tn->attributeCount() << std::endl;
        tn->for_each_attribute(lambda);
        std::cout << std::endl;

        auto m = gn->getMethod("test_method");
        if (m)
            m->invoke("Hello, World");
        m = gn->getMethod<int>("test_method");
        if (m)
            m->invoke(256);


        auto mt = rtti::MetaType{type_name<std::vector<int>>().c_str()};
        auto vec = rtti::MetaClass::findByTypeId(mt.typeId());
        if (vec)
        {
            std::cout << vec->qualifiedName() << std::endl;
            auto construct = vec->getConstructor("range1");
            if (construct)
            {
                int a[] = {1,2,3,4,5};
                auto v = construct->invoke(std::begin(a), std::end(a));
            }
        }

        vec = tn->getClass("TestBase1");
        if (vec)
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

            auto dc = vec->defaultConstructor();
            if (dc)
            {
                std::cout << "\ndefault\n";
                auto v = dc->invoke();
                auto cc = vec->copyConstructor();
                if (cc)
                {
                    std::cout << "copy \n";
                    auto vc = cc->invoke(v);
                    auto mc = vec->moveConstructor();
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
