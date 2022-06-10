#pragma once
#ifndef SEARCHER_HPP

#include <set>

#include "./dbg.hpp"
#include "./grammar.hpp"
#include "./stack.hpp"

using Selection = std::array<uint32_t, 2>;

struct Searcher {
  Searcher(int numnod, TreeNode* tree, std::set<char> vars)
      : root(tree),
        vars(vars),
        numvar(vars.size()),
        numnod(numnod),
        decs(numnod, std::array<std::vector<Selection>, 2>()),
        req(numnod, 0),
        cst(numnod, -1) {
    memset(tstab, 0xff, sizeof(tstab));
    int a = 0;
    for (auto& v : vars) {
      tstab[v] = a;
      tstab[a] = v;
      ++a;
    }
    if (root != nullptr) Run();
  }

  std::set<Selection> GetTrueSelectionSet() const {
    if (taut) {
      std::set<Selection> ret;
      ret.insert(Selection{0, 0});
      return std::move(ret);
    }
    return truesels;
  }

 private:
  int const numvar;
  int numnod;
  bool cont = false;
  bool taut = false;

  int tstab[0xff];
  TreeNode* root;
  std::set<char> vars;
  std::vector<std::array<std::vector<Selection>, 2> > decs;
  std::vector<int> req;
  std::vector<int> cst;
  std::set<Selection> truesels;

  void FindConstant() {
#define left (*cur.left)
#define right (*cur.right)
    Stack<TreeNode*> nstk;
    std::vector<bool> vis(numnod, 0);
    nstk += root;
    while (nstk.size()) {
      auto& cur = *nstk[-1];
      if (cur.IsLeaf()) {
        if (isdigit(cur.token)) cst[cur] = (cur.rev ^ (cur.token - '0'));
        nstk.pop();
        continue;
      }

      if (vis[cur]) {
        switch (cur.sym) {
          case IFF:
            if (cst[left] < 0 || cst[right] < 0) break;
            cst[cur] = (cst[left] == cst[right]);
            break;

          case FI:
            if (cst[left] == 0 || cst[right] == 1)
              cst[cur] = 1;
            else if (cst[left] == 1 && cst[right] == 0)
              cst[cur] = 0;
            break;

          case OR:
            if (cst[left] == 1 || cst[right] == 1)
              cst[cur] = 1;
            else if (cst[left] == 0 && cst[right] == 0)
              cst[cur] = 0;
            break;

          case AND:
            if (cst[left] == 0 || cst[right] == 0)
              cst[cur] = 0;
            else if (cst[left] == 1 && cst[right] == 1)
              cst[cur] = 1;
            break;
        }

        if (cst[cur] > -1 && cur.rev) cst[cur] = 1 - cst[cur];

        nstk.pop();
        continue;
      }

      vis[cur] = 1;
      nstk += &left;
      nstk += &right;
    }
#undef left
#undef right
  }

  void DeployRequest() {
    using namespace std;

    if (cst[*root] > -1) return;

    Stack<TreeNode*> nstk;

    req[*root] = 2;
    nstk += root;

    while (nstk.size()) {
#define left (*cur.left)
#define right (*cur.right)
      auto& cur = *nstk[-1];
      nstk.pop();
      if (cur.rev) req[cur] = ((req[cur] & 2) >> 1) | ((req[cur] & 1) << 1);

      if (cur.IsLeaf()) continue;

      if (req[cur] & 1) {
        switch (cur.sym) {
          case IFF:
            if (cst[left] > -1) {
              req[right] |= 1 << (1 ^ cst[left]);
            } else if (cst[right] > -1) {
              req[left] |= 1 << (1 ^ cst[right]);
            } else {
              req[left] |= 3;
              req[right] |= 3;
            }
            break;

          case FI:
            if (cst[left] > -1) {
              req[right] |= 1;
            } else if (cst[right] > -1) {
              req[left] |= 2;
            } else {
              req[left] |= 2;
              req[right] |= 1;
            }
            break;

          case OR:
            if (cst[left] < 0) {
              req[left] |= 1;
            }
            if (cst[right] < 0) {
              req[right] |= 1;
            }
            break;

          case AND:
            if (cst[left] < 0) {
              req[left] |= 1;
            }
            if (cst[right] < 0) {
              req[right] |= 1;
            }
            break;
        }
      }
      if (req[cur] & 2) {
        switch (cur.sym) {
          case IFF:
            if (cst[left] > -1) {
              req[right] |= 1 << (cst[left]);
            } else if (cst[right] > -1) {
              req[left] |= 1 << (cst[right]);
            } else {
              req[left] |= 3;
              req[right] |= 3;
            }
            break;

          case FI:
            if (cst[left] > -1) {
              req[right] |= 2;
            } else if (cst[right] > -1) {
              req[left] |= 1;
            } else {
              req[left] |= 1;
              req[right] |= 2;
            }
            break;

          case OR:
            if (cst[left] < 0) {
              req[left] |= 2;
            }
            if (cst[right] < 0) {
              req[right] |= 2;
            }
            break;

          case AND:
            if (cst[left] < 0) {
              req[left] |= 2;
            }
            if (cst[right] < 0) {
              req[right] |= 2;
            }
            break;
        }
      }

      if (cst[left] < 0) nstk += &left;
      if (cst[right] < 0) nstk += &right;
    }

#undef left
#undef right
  }

