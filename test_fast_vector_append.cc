/* by pts@fazekas.hu at Mon Feb 27 17:37:48 CET 2017 */

#include <stdio.h>

#include <vector>

#undef USE_CXX11
/* Maybe _MSC_VER >= 1800 &&  __cplusplus == 199711L also works. */
#if __cplusplus >= 201103L
#define USE_CXX11
#endif

// A legacy, C++98 class with member swap and no move constructor.
class L {
 public:
  L() { puts("L()"); }
  L(int) { puts("L(int)"); }
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

// TODO(pts): Which old C++98 compilers don't support this?
#define HAS_MEM_FUNC(func, name)                                        \
    template<typename T, typename Sign>                                 \
    struct name {                                                       \
        typedef char yes[1];                                            \
        typedef char no [2];                                            \
        template <typename U, U> struct type_check;                     \
        template <typename _1> static yes &chk(type_check<Sign, &_1::func > *); \
        template <typename   > static no  &chk(...);                    \
        static bool const value = sizeof(chk<T>(0)) == sizeof(yes);     \
    }

HAS_MEM_FUNC(swap, has_swap);

#ifdef USE_CXX11

#include <type_traits>  // std::is_move_constructible, std::enable_if etc.

// !! doc: .append may modify its t argument.

template<class V, class T>
typename std::enable_if<has_swap<T, void(T::*)(T&)>::value, void>::type
append(V& v, T& t) { puts("A1"); v.resize(v.size() + 1); v.back().swap(t); }

template<class V, class T>
typename std::enable_if<has_swap<T, void(T::*)(T&)>::value, void>::type
append(V& v, T&& t) { puts("A3"); v.resize(v.size() + 1); v.back().swap(t); }

template<class V, class T>
typename std::enable_if<!has_swap<T, void(T::*)(T&)>::value, void>::type
append(V& v, T&& t) { puts("A4"); v.push_back(std::move(t)); }

template<class V, class T>
typename std::enable_if<!has_swap<T, void(T::*)(T&)>::value, void>::type
append(V& v, T& t) { puts("A5"); v.push_back(std::move(t)); }

//template<class V, class T>
//typename std::enable_if<has_swap<T, void(T::*)(T&)>::value, void>::type
//append(V& v, const T& t) { puts("A2"); v.push_back(t); }
//template<class V, class T>
//typename std::enable_if<!has_swap<T, void(T::*)(T&)>::value, void>::type
//append(V& v, const T& t) { puts("A6"); v.push_back(t); }

template<class V, class T>
void append(V& v, const T& t) { puts("A7"); v.push_back(t); }  // Fallback, slow.

#else  // C++98.

// !! Use my_enable_if everywhere.

template<bool B, class T = void>
struct my_enable_if {};
 
template<class T>
struct my_enable_if<true, T> { typedef T type; };

template<class V, class T>
typename my_enable_if<has_swap<T, void(T::*)(T&)>::value, void>::type
append(V& v, T& t) { puts("O1"); v.push_back(T()); v.back().swap(t); }

template<class V, class T>
typename my_enable_if<!has_swap<T, void(T::*)(T&)>::value, void>::type
append(V& v, T& t) { puts("O5"); v.push_back(t); }  // Slow.

template<class V, class T>
void append(V& v, const T& t) { puts("O7"); v.push_back(t); }  // Fallback, slow.

#endif

// !! Update the doc.
//
// If your class C has a copy constructor which is slow unless the object
// is empty (i.e. C()), and you want to add a new object of type C to an
// std::vector, follow this advice from top to bottom to avoid the call to
// the slow copy constructor:
//
// * If it's C++11, and the object is being constructed (not returned by a
//   function!), use emplace_back without C(...).
// * If it's C++11 and the class has a move constructor, use push_back.
// * If it's C++11 and the class has .swap, use:
//   { C c(42); v.resize(v.size() + 1); v.back().swap(c); }
// * If the class has .swap, use:
//   { C c(45); v.push_back(C()); v.back().swap(c); }
// * Use push back. (This is the only case when it will be a slow copy.)

int main(int argc, char **argv) {
  (void)argc; (void)argv;
  puts("---C0");
  std::vector<C> v;
  v.reserve(20);  // Prevent reallocation: C(const C&) + ~C() for old elements.
  // append(v, 42);  // SUXX: Doesn't work, even thouh v.push_back(42) does.  !! Make it work.
  puts("---C1");
  append(v, C(42));  // Fast. Does a move.
#ifdef USE_CXX11
  puts("---C2");
  v.emplace_back(42);  // Fastest.
  puts("---C3");
  v.emplace_back(C(42));  // Fast: does a move.
#endif
  puts("---C4");
  { C c(42); append(v, c); }  // Fast. Does a move.
  puts("---C5");
  append(v, new_c(42));  // Fast. Does a move.

  puts("---L0");
  std::vector<L> w;
  w.reserve(20);  // Prevent reallocation: C(const C&) + ~C() for old elements.
  puts("---L1");
  append(w, L(42));  // Fast. Uses swap.
#ifdef USE_CXX11
  puts("---L2");
  w.emplace_back(42);  // Fastest.
  puts("---L3");
  w.emplace_back(L(42));  // Slow, does a copy.
#endif
  puts("---L4");
  { L l(42); append(w, l); }  // Fast. Uses swap.
  puts("---L5");
  append(w, new_l(42));  // Fast. Uses swap.
  
  puts("---RETURN");
  return 0;
}
