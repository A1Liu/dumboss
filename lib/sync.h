#ifndef __LIB_SYNC__
#define __LIB_SYNC__
#include <stdatomic.h>
#include <stddef.h>
#include <types.h>

// TODO: Add more macros for clang builtin as needed. Builtins list is here:
// https://releases.llvm.org/10.0.0/tools/clang/docs/LanguageExtensions.html
//                                      - Albert Liu, Nov 03, 2021 Wed 00:50 EDT
#define a_init(ptr_val, initial) __c11_atomic_init(ptr_val, initial)
#define a_load(obj)              __c11_atomic_load(obj, __ATOMIC_SEQ_CST)
#define a_store(obj, value)      __c11_atomic_store(obj, value, __ATOMIC_SEQ_CST)
#define a_add(obj, add)          __c11_atomic_fetch_add(obj, add, __ATOMIC_SEQ_CST)
#define a_cxweak(obj, expected, desired)                                                           \
  __c11_atomic_compare_exchange_weak(obj, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define a_cxstrong(obj, expected, desired)                                                         \
  __c11_atomic_compare_exchange_strong(obj, expected, desired, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

// NOTE: This ALSO breaks compatibility with GCC.
//                                    - Albert Liu, Nov 15, 2021 Mon 19:03 EST
#define spin_or(expr, spin)                                                                        \
  for (s64 M_reps = 0; ({                                                                          \
         if (expr) break;                                                                          \
         if (M_reps++ < spin) continue;                                                            \
         true;                                                                                     \
       });                                                                                         \
       ({ break; }))

bool Mutex__try_lock(_Atomic(u8) *mtx);
void Mutex__unlock(_Atomic(u8) *mtx);

// NOTE: This breaks compatibility with GCC. I guess that's fine, but like, seems
// weird and a bit uncomfy.
//                                    - Albert Liu, Nov 15, 2021 Mon 18:36 EST
#define CRITICAL(mtx)                                                                              \
  for (u8 *M_mtx = (mtx); Mutex__try_lock(M_mtx); ({                                               \
         Mutex__unlock(M_mtx);                                                                     \
         break;                                                                                    \
       }))

#endif

#ifdef __DUMBOSS_IMPL__
#undef __DUMBOSS_IMPL__
#include <macros.h>
#define __DUMBOSS_IMPL__

bool Mutex__try_lock(_Atomic(u8) *mtx) {
  u8 current = 0;
  return a_cxweak(mtx, &current, 1);
}

void Mutex__unlock(_Atomic(u8) *mtx) {
  a_store(mtx, 0);
}

_Static_assert(sizeof(_Atomic(s64)) == 8, "atomics are zero-cost right?? :)");

#endif
