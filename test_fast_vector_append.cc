//
// test_push_back_auto.cc: Doing std::vector::push_back without copies
// by pts@fazekas.hu at Mon Feb 27 17:37:48 CET 2017
//
// This C++ header helps you avoid unnecessary copying when appending to a
// vector (std::vector::push_back). In C++11, push_back takes a T&&
// (rvalue reference), so it won't be doing expensive copies e.g. with
// v.push_back(my_function()), but it will invoke the move constructor
// of the value returned by my_function(), which is typically much faster
// than the copy constructor (e.g. for long std::string values). In C++98
// there are no rvalue references, and unnecessary copies happen much more
// often. Fortunately it's possible to prevent most of them by using
// `v.back().swap(...)'.
//
// This C++ header automates such calls. You call `append(v, ...)', and it
// gets translated to `v.push_back(...)', `v.emplace_back(...)' or
// `v.back().swap(...)', whichever is fastest. It supports C++11
// (recommended, activate it with g++ -std=c++0x, uses the move constructor
// if available) and C++98 (activate it with g++ -ansi, can still avoid
// copies in some cases, using swap). It works with built-in types (e.g.
// int), STL types (e.g. std::string, std::set) and user-defined classes.
// It also works when compiling C++98 classes in C++11 mode.
//
// To aid autodetection with user-defined classes, follow the class defining
// guidelines:
//
// * Define the default constructor (`C() { ... }').
// * Define the destructor (`~C() { ... }').
// * Define the copy constructor (`C(const C& c) { ... }').
// * It's a good practice to define operator=, but not needed here.
// * For C++11 classes, define a move constructor (e.g. C(C&& c) { ... }).
// * For C++11 classes, don't define a member .swap. If you must define
//   it, then also define a method `void shrink_to_fit() { ... }'. It
//   doesn't matter what the metod does, you can just declare it.
// * For C++98 classes, don't define a move constructor. In fact, C++98
//   doesn't support move constructors.
// * For C++98 classes, define a member .swap.
//
// In general you should use g++-4.8 or later (or clang++-3.4 or later),
// because older STLs (such as libstdc++ 4.4.3 with g++4.4) tend to do
// 2 additional copy constructor calls (on the empty object) in v.resize()
// even in C++11 mode.
//
// If you don't want to use this header, but you are willing to do some
// manual work, here is how you can avoid copies when appending to an
// std::vector. Follow the guidelines above when defining your classes,
// then  follow this advice from top to bottom to avoid the call to
// the slow copy constructor:
//
// * If it's C++11 mode, and the object is being constructed (not returned
//   by a function!), use emplace_back without C(...).
// * If it's C++11 mode, and the class has a move constructor, use
//   push_back.
// * If it's C++11 mode, and the class has member .swap, use:
//   { C c(42); v.resize(v.size() + 1); v.back().swap(c); }
// * If the class has member .swap, use:
//   { C c(42); v.push_back(C()); v.back().swap(c); }
// * Use push_back. (This is the only case with a slow copy.)
//

#include <stdio.h>

#undef USE_CXX11
// Maybe _MSC_VER >= 1800 &&  __cplusplus == 199711L also works.
// g++-4.4 has only __GXX_EXPERIMENTAL_CXX0X__.
// g++-4.8 has both __cplusplus >= 201103L and __GXX_EXPERIMENTAL_CXX0X__.
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#define USE_CXX11
#endif

#ifdef USE_CXX11

#include <type_traits>  // std::is_move_constructible, std::enable_if etc.

#include <utility>  // For the declaration of std::get.

// Use swap iff: has swap, does't have std::get, doesn't have shrink_to_fit,
// doesn't have emplace, doesn't have remove_suffix. By doing so we match
// all C++11, C++14 and C++17 STL templates except for std::optional and
// std::any. Not matching a few of them is not a problem because then member
// .swap will be used on them, and that's good enough.
//
// Based on HAS_MEM_FUNC in http://stackoverflow.com/a/264088/97248 .
// Based on decltype(...) in http://stackoverflow.com/a/6324863/97248 .
template<typename T>
struct __aph_use_swap {
  template <typename U, U> struct type_check;
  // This also checks the return type of swap (void). The checks with
  // decltype below don't check the return type.
  template <typename B> static char (&chk_swap(type_check<void(B::*)(B&), &B::swap>*))[2];
  template <typename  > static char (&chk_swap(...))[1];
  template <typename B> static char (&chk_get(decltype(std::get<0>(*(B*)0), 0)))[1];  // C++11 only: std::pair, std::tuple, std::variant, std::array.
  template <typename  > static char (&chk_get(...))[2];
  template <typename B> static char (&chk_s2f(decltype(((B*)0)->shrink_to_fit(), 0)))[1];  // C++11 only: std::vector, std::deque, std::string, std::basic_string.
  template <typename  > static char (&chk_s2f(...))[2];
  template <typename B> static char (&chk_empl(decltype(((B*)0)->emplace(), 0)))[1];  // C++11 only: std::vector, std::deque, std::set, std::multiset, std::map, std::multimap, std::unordered_multiset, std::unordered_map, std::unordered_multimap, std::stack, std::queue, std::priority_queue.
  template <typename  > static char (&chk_empl(...))[2];
  template <typename B> static char (&chk_rsuf(decltype(((B*)0)->remove_suffix(0), 0)))[1];  // C++17 only: std::string_view, std::basic_string_view.
  template <typename  > static char (&chk_rsuf(...))[2];
  static bool const value = sizeof(chk_swap<T>(0)) == 2 && sizeof(chk_get<T>(0)) == 2 && sizeof(chk_s2f<T>(0)) == 2 && sizeof(chk_empl<T>(0)) == 2 && sizeof(chk_rsuf<T>(0)) == 2;
};

