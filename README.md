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

Look at [wiki](https://github.com/elohim-meth/rtti/wiki) for examples and help.
