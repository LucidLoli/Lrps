#pragma once
#ifndef STACK_HPP

#include <cstring>

#include "./dbg.hpp"

template <typename T>
struct Stack {
  T* mem;
  int len;
  int cap;

  Stack(int cap = 32) : cap(cap) {
    mem = new T[cap];
    len = 0;
  }

  Stack(const Stack& old) {
    len = old.len;
    cap = old.cap;
    mem = new T[cap];
    memcpy(mem, old.mem, len * sizeof(T));
  }

  Stack(Stack&& old) {
    len = old.len;
    cap = old.cap;
    mem = old.mem;
    old.mem = nullptr;
    old.len = old.cap = 0;
  }

  ~Stack() { delete[] mem; }

  void push(const T& obj) {
    if (cap <= len) {
      T* nptr = new T[cap + (cap >> 1) + 1];
      memcpy(nptr, mem, len * sizeof(T));
      delete[] mem;
      mem = nptr;
      cap *= 1.5;
    }
    mem[len++] = obj;
  }

  const T& top() const { return mem[len - 1]; }

  const T& operator[](int x) const {
    return x > -1 ? mem[x] : len + x > -1 ? mem[len + x] : mem[0];
  }

  size_t size() const { return len; }

  void operator+=(const T& obj) { push(obj); }

  void pop(int x = 1) {
    if (x < 1) return;
    if (cap > 16 && cap > 3 * len) {
      T* nptr = new T[cap - (cap >> 1) + 1];
      memcpy(nptr, mem, len * sizeof(T));
      delete[] mem;
      mem = nptr;
      cap *= 0.5;
    }
    if (x > len) x = len;
    len -= x;
  }
};

#endif /* STACK_HPP */
