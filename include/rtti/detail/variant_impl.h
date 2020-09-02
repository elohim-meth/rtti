#ifndef VARIANT_IMPL_H
#define VARIANT_IMPL_H

namespace rtti {

namespace internal {

template<typename T, bool Small = sizeof(T) <= STORAGE_SIZE>
using is_inplace = std::integral_constant<bool, Small>;

template<typename T>
using is_inplace_t = typename is_inplace<T>::type;

template<typename T>
constexpr auto is_inplace_v = is_inplace<T>::value;


template<typename T>
struct is_reference_wrapper: std::false_type
{};

template<typename T>
struct is_reference_wrapper<std::reference_wrapper<T>>: std::true_type
{};

template<typename T>
using is_reference_wrapper_t = typename is_reference_wrapper<T>::type;

template<typename T>
constexpr auto is_reference_wrapper_v = is_reference_wrapper<T>::value;

template<typename T, bool = is_reference_wrapper_v<T>>
struct unwrap_reference;

template<typename T>
struct unwrap_reference<T, false>: mpl::identity<T>
{};

template<typename T>
struct unwrap_reference<T, true>: mpl::identity<typename T::type>
{};

template<typename T>
using unwrap_reference_t = typename unwrap_reference<T>::type;

template<typename T,
         bool = is_inplace_v<std::remove_cv_t<T>>,
         bool = is_reference_wrapper_v<std::remove_cv_t<T>>
        >
struct variant_function_table_impl;

template<typename T>
struct variant_function_table_impl<T, true, false>
{
    using Decay = remove_all_cv_t<T>;

    static MetaType_ID type(type_attribute attr)
    {
        switch (attr) {
        case type_attribute::NONE:
            return metaTypeId<U>();
        case type_attribute::LREF:
            return metaTypeId<ULref>();
        case type_attribute::RREF:
            return metaTypeId<URref>();
        case type_attribute::LREF_CONST:
            return metaTypeId<UConstLref>();
        default:
            return metaTypeId<U>();
        }
    }

    static void const* access(variant_type_storage const &value) noexcept
    {
        return &value.buffer;
    }

    static void copy_construct(void const *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_copy_constructible_v<Decay>)
    {
        type_manager_t<Decay>::copy_construct(value, &storage.buffer);
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible_v<Decay>)
    {
        type_manager_t<Decay>::move_or_copy(value, &storage.buffer);
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible_v<Decay>)
    {
        type_manager_t<Decay>::copy_construct(&src.buffer, &dst.buffer);
    }

    static void move(variant_type_storage &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_move_constructible_v<Decay>)
    {
        type_manager_t<Decay>::move_or_copy(&src.buffer, &dst.buffer);
    }

    static void destroy(variant_type_storage &value) noexcept
    {
        type_manager_t<Decay>::destroy(&value.buffer);
    }

    static bool compare_eq(variant_type_storage const &lhs, void const *rhs)
        noexcept(has_nt_eq_v<Decay, Decay>)
    {
        return type_manager_t<Decay>::compare_eq(&lhs.buffer, rhs);
    }
private:
    using U = std::remove_cv_t<T>;
    using ULref = std::add_lvalue_reference_t<U>;
    using URref = std::add_rvalue_reference_t<U>;
    using UConstLref = std::add_lvalue_reference_t<std::add_const_t<U>>;
};

template<typename T>
struct variant_function_table_impl<T, true, true>
{
    static MetaType_ID type(type_attribute attr)
    {
        switch (attr) {
        case type_attribute::NONE:
            return metaTypeId<U>();
        case type_attribute::LREF:
            return metaTypeId<ULref>();
        case type_attribute::RREF:
            return metaTypeId<URref>();
        case type_attribute::LREF_CONST:
            return metaTypeId<UConstLref>();
        default:
            return metaTypeId<U>();
        }
    }

    static void const* access(variant_type_storage const &value) noexcept
    {
        if constexpr(std::is_array_v<U>)
            return &value.ptr;
        else
            return value.ptr;
    }

    static void copy_construct(void const *value, variant_type_storage &storage) noexcept
    {
        auto ptr = static_cast<Wrapper const*>(value);
        storage.ptr = const_cast<Decay*>(&ptr->get());
    }

    static void move_construct(void *value, variant_type_storage &storage) noexcept
    {
        auto ptr = static_cast<Wrapper*>(value);
        storage.ptr = const_cast<Decay*>(&ptr->get());
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst) noexcept
    {
        dst.ptr = src.ptr;
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        dst.ptr = src.ptr;
    }

