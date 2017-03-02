//
// test_fast_vector_append.c: test and example for fast_vector_append.h
// by pts@fazekas.hu at Mon Feb 27 17:37:48 CET 2017
//
// Example usage: g++ -std=c++0x -DFAST_VECTOR_APPEND_DEBUG -s -O2 -W -Wall test_fast_vector_append.cc && ./a.out >cxx11.out && diff cxx11_gxx48.exp cxx11.out
// Example usage: g++ -ansi      -DFAST_VECTOR_APPEND_DEBUG -s -O2 -W -Wall test_fast_vector_append.cc && ./a.out >cxx98.out && diff cxx98_gxx48.exp cxx98.out
//

//#define FAST_VECTOR_APPEND_DEBUG
#include "fast_vector_append.h"

#include <stdio.h>

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// A modern, C++11 class with a move constructor.
class C {
 public:
  C() { puts("C()"); }
  C(int) { puts("C(int)"); }
  C(int, int) { puts("C(int, int)"); }
  ~C() { puts("~C()"); }
  C(const C&) { puts("C(const C&)"); }
#ifdef FAST_VECTOR_APPEND_USE_CXX11
  C(C&&) { puts("C(C&&)"); }
#endif
  C& operator=(const C&) { puts("C="); return *this; }
  // No member swap, new classes shouldn't have it.
  // But we do add member swap if compiled over C++98,
  // for faster appending in some cases.
#ifndef FAST_VECTOR_APPEND_USE_CXX11
  void swap(C&) { puts("C.swap"); }
#endif
  friend void swap(C&, C&) { puts("C12.swap"); }
};

// This doesn't work work with `C(C&&) = delete;'. Sigh.
static inline C new_c(int i) { return i; }

// A legacy, C++98 class with member swap and no move constructor.
class L {
 public:
  L() { puts("L()"); }
  L(int) { puts("L(int)"); }
  L(int, int) { puts("L(int, int)"); }
  ~L() { puts("~L()"); }
  L(const L&) { puts("L(const L&)"); }
  L& operator=(const L&) { puts("L="); return *this; }
  void swap(L&) { puts("L.swap"); }
  friend void swap(L&, L&) { puts("L12.swap"); }
};

// This doesn't work work with `C(C&&) = delete;'. Sigh.
static inline L new_l(int i) { return i; }

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  // The ``fast'' applies to C++11 callers only.
  // TODO(pts): Describe C++98 callers as well.
  puts("---C0");
  std::vector<C> v;
  v.reserve(20);  // Prevent reallocation: C(const C&) + ~C() for old elements.
  puts("---C1");
  fast_vector_append(v, C(42));  // Fast: does a move.
#ifdef FAST_VECTOR_APPEND_USE_CXX11
  puts("---C2");
  v.emplace_back(42);  // Fastest.
  puts("---C3");
  v.emplace_back(C(42));  // Fast: does a move.
#endif
  puts("---C4");
  { C c(42); fast_vector_append_move(v, c); }  // Fast: does a move.
  puts("---C5");
  fast_vector_append(v, new_c(42));  // Fast: does a move.
  puts("---C6");
  fast_vector_append(v, 42);  // Fastest: uses A9.
  puts("---C7");
  fast_vector_append(v, 4, 2);  // Fastest: uses A9.
  puts("---C8SLOWOK");
  { const C& cr(42); fast_vector_append(v, cr); }  // Slow: does a copy.
  puts("---C9SLOWOK");
  { C c(42); fast_vector_append(v, c); }  // Slow: does a copy.
  puts("--C10");
  fast_vector_append(v);  // Fastest: uses A9.

  puts("---L0");
  std::vector<L> w;
  w.reserve(20);  // Prevent reallocation: C(const C&) + ~C() for old elements.
  puts("---L1");
  fast_vector_append(w, L(42));  // Fast: uses swap.
#ifdef FAST_VECTOR_APPEND_USE_CXX11
  puts("---L2");
  w.emplace_back(42);  // Fastest.
  puts("---L3");
  w.emplace_back(L(42));  // Slow: does a copy.
#endif
  puts("---L4");
  { L l(42); fast_vector_append_move(w, l); }  // Fast: uses swap.
  puts("---L5");
  fast_vector_append(w, new_l(42));  // Fast: uses swap.
  puts("---L6");
  fast_vector_append(w, 42);  // Fastest: uses O9.
  puts("---L7");
  fast_vector_append(w, 4, 2);  // Fastest: uses O9.
  puts("---L8SLOWOK");
  { const L& lr(42); fast_vector_append(w, lr); }  // Slow: does a copy.
  puts("---L9SLOWOK");
  { L l(42); fast_vector_append(w, l); }  // Slow, does a copy.
  puts("--L10");
  fast_vector_append(w);  // Fastest: uses A9.

  puts("---INT");
  std::vector<int> vi;
  fast_vector_append(vi, 42);

  puts("---RETURN");
  return 0;
}
