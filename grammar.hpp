#pragma once
#ifndef GRAMMAR_HPP

#include <map>
#include <string>
#include <vector>

#include "./stack.hpp"

enum Symbol {
  NONE,
  VAR,
  LB,
  RB,
  IFF,
  FI,
  OR,
  AND,
  B,
  C,
  R,
  A,
  E,
  S,
  NOT,
  TRUE,
  FALSE
};

struct {
  Symbol const type;
  char const* const str;
  bool const tem;
} const SymbolInfo[] = {{NONE, "$", true}, {VAR, "p", true},  {LB, "(", true},
                        {RB, ")", true},   {IFF, "↔", true},  {FI, "→", true},
                        {OR, "∨", true},   {AND, "∧", true},  {B, "B", false},
                        {C, "C", false},   {R, "R", false},   {A, "A", false},
                        {E, "E", false},   {S, "S", false},   {NOT, "¬", true},
                        {TRUE, "1", true}, {FALSE, "0", true}};

struct Action {
  char type = 'e';
  int tag;
};

struct Production {
  Symbol const left;
  std::vector<Symbol> const right;
  std::string str;
};

struct TreeNode {
  inline static int cnt = 0;

  int uid;
  bool rev = false;
  Symbol sym;
  std::string str;
  TreeNode* left = nullptr;
  TreeNode* right = nullptr;
  char token = 0;

  TreeNode(Symbol sym = NONE) : sym(sym), uid(cnt++) {}
  TreeNode(TreeNode const& a) = default;
  TreeNode(TreeNode&& a)
      : uid(std::move(a.uid)),
        rev(std::move(a.rev)),
        sym(std::move(a.sym)),
        str(std::move(a.str)),
        left(std::move(a.left)),
        right(std::move(a.right)),
        token(std::move(a.token)) {}

  TreeNode& operator=(TreeNode const& a) = default;

  TreeNode& operator=(TreeNode&& a) {
    using std::move;
    uid = move(a.uid);
    rev = move(a.rev);
    sym = move(a.sym);
    str = move(a.str);
    left = move(a.left);
    right = move(a.right);
    token = move(a.token);
    return *this;
  }

  int Tidy(int s = 0) {
    Stack<TreeNode*> stk;
    stk += this;
    while (stk.size()) {
      auto& cur = *std::move(stk[-1]);
      stk.pop();
      cur.uid = s++;
      if (cur.left == nullptr) {
        for (auto& c : cur.str) {
          if (isalnum(c)) {
            cur.token = c;
            break;
          }
        }
        continue;
      }
      stk += cur.left;
      stk += cur.right;
    }
    return s;
  }

  bool IsLeaf() const { return left == nullptr; }

  void operator+=(TreeNode* const& c) {
    if (left == nullptr)
      left = c;
    else
      right = c;
  }

  operator const int() const { return uid; }

  operator std::string() {
#define printf(...) buf += sprintf(buf, __VA_ARGS__)
    char _buf[0x400];
    char* buf = _buf;
    int i = 1;
    printf("{\n    \"uid\": %d,\n", uid);
    printf("    \"rev\": %s\n", rev ? "true" : "false");
    printf("    \"sym\": \"%s\"\n", SymbolInfo[sym].str);

    std::string ret = std::string(buf) + str;
    buf = _buf;
    if (left) {
      printf("    \"child\": [ %d, %d ]\n}", left->uid, right->uid);
    }
    if (token) {
      printf("    \"token\": \"%c\"\n}", token);
    }
    return (ret + buf);
#undef printf
  }

  void Print(bool recursive, FILE* fp = stdout) {
    if (!recursive) {
      Print(fp);
      return;
    }
    Stack<TreeNode*> stk;
    Print(fp);
    if (left) stk += left;
    if (right) stk += right;
    while (stk.size()) {
      auto cur = *std::move(stk[-1]);
      stk.pop();
      fprintf(fp, ",\n");
      cur.Print(fp);
      if (cur.left) stk += cur.left;
      if (cur.right) stk += cur.right;
    }
  }

  void Print(FILE* fp = stdout) {
#define printf(...) fprintf(fp, __VA_ARGS__)
    int i = 1;
    printf("{\n    \"uid\": %d,\n", uid);
    printf("    \"rev\": %s\n", rev ? "true" : "false");
    printf("    \"sym\": \"%s\"\n", SymbolInfo[sym].str);
    printf("    \"str\": \"%s\"\n", str.c_str());
    if (left) {
      printf("    \"child\": [ %d, %d ]\n}", left->uid, right->uid);
    }
    if (token) {
      printf("    \"token\": \"%c\"\n}", token);
    }

#undef printf
  }
};

#endif /* GRAMMAR_HPP */
