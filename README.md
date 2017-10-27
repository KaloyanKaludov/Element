## Element

A toy programming language. 
See some example code in the examples directory.

It is supposed to be a prototype-based object-oriented scripting language.
It is implemented in C++ without any external dependencies a part from STL.
It has its own virtual machine, mostly inspired by Python's VM.
Project files are provided for Visual Studio on Windows and
for the Codelite IDE on Linux. And a basic make file.

### Features

- Nil, Integer, Floating point, Boolean, String, Array and Object primitives.
- Functions.
- Closures.
- Coroutines.
- Block scopes.
- Modules.
- Iterators.
- Standard flow control with if, while, for expressions.
- Everything is an expression and everything returns a value.
- Error handling is based on an Error value type.
- Prototype-based OOP.
- REPL mode.

### Further development

- Add more tests
- Garbage Collection/Memory Management
- Saving and reading bytecode from compiled binary files
- Allow multiple lines to be entered when in REPL mode
- Tail recursion
- Benchmark the performance to find out exactly how slow everything is
- Add more to the standard library
