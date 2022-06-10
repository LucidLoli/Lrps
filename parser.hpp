#pragma once
#ifndef PARSER_HPP

#include <cassert>
#include <set>

#include "./dbg.hpp"
#include "./grammar.hpp"
#include "./stack.hpp"

struct Parser {
#define PrintError(...)                                                    \
  do {                                                                     \
    error = __LINE__;                                                      \
    __FprintfE(                                                            \
        stderr,                                                            \
        "[E] (%s: %d) Undefined State:\n  iptr - sptr: %ld\n  nsym: %s\n " \
        " statstk: [ ",                                                    \
        __FILE__, __LINE__, iptr - sptr, SymbolInfo[nsym].str);            \
    for (int i = 0; i < statstk.size(); i++) {                             \
      __FprintfE(stderr, "%d, ", statstk[i]);                              \
    }                                                                      \
    __FprintfE(stderr, " ]\n  symstk: [ ");                                \
    for (int i = 0; i < symstk.size(); i++) {                              \
      __FprintfE(stderr, "%s, ", SymbolInfo[symstk[i]].str);               \
    }                                                                      \
    __FprintfE(stderr, " ]\n");                                            \
  } while (0)

  using State = int;

 private:
  inline static bool init = false;
  inline static std::vector<Production> prodlst;
  inline static std::vector<std::vector<Action> > acttab;
  inline static std::map<int, std::vector<int> > transtab;

  int error;

  char* iptr;
  char* sptr;

  Symbol nsym;
  TreeNode* root;

  Stack<Symbol> symstk;
  Stack<State> statstk;
  Stack<TreeNode*> nodstk;

  std::set<char> vars;

 public:
  Parser(char* sentence) {
    if (!init) {
      Initialize();
      init = true;
    }

    error = 0;

    statstk += 0;
    nodstk += new TreeNode();
    sptr = iptr = sentence;
    Run();
    root = nodstk[-1];
  }

  TreeNode* const& GetTree() const { return root; }

  std::set<char> const& GetVariables() const { return vars; }

  int const& GetErrorCode() const { return error; }

 private:
  static void Initialize() {
#define NUM_PRODUCTION 11
#define NUM_STATE 41
    char* buf;
    size_t blen;
    FILE* fp = fopen("./production.table", "r");
    for (int i = 0; i < NUM_PRODUCTION; i++) {
      buf = NULL;
      blen = 0;
      if (getline(&buf, &blen, fp) < 1) break;
      char& last = buf[strlen(buf) - 1];
      if (!isalnum(last)) last = 0;
      AddProdution(buf);
      // free(buf);
    }
    fclose(fp);
    fp = fopen("./action.table", "r");
    for (int i = 0; i < NUM_STATE; i++) {
      buf = NULL;
      blen = 0;
      if (getline(&buf, &blen, fp) < 1) break;
      char& last = buf[strlen(buf) - 1];
      if (!isalnum(last)) last = 0;
      AddPrediction(buf);
      // free(buf);
    }
    fclose(fp);
#undef NUM_PRODUCTION
#undef NUM_STATE
  }

  static void AddProdution(char* buf) {
    int x, off;
    Symbol left;
    std::vector<Symbol> right;
    sscanf(buf, "%d", &x);
    ++buf;
    left = Symbol(x);
    while (1) {
      while (*buf != ',' && *buf != 0) {
        ++buf;
      }
      if (buf == 0) break;
      if (sscanf(buf, "%*c%d", &x) < 1) break;
      ++buf;
      right.push_back(Symbol(x));
    }
    while (*buf == ',' || isblank(*buf)) ++buf;
    prodlst.push_back(Production{left, right, std::string(buf)});
  }

  static void AddPrediction(char* buf) {
#define NUM_TERMINATOR 8
#define NUM_NONTERMINATOR 5
#define NUM_SYMBOL (NUM_TERMINATOR + NUM_NONTERMINATOR)
    char type;
    int t, off, ln;
    acttab.push_back(std::vector<Action>(NUM_TERMINATOR, Action{'e', -1}));
    auto& line = *acttab.rbegin();
    sscanf(buf, "%d", &ln);
    ++buf;
    for (int i = 0; i < NUM_SYMBOL; i++) {
      while (*buf != ',' && *buf != 0) {
        ++buf;
      }
      if (buf[0] == 0) break;
      if (buf[1] == ',') {
        ++buf;
        continue;
      }
      if (i < NUM_TERMINATOR) {
        if (buf[0] == ',' && buf[1] == 'a') {
          ++buf;
          line[i] = Action{'a', 0};
        } else {
          sscanf(buf, "%*c%c%d", &type, &t);
          ++buf;
          line[i] = Action{type, t};
        }
        continue;
      }
      sscanf(buf, "%*c%d", &t);
      ++buf;
      if (transtab[ln].size() == 0) {
        transtab[ln] = std::vector<int>(NUM_SYMBOL, 0);
      }
      transtab[ln][i] = t;
#undef NUM_SYMBOL
#undef NUM_TERMINATOR
#undef NUM_NONTERMINATOR
    }
  }

