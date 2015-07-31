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

        auto m = gNS->getMethod("test_method");
        if (m)
            m->invoke("Hello, World");
        m = gNS->getMethod<int>("test_method");
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

        test_cast_1();
        test_variant_1();

    } catch(const std::exception& e) {
        LOG_RED(e.what());
    } catch (...) {
        LOG_RED("Unknown exception!");
    }

    return 0;
}
