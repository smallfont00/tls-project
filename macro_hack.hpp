#pragma once

#define ASSIGN_EC(fd, func, op, final_stmt)                                 \
    fd = (func);                                                            \
    if ((fd)op) {                                                           \
        fprintf(stderr, "%s :: Fail [solution: %s]\n", #func, #final_stmt); \
        final_stmt;                                                         \
    };

#define DEFINE_EC(fd, func, op, final_stmt) \
    typeof(func) ASSIGN_EC(fd, func, op, final_stmt);

#define ERR_CHECK(func, op, final_stmt)                                     \
    if ((func)op) {                                                         \
        fprintf(stderr, "%s :: Fail [solution: %s]\n", #func, #final_stmt); \
        final_stmt;                                                         \
    };

#define max(a, b) ((a) > (b) ? (a) : (b))