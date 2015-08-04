# rtti
A try to add runtime reflection to C++ language.

Implemented meta namespaces, classes, methods, constructors, properties and enums.

Library contains generic variant type. 
Is's capable of holding any type inluding references (used std::reference_wrapper).
It supports polymorphic conversion using rtti_cast, when holding references or pointers to registered classes.
It also supports user defined conversion, registered through metatype system.

Library supports for meta_cast replacement of dynamic_cast. 

Invoking of methods, constructors and properties respect const correctness of parameters and methods.

Rtti library can be compiled as static or shared. Shared method is preferred since library contains
global static containers with various type information and they better to be in one place.

#### Usage
This example is taken from one of tests:

```C++
#define PRINT_PRETTY_FUNC \
    std::printf("%s\n", __PRETTY_FUNCTION__);

namespace test {

class A
{
    DECLARE_CLASSINFO
public:
    A()
    { PRINT_PRETTY_FUNC }

    explicit A(int value)
        : a(value)
    {
        PRINT_PRETTY_FUNC
    }

    A(const A &other)
        : a(other.a)
    {
        PRINT_PRETTY_FUNC
    }

    A(A &&other) noexcept
    {
        PRINT_PRETTY_FUNC
        std::swap(a, other.a);
    }

    A& operator=(const A &other)
    {
        PRINT_PRETTY_FUNC
        if (this != &other)
            A{other}.swap(*this);
        return *this;
    }

    A& operator=(A &&other) noexcept
    {
        PRINT_PRETTY_FUNC
        if (this != &other)
            A{std::move(other)}.swap(*this);
        return *this;
    }

    void swap(A &other) noexcept
    {
        std::swap(a, other.a);
    }


    virtual ~A() noexcept
    {
        PRINT_PRETTY_FUNC
        a = -1;
    }

    virtual void print() const noexcept
    {
        PRINT_PRETTY_FUNC
        std::printf("a = %d\n", a);
    }

    int getA() const
    {
        PRINT_PRETTY_FUNC
        return a;
    }

    void setA(int value)
    {
        PRINT_PRETTY_FUNC
        a = value;
    }

    std::string overload_on_const(int value) const
    {
        PRINT_PRETTY_FUNC
        return std::to_string(value);
    }

    std::string overload_on_const(int value)
    {
        PRINT_PRETTY_FUNC
        a = value;
        return std::to_string(value);
    }

private:
    int a = -1;
};

class B: public A
{
    DECLARE_CLASSINFO
public:
    B() : A()
    { PRINT_PRETTY_FUNC }

    explicit B(int value)
        : A{value}, b{value}
    {
        PRINT_PRETTY_FUNC
    }

    B(const B &other)
        : A{other}, b{other.b}
    {
        PRINT_PRETTY_FUNC
    }

    B(B &&other) noexcept
        : A{std::move(other)}
    {
        PRINT_PRETTY_FUNC
        std::swap(b, other.b);
    }

    B& operator=(const B &other)
    {
        PRINT_PRETTY_FUNC
        if (this != &other)
            B{other}.swap(*this);
        return *this;
    }

    B& operator=(B &&other) noexcept
    {
        PRINT_PRETTY_FUNC
        if (this != &other)
            B{std::move(other)}.swap(*this);
        return *this;
    }

    void swap(B &other) noexcept
    {
        A::swap(other);
        std::swap(b, other.b);
    }


    virtual ~B() noexcept
    {
        PRINT_PRETTY_FUNC
        b = -1;
    }

    void print() const noexcept override
    {
        PRINT_PRETTY_FUNC
        A::print();
        std::printf("b = %d\n", b);
    }

    int getB() const
    {
        PRINT_PRETTY_FUNC
        return b;
    }

    void setB(int value)
    {
        PRINT_PRETTY_FUNC
        b = value;
    }

private:
    int b = -1;
};

} // namespace test

void register_classes()
{
    using namespace rtti;
    global_define()
        ._namespace("test")
            ._class<A>("A")
                ._constructor<int>()
                ._method("print", &A::print)
                ._method("getA", &A::getA)
                ._method("setA", &A::setA)
                ._method<std::string(A::*)(int) const>("overload_on_const", &A::overload_on_const)
                ._method<std::string(A::*)(int)>("overload_on_const", &A::overload_on_const)
            ._end()
            ._class<B>("B")._base<A>()
                ._constructor<int>()
                ._property("b", &B::getB, &B::setB)
            ._end()
        ._end()
    ;
}

void test_reflection()
{
    using namespace rtti;
    register_classes();
    
    auto lambda = [] (const variant &v)
    {
        auto MC = MetaClass::findByTypeId(v.classInfo().typeId); assert(MC);
        auto getaM = MC->getMethod("getA"); assert(getaM);
        auto r = getaM->invoke(v); assert(r.to<int>() == 100);

        r = 256;
        auto setaM = MC->getMethod("setA"); assert(setaM);
        setaM->invoke(v, r);
        r = getaM->invoke(v); assert(r.value<int>() == 256);

        auto print = MC->getMethod("print"); assert(print);
        print->invoke(v);

        {
            auto overM = MC->getMethod<A&, int>("overload_on_const"); assert(overM);
            auto r = overM->invoke(v, 200); assert(r.value<std::string>() == "200");
        }

        {
            auto overM = MC->getMethod<const A&, int>("overload_on_const"); assert(overM);
            auto r = overM->invoke(v, 300); assert(r.value<std::string>() == "300");
        }

        r = getaM->invoke(v); assert(r.value<int>() == 200);
        print->invoke(v);

        {
            auto bP = MC->getProperty("b");
            if (bP)
            {
                auto r = bP->get(v);
                bP->set(v, 1024);

            }
        }

    };

    std::printf("\n");

    {
        variant v = A{100};
        lambda(v);
        auto a = new A{100};
        v = a;
        lambda(v);
        delete a;
    }

    std::printf("\n");

    {
        variant v = B{100};
        lambda(v);
        auto b = new B{100};
        v = b;
        lambda(v);
        delete b;
    }

    std::printf("\n");

    {
        B b{100};
        variant v = std::ref(b);
        lambda(v);
    }
}
```
