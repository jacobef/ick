## ick - the Imacculate C Kompiler

This is currently just a preprocessor. It's not ready for real-world use yet, but feel free to mess around with it if you're curious!
The following directives work in their entirety: #if, #ifdef, #ifndef, #elif, #else, #endif, #define, #undef.
#include works, but only with absolute paths.
#line, #error, #pragma, and empty directives (i.e. #) aren't implemented yet.
Error messages are terrible for now.

### Building and running

These commands are for Unix (i.e. anything but Windows):
```shell
git clone https://github.com/jacobef/ick
cd ick
cc data_structures/*.c debug/*.c driver/*.c preprocessor/*.c main.c -I . -o ick
./ick test/compile_this.c  # or replace with another file
```

The output is a preprocessed file named like the input file, and in the same folder as the input file, but with .i instead of .c (e.g. test/compile_this.c -> test/compile_this.i).