  Selection And(Selection const& a, Selection const& b) {
    if ((a[0] & b[1]) || (a[1] & b[0]))
      return std::move(Selection{0xffffffff, 0xffffffff});
    return std::move(Selection{a[0] | b[0], a[1] | b[1]});
  }

  void Select(TreeNode& n) {
    using namespace std;

    if (n.IsLeaf()) {
      uint32_t d = 1 << tstab[n.token];
      decs[n][n.rev].emplace_back(Selection{d, 0});
      decs[n][!n.rev].emplace_back(Selection{0, d});
      return;
    }

#define dn (decs[n])
#define left (*n.left)
#define right (*n.right)
#define dl (decs[left])
#define dr (decs[right])
#define apddn0(v) dn[0].insert(dn[0].end(), (v).begin(), (v).end())
#define apddn1(v) dn[1].insert(dn[1].end(), (v).begin(), (v).end())
#define anbebdn0(a, b)                             \
  do {                                             \
    auto s = And(a, b);                            \
    if (s[0] != 0xffffffff) dn[0].emplace_back(s); \
  } while (0)
#define anbebdn1(a, b)                             \
  do {                                             \
    auto s = And(a, b);                            \
    if (s[0] != 0xffffffff) dn[1].emplace_back(s); \
  } while (0)

    if (req[n] & 1) {
      switch (n.sym) {
        case IFF:
          if (cst[left] > -1) {
            apddn0(dr[1 - cst[left]]);
            break;
          }
          if (cst[right] > -1) {
            apddn0(dl[1 - cst[right]]);
            break;
          }
          for (auto& a : dl[0]) {
            for (auto& b : dr[1]) anbebdn0(a, b);
          }
          for (auto& a : dl[1]) {
            for (auto& b : dr[0]) anbebdn0(a, b);
          }
          break;

        case FI:
          if (cst[left] == 1) {
            apddn0(dr[0]);
            break;
          }
          if (cst[right] == 0) {
            apddn0(dl[1]);
            break;
          }
          for (auto& a : dl[1]) {
            for (auto& b : dr[0]) anbebdn0(a, b);
          }
          break;

        case OR:
          if (cst[left] == 0) {
            apddn0(dr[0]);
            break;
          }
          if (cst[right] == 0) {
            apddn0(dl[0]);
            break;
          }
          for (auto& a : dl[0]) {
            for (auto& b : dr[0]) anbebdn0(a, b);
          }
          break;

        case AND:
          if (cst[left] == 1) {
            apddn0(dr[0]);
            break;
          }
          if (cst[right] == 1) {
            apddn0(dl[0]);
            break;
          }
          apddn0(dl[0]);
          apddn0(dr[0]);
          break;
      }
    }
    if (req[n] & 2) {
      switch (n.sym) {
        case IFF:
          if (cst[left] > -1) {
            apddn1(dr[cst[left]]);
            break;
          }
          if (cst[right] > -1) {
            apddn1(dl[cst[right]]);
            break;
          }
          for (auto& a : dl[0]) {
            for (auto& b : dr[0]) anbebdn1(a, b);
          }
          for (auto& a : dl[1]) {
            for (auto& b : dr[1]) anbebdn1(a, b);
          }
          break;

        case FI:
          if (cst[left] == 1) {
            apddn1(dr[1]);
            break;
          }
          if (cst[right] == 0) {
            apddn1(dl[0]);
            break;
          }
          apddn1(dl[0]);
          apddn1(dr[1]);
          break;

        case OR:
          if (cst[left] == 0) {
            apddn1(dr[1]);
            break;
          }
          if (cst[right] == 0) {
            apddn1(dl[1]);
            break;
          }
          apddn1(dl[1]);
          apddn1(dr[1]);
          break;

        case AND:
          if (cst[left] == 1) {
            apddn1(dr[1]);
            break;
          }
          if (cst[right] == 1) {
            apddn1(dl[1]);
            break;
          }
          for (auto& a : dl[1]) {
            for (auto& b : dr[1]) anbebdn1(a, b);
          }
          break;
      }
    }

#undef dn
#undef dl
#undef dr
#undef left
#undef right
#undef apddn0
#undef apddn1
#undef anbebdn0
#undef anbebdn1
  }

