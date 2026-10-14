#pragma once
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
#include <xmmintrin.h>
#endif
namespace vm {
// Preserve the host's floating-point environment. FTZ/DAZ is local to callback.
class RealtimeGuard {
#if defined(__SSE__) || defined(_M_X64) || defined(_M_IX86)
 unsigned saved_;
public:
 RealtimeGuard() noexcept:saved_(_mm_getcsr()){_mm_setcsr(saved_|0x8040u);}
 ~RealtimeGuard() noexcept{_mm_setcsr(saved_);}
#else
public:
 RealtimeGuard() noexcept=default;
#endif
};
}