  void UpdateNextSymbol() {
    while (*iptr && isblank(*iptr)) {
      ++iptr;
    }
    if (*iptr == 0) {
      nsym = NONE;
      return;
    }
    if (isalpha(*iptr)) {
      nsym = VAR;
      vars.insert(*iptr);
      return;
    }
    switch (*iptr) {
      case '0':
      case '1':
        nsym = VAR;
        break;

      case '(':
        nsym = LB;
        break;

      case ')':
        nsym = RB;
        break;

      case '=':
        nsym = IFF;
        break;

      case '-':
        nsym = FI;
        break;

      case '+':
        nsym = OR;
        break;

      case '*':
        nsym = AND;
        break;

      case '!':
        nsym = NOT;
        break;

      default:
        error = __LINE__;
        __PrintError("Unknown Token `%c' at %ld\n", *iptr, iptr - sptr);
        break;
    }
  }

  int DealWithNot() {
    char* next = iptr + 1;
    while (*next && isblank(*next)) {
      ++next;
    }
    if (*next == 0 || (*next != '1' && *next != '0' && !isalpha(*next)) &&
                          *next != '!' && *next != '(')
      return 1;
    if (symstk.size() && symstk[-1] == NOT) {
      symstk.pop();
    } else {
      symstk += NOT;
    }
    return 0;
  }

  void JoinNode(Symbol sym, int len) {
    if (nodstk.size() < len) {
      error = __LINE__;
      __PrintError(
          "Nodes in `nodstk' not enough:\n"
          "  actual = %ld, excepted = %d\n",
          nodstk.size(), len);
      return;
    }
    if (len == 1) {
      return;
    }
    if (nodstk[-1]->sym == RB) {
      auto p = nodstk[-2];
      if (nodstk[-3]->rev) {
        p->rev = !p->rev;
      }
      p->str = nodstk[-3]->str + p->str + nodstk[-1]->str;
      nodstk.pop(3);
      nodstk += p;
      return;
    }
    auto p = nodstk[-2];
    p->left = nodstk[-3];
    p->right = nodstk[-1];
    p->str = nodstk[-3]->str + p->str + nodstk[-1]->str;
    nodstk.pop(3);
    nodstk += p;
  }

  void Run() {
    int t;
    while (error == 0) {
      UpdateNextSymbol();
      if (nsym == NOT) {
        if (DealWithNot()) {
          PrintError();
          return;
        }
        ++iptr;
        continue;
      }
      auto act = acttab[statstk[-1]][nsym];
      switch (act.type) {
        case 'a':  // accept
          __PrintInfo("Accepted\n");
          return;
          break;

        case 'r':  // reduce
#define prod prodlst[act.tag]
          t = prod.right.size();
          JoinNode(prod.left, t);
          symstk.pop(t);
          statstk.pop(t);
          symstk += prod.left;
          statstk += transtab[statstk[-1]][symstk[-1]];
          __PrintInfo(
              "Produce: `%s' (%d)."
              " Reduce: `%s'. @ %ld\n",
              prod.str.c_str(), act.tag, nodstk[-1]->str.c_str(), iptr - sptr);
          break;
#undef prod

        case 's':  // push in
          nodstk += new TreeNode(nsym);
          nodstk[-1]->str += *iptr;
          if (symstk.size() && symstk[-1] == NOT) {
            symstk.pop();
            nodstk[-1]->rev = true;
            nodstk[-1]->str = SymbolInfo[NOT].str + nodstk[-1]->str;
          }
          symstk += nsym;
          statstk += act.tag;
          ++iptr;
          __PrintInfo(
              "Push in: `%c' (%s)."
              " State: %d -> %d. @ %ld\n",
              iptr[-1], SymbolInfo[nsym].str, statstk[-2], statstk[-1],
              iptr - sptr);

          break;

        case 'e':  // error
        default:
          PrintError();
          return;
          break;
      }
    }
  }
#undef PrintError
};

#endif /* PARSER_HPP */
