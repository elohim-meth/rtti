#ifndef METAERROR_H
#define METAERROR_H

#include <stdexcept>

#include <rtti/export.h>

namespace rtti {

class RTTI_API runtime_error: public std::runtime_error
{
public:
    using base_t = std::runtime_error;
    using base_t::base_t;
};

class RTTI_API definition_error: public runtime_error
{
public:
    using runtime_error::runtime_error;
};

class RTTI_API unregistered_metaclass: public definition_error
{
public:
    using definition_error::definition_error;
};

class RTTI_API duplicate_metaclass: public definition_error
{
public:
    using definition_error::definition_error;
};

class RTTI_API invalid_metatype_id: public runtime_error
{
public:
    using runtime_error::runtime_error;
};

class RTTI_API invoke_error: public runtime_error
{
public:
    using runtime_error::runtime_error;
};

class RTTI_API bad_cast: public runtime_error
{
public:
    using runtime_error::runtime_error;
};

class RTTI_API bad_argument_cast final: public bad_cast
{
public:
    using bad_cast::bad_cast;
};

class RTTI_API bad_variant_cast: public bad_cast
{
public:
    using bad_cast::bad_cast;
};

class RTTI_API bad_variant_convert final: public bad_variant_cast
{
public:
    using bad_variant_cast::bad_variant_cast;
};

class RTTI_API bad_meta_cast final: public bad_cast
{
public:
    using bad_cast::bad_cast;
};

}

#endif // METAERROR_H

