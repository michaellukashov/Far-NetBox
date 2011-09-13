
#ifndef __DELEGATE_H
#define __DELEGATE_H


#include <list>


// 0 parameters...
#define SUFFIX             0
#define TEMPLATE_PARAMS
#define TEMPLATE_ARGS
#define PARAMS
#define ARGS

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS
    
// 1 parameter...
#define SUFFIX             1
#define TEMPLATE_PARAMS    , class TP1
#define TEMPLATE_ARGS      , TP1
#define PARAMS             TP1 p1
#define ARGS               p1

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS

// 2 parameters...
#define SUFFIX             2
#define TEMPLATE_PARAMS    , class TP1, class TP2
#define TEMPLATE_ARGS      , TP1, TP2
#define PARAMS             TP1 p1, TP2 p2
#define ARGS               p1, p2

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS

// 3 parameters...
#define SUFFIX             3
#define TEMPLATE_PARAMS    , class TP1, class TP2, class TP3
#define TEMPLATE_ARGS      , TP1, TP2, TP3
#define PARAMS             TP1 p1, TP2 p2, TP3 p3
#define ARGS               p1, p2, p3

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS

// 4 parameters...
#define SUFFIX             4
#define TEMPLATE_PARAMS    , class TP1, class TP2, class TP3, class TP4
#define TEMPLATE_ARGS      , TP1, TP2, TP3, TP4
#define PARAMS             TP1 p1, TP2 p2, TP3 p3, TP4 p4
#define ARGS               p1, p2, p3, p4

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS

// 5 parameters...
#define SUFFIX             5
#define TEMPLATE_PARAMS    , class TP1, class TP2, class TP3, class TP4, class TP5
#define TEMPLATE_ARGS      , TP1, TP2, TP3, TP4, TP5
#define PARAMS             TP1 p1, TP2 p2, TP3 p3, TP4 p4, TP5 p5
#define ARGS               p1, p2, p3, p4, p5

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS

// 6 parameters...
#define SUFFIX             6
#define TEMPLATE_PARAMS    , class TP1, class TP2, class TP3, class TP4, class TP5, class TP6
#define TEMPLATE_ARGS      , TP1, TP2, TP3, TP4, TP5, TP6
#define PARAMS             TP1 p1, TP2 p2, TP3 p3, TP4 p4, TP5 p5, TP6 p6
#define ARGS               p1, p2, p3, p4, p5, p6

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS

// 7 parameters...
#define SUFFIX             7
#define TEMPLATE_PARAMS    , class TP1, class TP2, class TP3, class TP4, class TP5, class TP6, class TP7
#define TEMPLATE_ARGS      , TP1, TP2, TP3, TP4, TP5, TP6, TP7
#define PARAMS             TP1 p1, TP2 p2, TP3 p3, TP4 p4, TP5 p5, TP6 p6, TP7 p7
#define ARGS               p1, p2, p3, p4, p5, p6, p7

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS

// 8 parameters...
#define SUFFIX             8
#define TEMPLATE_PARAMS    , class TP1, class TP2, class TP3, class TP4, class TP5, class TP6, class TP7, class TP8
#define TEMPLATE_ARGS      , TP1, TP2, TP3, TP4, TP5, TP6, TP7, TP8
#define PARAMS             TP1 p1, TP2 p2, TP3 p3, TP4 p4, TP5 p5, TP6 p6, TP7 p7, TP8 p8
#define ARGS               p1, p2, p3, p4, p5, p6, p7, p8

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS

// 9 parameters...
#define SUFFIX             9
#define TEMPLATE_PARAMS    , class TP1, class TP2, class TP3, class TP4, class TP5, class TP6, class TP7, class TP8, class TP9
#define TEMPLATE_ARGS      , TP1, TP2, TP3, TP4, TP5, TP6, TP7, TP8, TP9
#define PARAMS             TP1 p1, TP2 p2, TP3 p3, TP4 p4, TP5 p5, TP6 p6, TP7 p7, TP8 p8, TP9 p9
#define ARGS               p1, p2, p3, p4, p5, p6, p7, p8, p9

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS

// 10 parameters...
#define SUFFIX             10
#define TEMPLATE_PARAMS    , class TP1, class TP2, class TP3, class TP4, class TP5, class TP6, class TP7, class TP8, class TP9, class TP10
#define TEMPLATE_ARGS      , TP1, TP2, TP3, TP4, TP5, TP6, TP7, TP8, TP9, TP10
#define PARAMS             TP1 p1, TP2 p2, TP3 p3, TP4 p4, TP5 p5, TP6 p6, TP7 p7, TP8 p8, TP9 p9, TP10 p10
#define ARGS               p1, p2, p3, p4, p5, p6, p7, p8, p9, p10

#include "delegate_impl.h"

#undef SUFFIX
#undef TEMPLATE_PARAMS
#undef TEMPLATE_ARGS
#undef PARAMS
#undef ARGS


#endif /* __DELEGATE_H */