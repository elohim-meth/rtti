﻿#ifndef METAERROR_H
#define METAERROR_H

#include <stdexcept>

#include "global.h"

namespace rtti {

class DLL_PUBLIC runtime_error: public std::runtime_error
{
public:
    using base_t = std::runtime_error;
    using base_t::base_t;
};

class DLL_PUBLIC unregistered_metaclass: public runtime_error
{
public:
    using runtime_error::runtime_error;
};

class DLL_PUBLIC duplicate_metaclass: public runtime_error
{
public:
    using runtime_error::runtime_error;
};

class DLL_PUBLIC invalid_metatype_id: public runtime_error
{
public:
    using runtime_error::runtime_error;
};

class DLL_PUBLIC invalid_meta_define: public runtime_error
{
public:
    using runtime_error::runtime_error;
};

}

#endif // METAERROR_H

