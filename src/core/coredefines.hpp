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

#define TRY_FINALLY(CODE, CLEANUP) \
  try \
  { \
    CODE \
  } \
  __finally \
  { \
    CLEANUP \
  }

#define TRY_FINALLY1(VAR1, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)
#define TRY_FINALLY2(VAR1, VAR2, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)
#define TRY_FINALLY3(VAR1, VAR2, VAR3, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)
#define TRY_FINALLY4(VAR1, VAR2, VAR3, VAR4, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)
#define TRY_FINALLY5(VAR1, VAR2, VAR3, VAR4, VAR5, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)
#define TRY_FINALLY6(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)
#define TRY_FINALLY7(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, VAR7, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)
#define TRY_FINALLY8(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, VAR7, VAR8, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)
#define TRY_FINALLY9(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, VAR7, VAR8, VAR9, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)
#define TRY_FINALLY10(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, VAR7, VAR8, VAR9, VAR10, CODE, CLEANUP) \
  TRY_FINALLY(CODE, CLEANUP)

#define Self this

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

#define TRY_FINALLY(CODE, CLEANUP) \
    { \
      CLEANUP \
    } BOOST_SCOPE_EXIT_END \
    { \
      CODE \
    } \

#define TRY_FINALLY1(VAR1, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }
#define TRY_FINALLY2(VAR1, VAR2, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) (&VAR2) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }
#define TRY_FINALLY3(VAR1, VAR2, VAR3, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) (&VAR2) (&VAR3) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }
#define TRY_FINALLY4(VAR1, VAR2, VAR3, VAR4, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) (&VAR2) (&VAR3) (&VAR4) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }
#define TRY_FINALLY5(VAR1, VAR2, VAR3, VAR4, VAR5, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) (&VAR2) (&VAR3) (&VAR4) (&VAR5) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }
#define TRY_FINALLY6(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) (&VAR2) (&VAR3) (&VAR4) (&VAR5) (&VAR6) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }
#define TRY_FINALLY7(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, VAR7, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) (&VAR2) (&VAR3) (&VAR4) (&VAR5) (&VAR6) (&VAR7) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }
#define TRY_FINALLY8(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, VAR7, VAR8, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) (&VAR2) (&VAR3) (&VAR4) (&VAR5) (&VAR6) (&VAR7) (&VAR8) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }
#define TRY_FINALLY9(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, VAR7, VAR8, VAR9, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) (&VAR2) (&VAR3) (&VAR4) (&VAR5) (&VAR6) (&VAR7) (&VAR8) (&VAR9) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }
#define TRY_FINALLY10(VAR1, VAR2, VAR3, VAR4, VAR5, VAR6, VAR7, VAR8, VAR9, VAR10, CODE, CLEANUP) \
  { \
    BOOST_SCOPE_EXIT ( (&VAR1) (&VAR2) (&VAR3) (&VAR4) (&VAR5) (&VAR6) (&VAR7) (&VAR8) (&VAR9) (&VAR10) ) \
    TRY_FINALLY(CODE, CLEANUP) \
  }

#define TShellExecuteInfoW _SHELLEXECUTEINFOW

#endif /* _MSC_VER */