  void Search() {
#define left (cur.left)
#define right (cur.right)
    if (root->IsLeaf()) {
      Select(*root);
      return;
    }
    Stack<TreeNode*> nstk;
    std::vector<bool> vis(numnod, 0);
    nstk += root;
    while (nstk.size()) {
      auto& cur = *nstk[-1];

      if (cst[cur] > -1) {
        nstk.pop();
        continue;
      }

      if (vis[cur]) {
        Select(cur);
        nstk.pop();
        continue;
      }

      vis[cur] = 1;

      if (left->IsLeaf()) {
        if (cst[*left] < 0) Select(*left);
      } else {
        nstk += left;
      }
      if (right->IsLeaf()) {
        if (cst[*right] < 0) Select(*right);
      } else {
        nstk += right;
      }
    }
#undef left
#undef right

    if (decs[*root][root->rev ? 0 : 1].size()) {
      auto& v = decs[*root][root->rev ? 0 : 1];
      for (auto& s : v) truesels.insert(s);
    } else if (cst[*root] == 1) {
      taut = true;
    } else {
      cont = true;
    }
  }

  void PrintBinary(FILE* fp, uint32_t a, int len = 8) {
    len = len > 32 ? 32 : len;
    for (int i = 0; i < len; i++) {
      __FprintfE(fp, "%c", (a & 1) + '0');
      a >>= 1;
    }
  }

  void Run() {
    FindConstant();
    DeployRequest();
    Search();
    auto v = decs[*root][root->rev ? 0 : 1];
    if (v.size() == 0) {
      if (taut)
        __FprintfI(stderr,
                   taut ? "\n[I] Tautology!\n" : "\n[I] Contradiction!\n");
      return;
    } else {
      __FprintfI(stderr,
                 "\n[I] The key item of the input is as following:\n > ");
    }
    for (int i = 0; i < numvar; i++) {
      __FprintfI(stderr, "%c", tstab[i]);
    }
    for (auto& d : v) {
      __FprintfI(stderr, " \n : ");
      for (int i = 0; i < numvar; i++) {
        uint32_t mask = 1 << i;
        if ((d[0] & mask) && (d[1] & mask) == 0) {
          __FprintfI(stderr, "0");
        } else if ((d[1] & mask) && (d[0] & mask) == 0) {
          __FprintfI(stderr, "1");
        } else if ((d[1] & mask) == 0 && (d[0] & mask) == 0) {
          __FprintfI(stderr, "#");
        } else {
          __FprintfE(stderr, "[E] (%s: %d) Invalid Selection { ", __FILE__,
                     __LINE__);
          PrintBinary(stderr, d[0], numvar);
          __FprintfE(stderr, ", ");
          PrintBinary(stderr, d[1], numvar);
          __FprintfE(stderr, "}\n");
          return;
        }
      }
    }
    __FprintfI(stderr, "\n\n");
  }
};

#endif /* SEARCHER_HPP */