    static void destroy(variant_type_storage&) noexcept
    { /* do nothing */ }

    static bool compare_eq(variant_type_storage const &lhs, void const *rhs)
        noexcept(has_nt_eq_v<Decay, Decay>)
    {
        return type_manager_t<Decay>::compare_eq(lhs.ptr, rhs);
    }

private:
    using Wrapper = remove_all_cv_t<T>;
    using U = unwrap_reference_t<Wrapper>;
    using ULref = std::add_lvalue_reference_t<U>;
    using URref = std::add_rvalue_reference_t<U>;
    using UConstLref = std::add_lvalue_reference_t<std::add_const_t<U>>;

    using Decay = std::conditional_t<std::is_array_v<U>, remove_all_cv_t<U>, full_decay_t<U>>;
};

template<typename T>
struct variant_function_table_impl<T, false, false>
{
    using Decay = remove_all_cv_t<T>;

    static MetaType_ID type(type_attribute attr)
    {
        switch (attr) {
        case type_attribute::NONE:
            return metaTypeId<U>();
        case type_attribute::LREF:
            return metaTypeId<ULref>();
        case type_attribute::RREF:
            return metaTypeId<URref>();
        case type_attribute::LREF_CONST:
            return metaTypeId<UConstLref>();
        default:
            return metaTypeId<U>();
        }
    }

    static void const* access(variant_type_storage const &value) noexcept
    {
        return value.ptr;
    }

    static void copy_construct(void const *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_copy_constructible_v<Decay>)
    {
        storage.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::copy_construct(value, storage.ptr);
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible_v<Decay>)
    {
        storage.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::move_or_copy(value, storage.ptr);
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible_v<Decay>)
    {
        dst.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::copy_construct(src.ptr, dst.ptr);
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
    }

    static void destroy(variant_type_storage &value) noexcept
    {
        type_manager_t<Decay>::destroy(value.ptr);
        type_manager_t<Decay>::deallocate(value.ptr);
    }

    static bool compare_eq(variant_type_storage const &lhs, void const *rhs)
        noexcept(has_nt_eq_v<Decay, Decay>)
    {
        return type_manager_t<Decay>::compare_eq(lhs.ptr, rhs);
    }

private:
    using U = std::remove_cv_t<T>;
    using ULref = std::add_lvalue_reference_t<U>;
    using URref = std::add_rvalue_reference_t<U>;
    using UConstLref = std::add_lvalue_reference_t<std::add_const_t<U>>;
};

template<typename T, std::size_t N>
struct variant_function_table_impl<T[N], false, false>
{
    using Decay = remove_all_cv_t<T[N]>;
    using Base = std::remove_all_extents_t<Decay>;

    static MetaType_ID type(type_attribute attr)
    {
        switch (attr) {
        case type_attribute::NONE:
            return metaTypeId<U>();
        case type_attribute::LREF:
            return metaTypeId<ULref>();
        case type_attribute::RREF:
            return metaTypeId<URref>();
        case type_attribute::LREF_CONST:
            return metaTypeId<UConstLref>();
        default:
            return metaTypeId<U>();
        }
    }

    static void const* access(variant_type_storage const &value) noexcept
    {
        return &value.ptr;
    }

    static void copy_construct(void const *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_copy_constructible_v<Base>)
    {
        storage.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::copy_construct(value, storage.ptr);
    }

    static void move_construct(void *value, variant_type_storage &storage)
        noexcept(std::is_nothrow_move_constructible_v<Base>)
    {
        storage.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::move_or_copy(value, storage.ptr);
    }

    static void copy(variant_type_storage const &src, variant_type_storage &dst)
        noexcept(std::is_nothrow_copy_constructible_v<Base>)
    {
        dst.ptr = type_manager_t<Decay>::allocate();
        type_manager_t<Decay>::copy_construct(src.ptr, dst.ptr);
    }

    static void move(variant_type_storage &src, variant_type_storage &dst) noexcept
    {
        std::swap(src.ptr, dst.ptr);
    }

    static void destroy(variant_type_storage &value) noexcept
    {
        type_manager_t<Decay>::destroy(value.ptr);
        type_manager_t<Decay>::deallocate(value.ptr);
    }

