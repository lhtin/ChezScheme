#include <iostream>

#include <unistd.h>

int main(int argc, char* argv[] ) {
  std::cout << getpagesize() << std::endl;
  return 0;
}