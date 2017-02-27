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


class C {
 public:
  C() { puts("C()"); }
  C(int) { puts("C(int)"); }
  ~C() { puts("~C()"); }
  C(const C&) { puts("C(const C&)"); }
#if defined(USE_CXX11) && defined(USE_MOVE)
  C(C&&) { puts("C(C&&)"); }
#else
  //C(C&&) = delete;
#endif
  C& operator=(const C&) { puts("C="); return *this; }
  // No member swap, new classes shouldn't have it.
  // void swap(C&) { puts("C.swap"); }
  friend void swap(C&, C&) { puts("C12.swap"); }
};

// This doesn't work work with `C(C&&) = delete;'. Sigh.
static inline C new_c(int i) { return i; }

#ifdef USE_CXX11
#include <type_traits>  // std::is_move_constructible, std::enable_if etc.

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

// !! doc: .append may modify its t argument.

template<class V, class T>
typename std::enable_if<has_swap<T, void(T::*)(T&)>::value, void>::type
append(V& v, T& t) { puts("A1"); v.resize(v.size() + 1); v.back().swap(t); }

// !! With L (has_swap is true), wh

// !! TODO(pts): Why is remove_const needed here? otherwise it wouldn't match, but why?
template<class V, class T>
typename std::enable_if<!has_swap<T, void(T::*)(std::remove_const<T>&)>::value, void>::type
append(V& v, T&& t) { puts("A2"); v.push_back(std::move(t)); }

template<class V, class T>
typename std::enable_if<!has_swap<T, void(T::*)(T&)>::value, void>::type
append(V& v, T& t) { puts("A3"); v.push_back(std::move(t)); }

template<class V, class T>
typename std::enable_if<!has_swap<T, void(T::*)(std::remove_const<T>&)>::value, void>::type
append(V& v, const T& t) { puts("A4"); v.push_back(t); }

#else  // C++98.

template<class V, class T>
void append(V& v, const Y& t) { v.push_back(t); }

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
  // append(v, 42);  // SUXX: Doesn't work, even thouh v.push_back(42) does.  !! Make it fast.
  puts("---C1");
  append(v, C(42));  // Fast. Uses the move constructor.
#ifdef USE_CXX11
  puts("---C2");
  v.emplace_back(42);  // Fastest.
#endif
  puts("---C3");
  { C c(42); append(v, c); }


  puts("---L0");
  std::vector<L> w;
  w.reserve(20);  // Prevent reallocation: C(const C&) + ~C() for old elements.
  puts("---L1");
  append(w, L(42));  // Slow.  !! Why A2 here?
#ifdef USE_CXX11
  puts("---L2");
  w.emplace_back(42);  // Fastest.
#endif
  puts("---L3");
  { L l(42); append(w, l); }
  
  puts("---RETURN");
  return 0;
}