    static bool compare_eq(variant_type_storage const &lhs, void const *rhs)
        noexcept(has_nt_eq_v<Decay, Decay>)
    {
        return type_manager_t<Decay>::compare_eq(lhs.ptr, rhs);
    }
private:
    using U = Decay;
    using ULref = std::add_lvalue_reference_t<U>;
    using URref = std::add_rvalue_reference_t<U>;
    using UConstLref = std::add_lvalue_reference_t<std::add_const_t<U>>;
};


// Since two pointer indirections when casting array<T> -> T*
// we need to use temp storage and have no room for inplace
template<typename T, std::size_t N>
struct variant_function_table_impl<T[N], true, false>:
       variant_function_table_impl<T[N], false, false>
{};

template<typename T>
struct class_info_get
{
    static ClassInfo info(variant_type_storage const &value)
    {
        if constexpr(!(std::is_class_v<Decay> || is_class_ptr_v<Decay>))
        {
            return ClassInfo{};
        }
        else
        {
            using is_registered = typename has_method_classInfo<ClassInfo(C::*)() const>::type;

            auto instance = Selector::access(value);
            if constexpr(std::is_class_v<Decay>)
            {
                if constexpr(is_registered::value)
                {
                    auto ptr = static_cast<C const*>(instance);
                    return ptr->classInfo();
                }
                else
                    return ClassInfo{metaTypeId<C>(), instance};
            }
            else if constexpr(is_class_ptr_v<Decay>)
            {
                if constexpr(is_registered::value)
                {
                    auto ptr = reinterpret_cast<C const * const *>(instance);
                    return (*ptr)->classInfo();
                }
                else
                {
                    auto ptr = reinterpret_cast<C const * const *>(instance);
                    return ClassInfo{metaTypeId<C>(), *ptr};
                }
            }
        }
    }
private:
    using Unwrap = unwrap_reference_t<std::remove_cv_t<T>>;
    using Decay = std::conditional_t<std::is_array_v<Unwrap>, void, full_decay_t<Unwrap>>;
    using Selector = variant_function_table_impl<T>;
    using C = std::remove_pointer_t<Decay>;
};

template<typename T>
inline variant_function_table const* variant_function_table_for() noexcept
{
    static auto const result = variant_function_table{
        &variant_function_table_impl<T>::type,
        &variant_function_table_impl<T>::access,
        &variant_function_table_impl<T>::copy_construct,
        &variant_function_table_impl<T>::move_construct,
        &variant_function_table_impl<T>::copy,
        &variant_function_table_impl<T>::move,
        &variant_function_table_impl<T>::destroy,
        &variant_function_table_impl<T>::compare_eq,
        &class_info_get<T>::info
    };
    return &result;
}

template<>
inline variant_function_table const* variant_function_table_for<void>() noexcept
{
    static auto const result = variant_function_table{
        [] (type_attribute) noexcept -> MetaType_ID { return MetaType_ID(); },
        [] (variant_type_storage const&) noexcept -> void const* { return nullptr; },
        [] (void const*, variant_type_storage&) noexcept {},
        [] (void*, variant_type_storage&) noexcept {},
        [] (variant_type_storage const&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&) noexcept {},
        [] (variant_type_storage const&, void const*) noexcept -> bool { return false; },
        [] (variant_type_storage const&) noexcept { return ClassInfo(); }
    };
    return &result;
}

template<>
inline variant_function_table const* variant_function_table_for<variant>() noexcept
{
    static auto const result = variant_function_table{
        [] (type_attribute) noexcept -> MetaType_ID { return metaTypeId<variant>(); },
        [] (variant_type_storage const&) noexcept -> void const* { return nullptr; },
        [] (void const*, variant_type_storage&) noexcept {},
        [] (void*, variant_type_storage&) noexcept {},
        [] (variant_type_storage const&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&, variant_type_storage&) noexcept {},
        [] (variant_type_storage&) noexcept {},
        [] (variant_type_storage const&, void const*) noexcept -> bool { return false; },
        [] (variant_type_storage const&) noexcept { return ClassInfo(); }
    };
    return &result;
}

} // namespace internal

template<typename T, typename>
variant::variant(T &&value)
    : manager{internal::variant_function_table_for<std::remove_reference_t<T>>()}
{
    using NoRef = std::remove_reference_t<T>;
    using Type = std::conditional_t<std::is_array_v<NoRef>, std::remove_all_extents_t<NoRef>, NoRef>;
    constexpr auto move = !std::is_reference_v<T> && !std::is_const_v<T>;
    constexpr auto valid = std::is_copy_constructible_v<Type>
                           || (move && std::is_move_constructible_v<Type>);
    static_assert(valid, "The contained type must be CopyConstructible or MoveConstructible");
    using selector_t = std::conditional_t<move, std::true_type, std::false_type>;

    constructor(std::addressof(value), selector_t{});
}

template<typename T, typename>
variant& variant::operator=(T &&value)
{
    variant{std::forward<T>(value)}.swap(*this);
    return *this;
}

template<typename T>
bool variant::eq(T const &value) const
{
    if (*this == variant{std::cref(value)})
        return true;

    auto mt_self = MetaType{internalTypeId(type_attribute::NONE)};
    auto mt_value = MetaType{metaTypeId<T>()};

    if (MetaType::hasConverter(mt_self, mt_value))
    {
        std::aligned_storage_t<sizeof(T), alignof(T)> buffer;
        if (MetaType::convert(raw_data_ptr(), mt_self, &buffer, mt_value))
        {
            FINALLY { type_manager_t<T>::destroy(&buffer); };
            return type_manager_t<T>::compare_eq(std::addressof(value), &buffer);
        }
    }

    if (MetaType::hasConverter(mt_value, mt_self))
    {
        auto *buffer = mt_self.allocate();
        FINALLY { mt_self.deallocate(buffer); };

        // Array is passed as **data
        auto tmp = std::addressof(value);
        void const *address = tmp;
        if (mt_value.isArray())
            address = std::addressof(tmp);

        if (MetaType::convert(address, mt_value, buffer, mt_self))
        {
            FINALLY { mt_self.destroy(buffer); };
            auto ptr = raw_data_ptr();
            if (mt_self.isArray())
                ptr = *reinterpret_cast<void const *const *>(ptr);
            return mt_self.compare_eq(ptr, buffer);
        }
    }

    return false;
}

template<typename ...Args>
variant variant::invoke(std::string_view name, Args&& ...args)
{
    using namespace std::literals;

    auto type = MetaType{typeId()};
    if (type.isClass() || type.isClassPtr())
    {
        if (auto *mt_class = this->metaClass())
        {
            if (auto mt_method = mt_class->getMethod(name))
                return mt_method->invoke(*this, std::forward<Args>(args)...);

            throw runtime_error{"Method ["s + name + "] isn't found in class T = " + type.typeName()};
        }
        throw runtime_error{"Class T = "s + type.typeName() + " isn't registered"};
    }
    throw runtime_error{"Type T = "s + type.typeName() + " isn't Class or ClassPtr"};
}

template<typename ...Args>
variant variant::invoke(std::string_view name, Args&& ...args) const
{
    using namespace std::literals;

    auto type = MetaType{typeId()};
    if (type.isClass() || type.isClassPtr())
    {
        if (auto *mt_class = this->metaClass())
        {
            if (auto *mt_method = mt_class->getMethod(name))
                return mt_method->invoke(*this, std::forward<Args>(args)...);

            throw runtime_error{"Method ["s + name + "] isn't found in class T = " + type.typeName()};
        }
        throw runtime_error{"Class T = "s + type.typeName() + " isn't registered"};
    }
    throw runtime_error{"Type T = "s + type.typeName() + " isn't Class or ClassPtr"};
}


template<typename T>
void variant::set_property(std::string_view name, T &&value)
{
    using namespace std::literals;

    auto type = MetaType{typeId()};
    if (type.isClass() || type.isClassPtr())
    {
        if (auto *mt_class = this->metaClass())
        {
            if (auto *mt_property = mt_class->getProperty(name))
                return mt_property->set(*this, std::forward<T>(value));

            throw runtime_error{"Property ["s + name + "] isn't found in class T = " + type.typeName()};
        }
        throw runtime_error{"Class T = "s + type.typeName() + " isn't registered"};
    }
    throw runtime_error{"Type T = "s + type.typeName() + " isn't Class or ClassPtr"};
}

} // namespace rtti

namespace std {

template<>
struct hash<rtti::variant>: public std::__hash_base<std::size_t, rtti::variant>
{
    std::size_t operator()(rtti::variant const &value) const noexcept
    {
        if (!value)
            return 0;

        auto type = rtti::MetaType{value.typeId()};
        auto ptr = value.raw_data_ptr({});
        if (type.isArray())
            ptr = *reinterpret_cast<void const * const *>(ptr);
        return _Hash_impl::hash(ptr, type.typeSize());
    }
};

} //namespace std


#endif // VARIANT_IMPL_H
