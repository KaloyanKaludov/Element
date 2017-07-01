## Element

A toy programming language. 
See some example code in the examples directory.

It is supposed to be a prototype-based object-oriented scripting language.
It is implemented in C++ without any external dependencies a part from STL.
It has its own virtual machine, mostly inspired by Python's VM.
The Element interpreter can interpret only a single source file for now or it
can also be ran in REPL mode. Project files are provided for Visual Studio
on Windows and for the Codelite IDE for Linux. No makefile because I am lazy.

### Features

- Integer, Floating point, Boolean, String, Array and Object primitives.
- Functions, Closures.
- Block scopes.
- Standard flow control with if, while, for expressions.
- Everything is an expression and everything returns a value.
- Prototype-based OOP.
- REPL mode.

### Further development

- Importing code from files (module system?)
- Saving and reading bytecode from compiled binary files
- Access object members by runtime generated string keys
- Multiple return values from functions
- Multiple assignment (same as above?)
- Underscore variable
- Allow multiple lines to be entered when in REPL mode
- Move symbols hashing to the semantic analyzer
- Better Garbage Collection/Memory Management
- Tail recursion
- Benchmark the performance to find out exactly how slow everything is
- Replace STL containers in basic data types with C style structures
- Extend (Create?) a standard library


#### Potential problems:

The Codelite project uses the clang compiler, which depending on the Codelite
version might not be included and so you may have to install it separately.
Or you can just compile with gcc.
