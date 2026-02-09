#include <cstdio>

extern "C"
{
  int add(int a, int b)
  {
    return a + b;
  }

  void greet(const char *name)
  {
    std::printf("Hello from C++, %s!\n", name);
  }
}
