Coding Style Guidelines for Stateline
=====================================
This document contains some guidelines to contributing code to Stateline. They are
mostly inspired by the [Google C++ Style Guide](http://google-styleguide.googlecode.com/svn/trunk/cppguide.xml),
but with some changes. Where this document is different from the style guide, use this document.

Language
--------
* Use C++11 features where appropriate.
* There is rarely a need to use a pointer.
* If you're using a pointer, use C++11 [smart pointers](http://www.cplusplus.com/reference/memory/shared_ptr/) instead.
* Use STL algorithms over explicit looping.
* Does it really need to be a class/have internal state?
* Do you really need inheritance in this circumstance? (probably not, unless you've got a container of the parent type full of different child types).
* By default, pass function arguments by const reference and return by value.
* Use initialisation lists in constructors.
* Use explicit in constructors taking in one argument.
* Move semantics where possible / appropriate.
* Use namespaces
* Use `#pragma once` for header guards.

Whitespace
----------
* Braces on a newline.
* 2 space indent.
* Use spaces not tabs.

Documentation
-------------
* Use Doxygen for documenting functions and classes.
* Use `//` style comments. 
* Document files, classes, functions and methods using Doxygen C++ style.
* Add the copyright licence header to every source and header file.
* Comment with proper capitalisation and punctuation.

Naming
------
* camelCase for variables and functions with lower case beginning
* PascalCase for classes.
* SCREAMING_CAPS_FOR_CONSTANTS
* `underscore_` notation for private member variables.
* `.hpp` for headers, `.cpp` for source files
* Australian english spelling of all variables (e.g. marginalise not marginalize).

IO
--
* Use logging over iostream
* Use iostream over stdio
