#pragma once

#define FORWARD0(expr) expr
#define FORWARD1(a1, expr)                                                                                             \
    namespace a1 {                                                                                                     \
    expr;                                                                                                              \
    }
#define FORWARD2(a1, a2, expr)                                                                                         \
    namespace a1 { namespace a2 {                                                                                      \
    expr;                                                                                                              \
    }}
#define FORWARD3(a1, a2, a3, expr)                                                                                     \
    namespace a1 { namespace a2 { namespace a3 {                                                                       \
    expr;                                                                                                              \
    }}}
