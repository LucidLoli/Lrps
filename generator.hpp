#pragma once
#ifndef GENERATOR_HPP

#include <set>

#include "./searcher.hpp"

struct Generator {
#define set std::set

 private:
  int numvar;
  char buf[32];

  set<char> vars;
  set<Selection> sels;
  set<uint32_t> ttab;

  void Search(int off = 0) {
    if (off == numvar) {
      __PrintInfo("%s\n", buf);
      uint32_t t = 0;
      for (int i = 0; i < numvar; i++) {
        if (buf[i] == '1') t |= 1 << i;
      }
      ttab.insert(t);
      return;
    }
    if (buf[off] != 0) {
      Search(off + 1);
      return;
    }
    buf[off] = '0';
    Search(off + 1);
    buf[off] = '1';
    Search(off + 1);
    buf[off] = 0;
  }

  void Run() {
    for (auto& [f, t] : sels) {
      memset(buf, 0, sizeof(buf));
      for (int d = 0; d < numvar; d++) {
        if (f & (1 << d)) {
          buf[d] = '0';
        }
        if (t & (1 << d)) {
          buf[d] = '1';
        }
      }
      Search();
    }
  }

 public:
  Generator(set<char> vars, set<Selection> sels)
#undef set
      : vars(vars), sels(sels), numvar(vars.size()) {
    Run();
    int i = 0;
    for (auto& c : vars) buf[i++] = c;
    buf[i] = 0;
  }

  void PrintTruthTable(FILE* fp) const {
#define printf(...) fprintf(fp, __VA_ARGS__)
    for (auto& v : vars) {
      printf("%c   ", v);
    }
    printf("\n");

    int m = (1 << numvar);
    for (int i = 0; i < m; i++) {
      for (int j = 0; j < numvar; j++) {
        if (i & (1 << j))
          printf("1   ");
        else
          printf("0   ");
      }
      if (ttab.find(i) != ttab.end()) {
        printf("  <1>  \n");
      } else {
        printf("  <0>  \n");
      }
    }
  }

  void PrintPDNF(FILE* fp) const {
    bool of = 1, nf = 1;
    int m = (1 << numvar);
    for (int i = 0; i < m; i++) {
      if (ttab.find(i) == ttab.end()) continue;

      if (of)
        of = 0;
      else
        printf(" ∨ ");

      nf = 1;
      for (int j = 0; j < numvar; j++) {
        if (nf)
          nf = 0;
        else
          printf("∧");

        if (i & (1 << j))
          printf("%c", buf[j]);
        else
          printf("¬%c", buf[j]);
      }
    }
  }
#undef printf
};

#endif /* GENERATOR_HPP */