// TODO(pts): Maybe use remove_const on V::value_type? Add tests.
template<typename V, typename T> static inline
typename std::enable_if<std::is_same<typename V::value_type, T>::value && __aph_use_swap<typename V::value_type>::value, void>::type
append(V& v, T&& t) { puts("A3"); v.resize(v.size() + 1); v.back().swap(t); }

template<typename V, typename T> static inline
typename std::enable_if<std::is_same<typename V::value_type, T>::value && !__aph_use_swap<typename V::value_type>::value, void>::type
append(V& v, T&& t) { puts("A4"); v.push_back(std::move(t)); }

template<typename V, typename T> static inline
typename std::enable_if<std::is_same<typename V::value_type, T>::value, void>::type
append(V& v, const T& t) { puts("A7SLOW"); v.push_back(t); }  // Fallback, slow.

template<typename V, typename... Args> static inline
void append(V& v, Args... args) { puts("A9"); v.emplace_back(args...); }  // Fastest.


template<typename V, typename T> static inline
typename std::enable_if<std::is_same<typename V::value_type, T>::value && __aph_use_swap<typename V::value_type>::value, void>::type
append_move(V& v, T& t) { puts("AM1"); v.resize(v.size() + 1); v.back().swap(t); }

template<typename V, typename T> static inline
typename std::enable_if<std::is_same<typename V::value_type, T>::value && __aph_use_swap<typename V::value_type>::value, void>::type
append_move(V& v, T&& t) { puts("AM3"); v.resize(v.size() + 1); v.back().swap(t); }

template<typename V, typename T> static inline
typename std::enable_if<std::is_same<typename V::value_type, T>::value && !__aph_use_swap<typename V::value_type>::value, void>::type
append_move(V& v, T&& t) { puts("AM4"); v.push_back(std::move(t)); }

template<typename V, typename T> static inline
typename std::enable_if<std::is_same<typename V::value_type, T>::value && !__aph_use_swap<typename V::value_type>::value, void>::type
append_move(V& v, T& t) { puts("AM5"); v.push_back(std::move(t)); }

#else  // C++98. Works in g++-4.4 in -ansi (C++98) mode.

template<bool B, typename T = void>
struct __aph_enable_if {};
template<typename T>
struct __aph_enable_if<true, T> { typedef T type; };

template<class T, class U>
struct __aph_is_same { static bool const value = false; };
template<class T>
struct __aph_is_same<T, T> { static bool const value = true; };

template<typename T>
struct __aph_use_swap {
  template <typename U, U> struct type_check;
  template <typename B> static char (&chk_swap(type_check<void(B::*)(B&), &B::swap>*))[1];
  template <typename  > static char (&chk_swap(...))[2];
  static bool const value = sizeof(chk_swap<T>(0)) == sizeof(char[1]);
};

template<typename V, typename T> static inline
typename __aph_enable_if<__aph_is_same<typename V::value_type, T>::value && __aph_use_swap<typename V::value_type>::value, void>::type
append(V& v, T& t) { puts("O1"); v.push_back(T()); v.back().swap(t); }

template<typename V, typename T> static inline
typename __aph_enable_if<__aph_is_same<typename V::value_type, T>::value && !__aph_use_swap<typename V::value_type>::value, void>::type
append(V& v, T& t) { puts("O5SLOW"); v.push_back(t); }  // Slow.

template<typename V, typename T> static inline
typename __aph_enable_if<__aph_is_same<typename V::value_type, T>::value || !__aph_use_swap<typename V::value_type>::value, void>::type
append(V& v, const T& t) { puts("O7SLOW"); v.push_back(t); }  // Fallback, slow.

template<typename V, typename T> static inline
typename __aph_enable_if<!__aph_is_same<typename V::value_type, T>::value && __aph_use_swap<typename V::value_type>::value, void>::type
append(V& v, const T& t) { puts("O9"); v.push_back(typename V::value_type()); typename V::value_type tt(t); v.back().swap(tt); }

