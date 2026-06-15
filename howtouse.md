# How to use the library
*The library is written entirely in C++ -- Linux-based systems support only for now, keep that in mind.*

## 1. Starting-out
This library is object-oriented featuring many classes. To keep it simple for now we'll start off by creating a screen buffer.
```cpp
#include "ostl.hpp"

int main()
{
  const auto termSize = get_terminal_size();
  OSTL::Buffer screen(termSize);
  return 0;
}
```
This is just creating the object. Lets move onto the simplest printing.
```cpp
#include "ostl.hpp"

int main()
{
  const auto termSize = get_terminal_size();
  OSTL::Buffer screen(termSize);
  screen.clear(); // Clears the buffer
  screen.write((OSTL::Coords){0,0}, "Hello, World!"); // Writes "Hello, World!" to the buffer at position {0, 0}
  screen.flush(); // Prints the buffer
  return 0;
}
```
# MORE PARTS WIP