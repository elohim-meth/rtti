#include "test_dll.h"

#include <iostream>
#include <vector>

#include <debug.h>

int main(int argc, char* argv[])
{
    (void) argc; (void) argv;
    try {
//        std::cout << std::boolalpha << rtti::has_move_constructor<temp>::value;
//        temp t1;
//        temp t2 = std::move(t1);
//        return 0;

        register_rtti();

        test_cast_1();
        test_variant_1();

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

    } catch(const std::exception& e) {
        LOG_RED(e.what());
    } catch (...) {
        LOG_RED("Unknown exception!");
    }

    return 0;
}
