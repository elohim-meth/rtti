#include <rtti/metadefine.h>
#include <iostream>

using namespace std::literals;

namespace test {

void greeting(char const *message)
{
    std::cout << "Hello, " << message << std::endl;
}

class Point2D
{
    // This macro adds one virtual method and is required for
    // polymorphic behaviour of variants and meta_cast feature
    // DECLARE_CLASSINFO
public:
    Point2D() : m_x(0), m_y(0) {
    }

    Point2D(int x, int y) : m_x(x), m_y(y) {
    }

    void extend(int scale) {
        m_x *= scale;
        m_y *= scale;
    }

    int getArea() const { return m_x * m_y; }

    int getX() const { return m_x; }
    void setX(int value) { m_x = value; }

    int& y() { return m_y; }
    int const& y() const { return m_y; }

private:
    int m_x;
    int m_y;
};

} // namespace test

// Reflection for test namespace.
// RTTI_REGISTER defines static global object that runs before main.
// You can of course use ordinary method instead.

RTTI_REGISTER
{
    rtti::global_define()
        ._namespace("test"sv)
            ._method("greeting"sv, &test::greeting)
            ._class<test::Point2D>("Point2D")
                ._constructor<int, int>()
                ._method("extend", &test::Point2D::extend)
                ._method("getArea", &test::Point2D::getArea)
                ._property("X"sv, &test::Point2D::getX, &test::Point2D::setX)
                ._method<int& (test::Point2D::*)()>("ref_Y", &test::Point2D::y)
                ._method<int const& (test::Point2D::*)() const>("const_ref_Y", &test::Point2D::y)
            ._end()
        ._end()
    ;
}

int main(int, char**)
{
    // Get the global meta data repository.
    auto *ns_global = rtti::MetaNamespace::global();

    // Get the meta method for global function "greeting".
    auto *mm_greeting = ns_global->getNamespace("test"sv)->getMethod("greeting"sv);

    // Invoke the global function.
    mm_greeting->invoke("World!");

    // Now let's use the class Point2D via reflection system.
    auto *mc_point = ns_global->getNamespace("test"sv)->getClass("Point2D"sv);

    {
        // Create an instance of Point2D to be used by meta functions.
        test::Point2D point(5, 8);

        // Get the meta property for "x".
        auto *mp_x = mc_point->getProperty("X"sv);

        // Get the value of "x". The result is a rtti::variant. x_value contains a copy of point.x!
        auto x_value = mp_x->get(&point);

        // Print the value. We use "to" method to convert a rtti::variant to relevant C++ type.
        std::cout << "point.x is " << x_value.to<int>() << " (should be 5)" << std::endl;

        // Get the meta method for "y".
        auto *mm_y = mc_point->getMethod("const_ref_Y");

        // Get the value of "y". The result is a rtti::variant. y_value contains a const reference to point.y!
        auto y_value = mm_y->invoke(&point);

        // Print the value. We use "to" method to convert a rtti::variant to relevant C++ type.
        std::cout << "point.y is " << y_value.to<int>() << " (should be 8)" << std::endl;

        // Get the meta method for "extend".
        auto *mm_extend = mc_point->getMethod("extend"sv);

        // Invoke Point2D::extend.
        mm_extend->invoke(&point, 2);

        // Since x property returns a copy we need to get the value again to reflect changes!
        x_value = mp_x->get(&point);
        std::cout << "After extend, point.x is " << x_value.to<int>() << " (should be 10)" << std::endl;

        // But y method returns reference! No need to get value again!
        std::cout << "After extend, point.y is " << y_value.to<int>() << " (should be 16)" << std::endl;

        // Get the meta method for "getArea".
        auto *mm_area = mc_point->getMethod("getArea"sv);

        // Invoke Point::getArea, and obtain the return value (rtti::variant).
        auto v_area = mm_area->invoke(&point);

        // Print the return value.
        std::cout << "The area is " << v_area.to<int>() << " (should be 160)" << std::endl;
    }

    {
        // Invoke default constructor of Point2D class
        auto v_point = mc_point->defaultConstructor()->invoke();

        // Extract reference from rtti::variant
        auto &point = v_point.ref<test::Point2D>();

        // Convenient way to get property instead of:
        // auto *mp_x = mc_point->getProperty("X"sv);
        // auto x_value = mp_x->get(v_point);
        auto x_value = v_point.get_property("X"sv);

        // Convenient way to invoke a method instead of:
        // auto *mm_y = mc_point->getMethod("const_ref_Y");
        // auto y_value = mm_y->invoke(v_point);
        auto y_value = v_point.invoke("const_ref_Y"sv);

        std::cout << "default_constructor -- point.x is " << x_value.to<int>() << " (should be 0), point.y is " << y_value.to<int>() << " (should be 0)" << std::endl;
        std::cout << "extracted reference -- point.x is " << point.getX() << " (should be 0), point.y is " << point.y() << " (should be 0)" << std::endl;
    }

    {
        // Invoke constructor with arguments
        auto v_point = mc_point->getConstructor<int, int>()->invoke(3, 8);
        // Extract reference from rtti::variant
        auto &point = v_point.ref<test::Point2D>();

        // Get values
        auto x_value = v_point.get_property("X"sv);
        auto y_value = v_point.invoke("const_ref_Y"sv);

        std::cout << "constructor -- point.x is " << x_value.to<int>() << " (should be 3), point.y is " << y_value.to<int>() << " (should be 8)" << std::endl;
        std::cout << "extracted reference -- point.x is " << point.getX() << " (should be 3), point.y is " << point.y() << " (should be 8)" << std::endl;

        // Get area
        auto v_area = v_point.invoke("getArea"sv);
        std::cout << "The area is " << v_area.to<int>() << " (should be 24)" << std::endl;

        // Scale 3 times
        v_point.invoke("extend", 3);

        // Print values
        x_value = v_point.get_property("X"sv);
        std::cout << "After extend, point.x is " << x_value.to<int>() << " (should be 9), point.y is " << y_value.to<int>() << " (should be 24)" << std::endl;

        // Get area
        v_area = v_point.invoke("getArea"sv);
        std::cout << "The area is " << v_area.to<int>() << " (should be 216)" << std::endl;
    }

    return EXIT_SUCCESS;
}