template<typename V> static inline
void append(V& v) { puts("O9D"); v.push_back(typename V::value_type()); }

template<typename V, typename T1, typename T2> static inline
typename __aph_enable_if<__aph_use_swap<typename V::value_type>::value, void>::type
append(V& v, const T1& t1, const T2& t2) { puts("O9OO"); v.push_back(typename V::value_type()); typename V::value_type tt(t1, t2); v.back().swap(tt); }

template<typename V, typename T1, typename T2, typename T3> static inline
typename __aph_enable_if<__aph_use_swap<typename V::value_type>::value, void>::type
append(V& v, const T1& t1, const T2& t2, const T3& t3) { puts("O9OOO"); v.push_back(typename V::value_type()); typename V::value_type tt(t1, t2, t3); v.back().swap(tt); }

template<typename V, typename T1, typename T2, typename T3, typename T4> static inline
typename __aph_enable_if<__aph_use_swap<typename V::value_type>::value, void>::type
append(V& v, const T1& t1, const T2& t2, const T3& t3, const T4& t4) { puts("O9OOOO"); v.push_back(typename V::value_type()); typename V::value_type tt(t1, t2, t3, t4); v.back().swap(tt); }


template<typename V, typename T> static inline
typename __aph_enable_if<__aph_is_same<typename V::value_type, T>::value && __aph_use_swap<typename V::value_type>::value, void>::type
append_move(V& v, T& t) { puts("O1"); v.push_back(T()); v.back().swap(t); }

template<typename V, typename T> static inline
typename __aph_enable_if<__aph_is_same<typename V::value_type, T>::value && !__aph_use_swap<typename V::value_type>::value, void>::type
append_move(V& v, T& t) { puts("O5SLOW"); v.push_back(t); }  // Slow.

#endif

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

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

class C {
 public:
  C() { puts("C()"); }
  C(int) { puts("C(int)"); }
  C(int, int) { puts("C(int, int)"); }
  ~C() { puts("~C()"); }
  C(const C&) { puts("C(const C&)"); }
#ifdef USE_CXX11
  C(C&&) { puts("C(C&&)"); }
#endif
  C& operator=(const C&) { puts("C="); return *this; }
  // No member swap, new classes shouldn't have it.
  // But we do add member swap if compiled over C++98,
  // for faster append in some cases.
#ifndef USE_CXX11
  void swap(C&) { puts("C.swap"); }
#endif
  friend void swap(C&, C&) { puts("C12.swap"); }
};

// This doesn't work work with `C(C&&) = delete;'. Sigh.
static inline C new_c(int i) { return i; }

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  // The ``fast'' applies to C++11 callers only.
  // TODO(pts): Describe C++98 callers as well.
  puts("---C0");
  std::vector<C> v;
  v.reserve(20);  // Prevent reallocation: C(const C&) + ~C() for old elements.
  puts("---C1");
  append(v, C(42));  // Fast: does a move.
#ifdef USE_CXX11
  puts("---C2");
  v.emplace_back(42);  // Fastest.
  puts("---C3");
  v.emplace_back(C(42));  // Fast: does a move.
#endif
  puts("---C4");
  { C c(42); append_move(v, c); }  // Fast: does a move.
  puts("---C5");
  append(v, new_c(42));  // Fast: does a move.
  puts("---C6");
  append(v, 42);  // Fastest: uses A9.
  puts("---C7");
  append(v, 4, 2);  // Fastest: uses A9.
  puts("---C8SLOWOK");
  { const C& cr(42); append(v, cr); }  // Slow: does a copy.
  puts("---C9SLOWOK");
  { C c(42); append(v, c); }  // Slow: does a copy.
  puts("--C10");
  append(v);  // Fastest: uses A9.

  puts("---L0");
  std::vector<L> w;
  w.reserve(20);  // Prevent reallocation: C(const C&) + ~C() for old elements.
  puts("---L1");
  append(w, L(42));  // Fast: uses swap.
#ifdef USE_CXX11
  puts("---L2");
  w.emplace_back(42);  // Fastest.
  puts("---L3");
  w.emplace_back(L(42));  // Slow: does a copy.
#endif
  puts("---L4");
  { L l(42); append_move(w, l); }  // Fast: uses swap.
  puts("---L5");
  append(w, new_l(42));  // Fast: uses swap.
  puts("---L6");
  append(w, 42);  // Fastest: uses O9.
  puts("---L7");
  append(w, 4, 2);  // Fastest: uses O9.
  puts("---L8SLOWOK");
  { const L& lr(42); append(w, lr); }  // Slow: does a copy.
  puts("---L9SLOWOK");
  { L l(42); append(w, l); }  // Slow, does a copy.
  puts("--L10");
  append(w);  // Fastest: uses A9.

  puts("---INT");
  std::vector<int> vi;
  append(vi, 42);

  puts("---RETURN");
  return 0;
}
