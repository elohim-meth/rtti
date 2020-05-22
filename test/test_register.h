#ifndef TEST_REGISTER_H
#define TEST_REGISTER_H

#include <vector>
#include <memory>

#include <rtti/metadefine.h>

template<typename T>
void define_std_vector(rtti::meta_define<std::vector<T>> define)
{
    using V = std::vector<T>;
    using size_type = typename V::size_type;
    using value_type = typename V::value_type;
    using pointer = typename V::pointer;
    using reference = typename V::reference;
    using const_reference = typename V::const_reference;
    using iterator = typename V::iterator;
    using const_iterator = typename V::const_iterator;
    using initializer_list = std::initializer_list<value_type>;

    define
        .template _constructor<size_type>()
        .template _constructor<size_type, const_reference>()
        .template _constructor<initializer_list>()
        .template _constructor<pointer, pointer>("range1")
        .template _constructor<iterator, iterator>("range2")

        .template _method("size", &V::size)
        .template _method<void (V::*)(size_type)>("resize", &V::resize)
        .template _method<void (V::*)(size_type, const_reference)>("resize", &V::resize)
        .template _method("capacity", &V::capacity)
        .template _method("empty", &V::empty)
        .template _method("clear", &V::clear)
        .template _method("reserve", &V::reserve)

        .template _method<iterator (V::*)()>("begin", &V::begin)
        .template _method<const_iterator (V::*)() const>("begin", &V::begin)
        .template _method("cbegin", &V::cbegin)

        .template _method<reference (V::*)()>("front", &V::front)
        .template _method<const_reference(V::*)() const>("front", &V::front)

        .template _method<iterator (V::*)()>("end", &V::end)
        .template _method<const_iterator (V::*)() const>("end", &V::end)
        .template _method("cend", &V::cend)

        .template _method<reference (V::*)()>("back", &V::back)
        .template _method<const_reference(V::*)() const>("back", &V::back)

        .template _method<reference (V::*)(size_type)>("[]", &V::operator[])
        .template _method<const_reference (V::*)(size_type) const>("const []", &V::operator[])
        .template _method<reference (V::*)(size_type)>("at", &V::at)
        .template _method<const_reference (V::*)(size_type) const>("const at", &V::at)

        .template _method<void (V::*)(value_type const&)>("push_back", &V::push_back)
        .template _method<void (V::*)(value_type &&)>("push_back", &V::push_back)
        .template _method("pop_back", &V::pop_back)
    ;
}

extern template void define_std_vector<int>(rtti::meta_define<std::vector<int>>);
extern template void define_std_vector<std::string>(rtti::meta_define<std::vector<std::string>>);
extern template void define_std_vector<char const*>(rtti::meta_define<std::vector<char const*>>);

template<typename T>
void define_std_string(rtti::meta_define<T> define)
{
    using size_type = typename T::size_type;
    using value_type = typename T::value_type;
    using reference = typename T::reference;
    using const_reference = typename T::const_reference;
    using iterator = typename T::iterator;
    using const_iterator = typename T::const_iterator;

    define
        .template _constructor<value_type const*>()
        .template _constructor<value_type const*, size_type>()

        .template _method("size", &T::size)
        .template _method<void (T::*)(size_type)>("resize", &T::resize)
        .template _method("capacity", &T::capacity)
        .template _method("empty", &T::empty)
        .template _method("clear", &T::clear)
        .template _method("reserve", &T::reserve)

        .template _method("c_str", &T::c_str)

        .template _method<iterator (T::*)()>("begin", &T::begin)
        .template _method<const_iterator (T::*)() const>("begin", &T::begin)
        .template _method("cbegin", &T::cbegin)

        .template _method<iterator (T::*)()>("end", &T::end)
        .template _method<const_iterator (T::*)() const>("end", &T::end)
        .template _method("cend", &T::cend)

        .template _method<reference (T::*)(size_type)>("[]", &T::operator[])
        .template _method<const_reference (T::*)(size_type) const>("[]", &T::operator[])
        .template _method<reference (T::*)(size_type)>("at", &T::at)
        .template _method<const_reference (T::*)(size_type) const>("at", &T::at)

        .template _method<T& (T::*)(T const&)>("+=", &T::operator+=)
        .template _method<T& (T::*)(value_type const*)>("+=", &T::operator+=)
        .template _method<T& (T::*)(value_type)>("+=", &T::operator+=)

        .template _method<size_type (T::*)(value_type const*, size_type) const>("find", &T::find)
        .template _method<size_type (T::*)(T const&, size_type) const>("find", &T::find)
        .template _method<size_type (T::*)(value_type, size_type) const>("find", &T::find)

        .template _method<size_type (T::*)(value_type const*, size_type) const>("rfind", &T::rfind)
        .template _method<size_type (T::*)(T const&, size_type) const>("rfind", &T::rfind)
        .template _method<size_type (T::*)(value_type, size_type) const>("rfind", &T::rfind)
    ;
}

extern template void define_std_string<std::string>(rtti::meta_define<std::string>);
extern template void define_std_string<std::wstring>(rtti::meta_define<std::wstring>);

template<typename T>
void define_std_unique_ptr(rtti::meta_define<std::unique_ptr<T>> define)
{
    using U = std::unique_ptr<T>;
    define
        .template _constructor<typename U::pointer>()
        .template _method("get", &U::get)
        .template _method("release", &U::release)
        .template _method("reset", &U::reset)
    ;
}

extern template void define_std_unique_ptr<int>(rtti::meta_define<std::unique_ptr<int>>);
extern template void define_std_unique_ptr<int const>(rtti::meta_define<std::unique_ptr<int const>>);

template<typename T>
void register_std_vector()
{
    rtti::global_define()
        ._namespace("std")
            ._class<std::vector<T>>("vector<" + mpl::type_name<T>() + ">")
                ._lazy(define_std_vector<T>)
            ._end()
        ._end()
    ;
}

template<typename T>
void register_std_unique_ptr()
{
    rtti::global_define()
        ._namespace("std")
            ._class<std::unique_ptr<T>>("unique_ptr<" + mpl::type_name<T>() + ">")
                ._lazy(define_std_unique_ptr<T>)
            ._end()
        ._end()
    ;
}

void define_test_namespace(rtti::meta_define<void> define);
void register_rtti();

void test_rtti_1();

#endif // TEST_REGISTER_H
