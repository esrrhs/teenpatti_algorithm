// Command teenpatti_gen regenerates teenpatti_data.txt into the working
// directory. It is the C++ counterpart of running the Java
// TeenPattiAlgorithmUtil.main: GenUtil.genKey() followed by
// GenUtil.outputData().
#include <cstdio>

#include <teenpatti/teenpatti.hpp>

int main() {
  teenpatti::gen_key();
  if (!teenpatti::output_data()) {
    std::fprintf(stderr, "outputData failed\n");
    return 1;
  }
  return 0;
}
