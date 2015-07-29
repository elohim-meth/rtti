# rtti
A try to add runtime reflection to C++ language.

For now implemented meta namespaces, classes, methods, constructors and enums.
Support for meta properties is coming  soon.

Library contains generic variant type, it supports user defined conversion, provided by metatype system.

Added support for meta_cast replacement of dynamic_cast. 

Rtti library can be compiled as static or shared. Shared method is preferred since library contains
global static containers with various type information and they better to be in one place.
