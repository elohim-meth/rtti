#include <rtti/metadefine.h>


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

template<typename T>
void register_std_unique_ptr()
{
    using namespace std::literals;

    auto name = "unique_ptr<"s.append(rtti::type_name<T>()) + '>';
    rtti::global_define()
        ._namespace("std")
            ._class<std::unique_ptr<T>>(name)
                ._lazy(define_std_unique_ptr<T>)
            ._end()
        ._end()
    ;
}

template<typename Char>
void define_std_string(rtti::meta_define<std::basic_string<Char>> define)
{
    using T = std::basic_string<Char>;
    using size_type = typename T::size_type;
    using value_type = typename T::value_type;
    using reference = typename T::reference;
    using const_reference = typename T::const_reference;
    using iterator = typename T::iterator;

    define
        .template _constructor<value_type const*>()
        .template _constructor<value_type const*, size_type>()

        .template _method("size", &T::size)
        .template _method<void (T::*)(size_type)>("resize", &T::resize)
        .template _method<void (T::*)(size_type, value_type)>("resize_init_char", &T::resize)
        .template _method("capacity", &T::capacity)
        .template _method("empty", &T::empty)
        .template _method("clear", &T::clear)
        .template _method("reserve", &T::reserve)

        .template _method("c_str", &T::c_str)

        .template _method<iterator (T::*)()>("begin", &T::begin)
        .template _method("cbegin", &T::cbegin)

        .template _method<iterator (T::*)()>("end", &T::end)
        .template _method("cend", &T::cend)

        .template _method<reference (T::*)(size_type)>("[]", &T::operator[])
        .template _method<const_reference (T::*)(size_type) const>("const []", &T::operator[])
        .template _method<reference (T::*)(size_type)>("at", &T::at)
        .template _method<const_reference (T::*)(size_type) const>("const at", &T::at)

        .template _method<T& (T::*)(T const&)>("append str", &T::operator+=)
        .template _method<T& (T::*)(value_type const*)>("append pchar", &T::operator+=)
        .template _method<T& (T::*)(value_type)>("append char", &T::operator+=)

        .template _method<size_type (T::*)(value_type const*, size_type) const>("find pchar", &T::find)
        .template _method<size_type (T::*)(T const&, size_type) const>("find str", &T::find)
        .template _method<size_type (T::*)(value_type, size_type) const>("find char", &T::find)

        .template _method<size_type (T::*)(value_type const*, size_type) const>("rfind pchar", &T::rfind)
        .template _method<size_type (T::*)(T const&, size_type) const>("rfind str", &T::rfind)
        .template _method<size_type (T::*)(value_type, size_type) const>("rfind char", &T::rfind)
    ;
}

template<typename Char>
void register_std_string(std::string_view name)
{
    rtti::global_define()
        ._namespace("std")
            ._class<std::basic_string<Char>>(name)
                ._lazy(define_std_string<Char>)
            ._end()
        ._end()
    ;
}

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
    using initializer_list = std::initializer_list<value_type>;

    define
        .template _constructor<size_type>("size")
        .template _constructor<size_type, const_reference>("size_init_val")
        .template _constructor<initializer_list>("init_list")
        .template _constructor<pointer, pointer>("range1")
        .template _constructor<iterator, iterator>("range2")

        .template _method("size", &V::size)
        .template _method<void (V::*)(size_type)>("resize", &V::resize)
        .template _method<void (V::*)(size_type, const_reference)>("resize_init_val", &V::resize)
        .template _method("capacity", &V::capacity)
        .template _method("empty", &V::empty)
        .template _method("clear", &V::clear)
        .template _method("reserve", &V::reserve)

        .template _method<iterator (V::*)()>("begin", &V::begin)
        .template _method("cbegin", &V::cbegin)

        .template _method<reference (V::*)()>("front", &V::front)
        .template _method<const_reference(V::*)() const>("cfront", &V::front)

        .template _method<iterator (V::*)()>("end", &V::end)
        .template _method("cend", &V::cend)

        .template _method<reference (V::*)()>("back", &V::back)
        .template _method<const_reference(V::*)() const>("cback", &V::back)

        .template _method<reference (V::*)(size_type)>("[]", &V::operator[])
        .template _method<const_reference (V::*)(size_type) const>("const []", &V::operator[])
        .template _method<reference (V::*)(size_type)>("at", &V::at)
        .template _method<const_reference (V::*)(size_type) const>("const at", &V::at)

        .template _method<void (V::*)(value_type const&)>("push_back", &V::push_back)
        .template _method<void (V::*)(value_type &&)>("push_back_rv", &V::push_back)
        .template _method("pop_back", &V::pop_back)
    ;
}

template<typename T>
void register_std_vector()
{
    using namespace std::literals;

    auto name = "vector<"s.append(rtti::type_name<T>()) + '>';
    rtti::global_define()
        ._namespace("std")
            ._class<std::vector<T>>(name)
                ._lazy(define_std_vector<T>)
            ._end()
        ._end()
    ;
}

void register_std_ns()
{
    register_std_unique_ptr<std::string>();
    register_std_string<char>("string");
    register_std_vector<std::string>();
    register_std_vector<int>();
}
