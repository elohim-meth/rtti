#include "test_dll.h"

#include <iostream>
#include <vector>

#include <debug.h>

class TestBase1 {
public:
    TestBase1() { PRINT_PRETTY_FUNC }
//    TestBase1(const TestBase1&) { PRINT_PRETTY_FUNC }
//    TestBase1(TestBase1&&) { PRINT_PRETTY_FUNC }
    TestBase1(int, const std::string&){ PRINT_PRETTY_FUNC }
    virtual ~TestBase1(){ PRINT_PRETTY_FUNC }
    enum TestEnum {
        te1 = 10,
        te2 = 20
    };

    enum class Color {
        Red,
        Green,
        Blue
    };
};
class TestBase2 {};
class TestDerived: public TestBase1, public TestBase2 {};

template<typename T>
void define_std_vector(rtti::meta_define<std::vector<T>> define)
{
    define
        ._attribute("1", "Standard vector<T>")
    ;
}

template<typename T>
void register_std_vector()
{
    rtti::global_define()
        ._namespace("std")
            ._class<std::vector<T>>("vector<" + type_name<T>() + ">")
                ._lazy(&define_std_vector<T>)
            ._end()
        ._end()
    ;
}

void test_register()
{
    rtti::global_define()
        ._attribute("Attribute 1", 100)
        ._attribute("Attribute 1", 200)
        ._namespace("std")._lazy([](rtti::meta_define<void> define)
            {
                define
                    ._attribute("Attribute 2", 300)
                    ._class<std::string>("string")
                    ._end()
                ;
            })
        ._end()
        ._attribute("Attribute 3", std::string{"Hello, World!"})
        ._class<TestBase1>("TestBase1")
            ._enum<TestBase1::TestEnum>("TestEnum")
                ._element("te1", TestBase1::te1)
                ._element("te2", TestBase1::te2)
            ._enum<TestBase1::Color>("Color")
                ._element("Red", TestBase1::Color::Red)
                ._element("Green", TestBase1::Color::Green)
                ._element("Blue", TestBase1::Color::Blue)
            ._constructor()
            ._constructor<const TestBase1&>()
        ._end()
        ._class<TestBase2>("TestBase2")
            ._constructor()
        ._end()
        ._class<TestDerived>("TestDerived")
            ._base<TestBase1, TestBase2>()
        ._end()
    ;
}

int main(int argc, char* argv[])
{
    (void) argc; (void) argv;
    try {
        test_register();
        register_std_vector<int>();
        register_std_vector<std::string>();
        register_std_vector<const char*>();

        auto lambda = [](const std::string &name, const rtti::variant &value)
        {
            std::cout << name << std::endl;
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
            auto e = c->getEnum("TestEnum");
            if (e)
            {
                std::cout << e->qualifiedName() << std::endl;
                e->for_each_element([](const std::string &name, const rtti::variant &value)
                {
                    std::cout << name << " = " << rtti::variant_cast<TestBase1::TestEnum>(value) << std::endl;
                });
            }

            std::cout << std::endl;

            e = c->getEnum("Color");
            if (e)
            {
                std::cout << e->qualifiedName() << std::endl;
                e->for_each_element([](const std::string &name, const rtti::variant &value)
                {
                    std::cout << name << " = " << static_cast<typename std::underlying_type<TestBase1::Color>::type>(
                                     rtti::variant_cast<TestBase1::Color>(value)) << std::endl;
                });
            }

            std::cout << "\ndefault\n";
            auto dc = c->defaultConstructor();
            if (dc)
            {
                auto v = dc->invoke();
                std::cout << "copy \n";
                auto cc = c->copyConstructor();
                if (cc)
                {
                    auto vc = cc->invoke(v);
                    std::cout << "move \n";
                    auto mc = c->moveConstructor();
                    if (mc)
                    {
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
