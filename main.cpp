#include <cstdio>
#include <iostream>

#include "./dbg.hpp"
#include "./generator.hpp"
#include "./grammar.hpp"
#include "./parser.hpp"
#include "./searcher.hpp"

int main(int argc, char* argv[]) {
  DBG.LogLevel = 2;
  while (1) {
    char buf[0x1000];
    if (!std::cin.getline(buf, 0xfff)) {
      break;
    }
    printf("\n\n");
    size_t i = 0;
    for (i = 0; buf[i]; i++) {
      if (buf[i] == '\n') buf[i] = 0;
    }
    if (strlen(buf) < 1) continue;
    auto a = Parser(buf);

    if (a.GetErrorCode()) {
      fprintf(stderr, "\nError in Parser. Abort.\n");
      break;
    }

    auto root = a.GetTree();
    auto vars = a.GetVariables();
    auto sz = root->Tidy();
    // root->Print(true);
    auto s = Searcher(sz, root, vars);

    auto sels = s.GetTrueSelectionSet();

    auto g = Generator(vars, sels);
    printf("\n\n");
    g.PrintTruthTable(stdout);
    printf("\n");
    g.PrintPDNF(stdout);
    printf("\n\n");
    // fprintf(stderr, "\n\n");
    // break;
  }
  return 0;
}
