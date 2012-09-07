#pragma once

#ifndef _MSC_VER

#define DEFINE_CALLBACK_TYPE0(EVENT, R) \
  typedef R __fastcall (__closure *EVENT)()
#define DEFINE_CALLBACK_TYPE1(EVENT, R, T1) \
  typedef R __fastcall (__closure *EVENT)(T1)
#define DEFINE_CALLBACK_TYPE2(EVENT, R, T1, T2) \
  typedef R __fastcall (__closure *EVENT)(T1, T2)
#define DEFINE_CALLBACK_TYPE3(EVENT, R, T1, T2, T3) \
  typedef R __fastcall (__closure *EVENT)(T1, T2, T3)
#define DEFINE_CALLBACK_TYPE4(EVENT, R, T1, T2, T3, T4) \
  typedef R __fastcall (__closure *EVENT)(T1, T2, T3, T4)
#define DEFINE_CALLBACK_TYPE5(EVENT, R, T1, T2, T3, T4, T5) \
  typedef R __fastcall (__closure *EVENT)(T1, T2, T3, T4, T5)
#define DEFINE_CALLBACK_TYPE6(EVENT, R, T1, T2, T3, T4, T5, T6) \
  typedef R __fastcall (__closure *EVENT)(T1, T2, T3, T4, T5, T6)
#define DEFINE_CALLBACK_TYPE7(EVENT, R, T1, T2, T3, T4, T5, T6, T7) \
  typedef R __fastcall (__closure *EVENT)(T1, T2, T3, T4, T5, T6, T7)
#define DEFINE_CALLBACK_TYPE8(EVENT, R, T1, T2, T3, T4, T5, T6, T7, T8) \
  typedef R __fastcall (__closure *EVENT)(T1, T2, T3, T4, T5, T6, T7, T8)

#define MAKE_CALLBACK0(METHOD, OBJECT) \
  (OBJECT)->METHOD
#define MAKE_CALLBACK1(METHOD, OBJECT) \
  (OBJECT)->METHOD
#define MAKE_CALLBACK2(METHOD, OBJECT) \
  (OBJECT)->METHOD
#define MAKE_CALLBACK3(METHOD, OBJECT) \
  (OBJECT)->METHOD
#define MAKE_CALLBACK4(METHOD, OBJECT) \
  (OBJECT)->METHOD
#define MAKE_CALLBACK5(METHOD, OBJECT) \
  (OBJECT)->METHOD
#define MAKE_CALLBACK6(METHOD, OBJECT) \
  (OBJECT)->METHOD
#define MAKE_CALLBACK7(METHOD, OBJECT) \
  (OBJECT)->METHOD
#define MAKE_CALLBACK8(METHOD, OBJECT) \
  (OBJECT)->METHOD

#else

#define DEFINE_CALLBACK_TYPE0(EVENT,     R) \
  typedef fastdelegate::FastDelegate0<R> EVENT
#define DEFINE_CALLBACK_TYPE1(EVENT,     R, T1) \
  typedef fastdelegate::FastDelegate1<R, T1> EVENT
#define DEFINE_CALLBACK_TYPE2(EVENT,     R, T1, T2) \
  typedef fastdelegate::FastDelegate2<R, T1, T2> EVENT
#define DEFINE_CALLBACK_TYPE3(EVENT,     R, T1, T2, T3) \
  typedef fastdelegate::FastDelegate3<R, T1, T2, T3> EVENT
#define DEFINE_CALLBACK_TYPE4(EVENT,     R, T1, T2, T3, T4) \
  typedef fastdelegate::FastDelegate4<R, T1, T2, T3, T4> EVENT
#define DEFINE_CALLBACK_TYPE5(EVENT,     R, T1, T2, T3, T4, T5) \
  typedef fastdelegate::FastDelegate5<R, T1, T2, T3, T4, T5> EVENT
#define DEFINE_CALLBACK_TYPE6(EVENT,     R, T1, T2, T3, T4, T5, T6) \
  typedef fastdelegate::FastDelegate6<R, T1, T2, T3, T4, T5, T6> EVENT
#define DEFINE_CALLBACK_TYPE7(EVENT,     R, T1, T2, T3, T4, T5, T6, T7) \
  typedef fastdelegate::FastDelegate7<R, T1, T2, T3, T4, T5, T6, T7> EVENT
#define DEFINE_CALLBACK_TYPE8(EVENT,     R, T1, T2, T3, T4, T5, T6, T7, T8) \
  typedef fastdelegate::FastDelegate8<R, T1, T2, T3, T4, T5, T6, T7, T8> EVENT

#define MAKE_CALLBACK0(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT)
#define MAKE_CALLBACK1(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT, _1)
#define MAKE_CALLBACK2(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT, _1, _2)
#define MAKE_CALLBACK3(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT, _1, _2, _3)
#define MAKE_CALLBACK4(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT, _1, _2, _3, _4)
#define MAKE_CALLBACK5(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT, _1, _2, _3, _4, _5)
#define MAKE_CALLBACK6(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT, _1, _2, _3, _4, _5, _6)
#define MAKE_CALLBACK7(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT, _1, _2, _3, _4, _5, _6, _7)
#define MAKE_CALLBACK8(METHOD, OBJECT) \
  fastdelegate::bind(&METHOD, OBJECT, _1, _2, _3, _4, _5, _6, _7, _8)

#endif /* _MSC_VER */

