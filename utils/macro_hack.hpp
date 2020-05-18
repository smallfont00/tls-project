#pragma once

#define ASSIGN_EC(fd, func, op, final_stmt)                                 \
    fd = (func);                                                            \
    if ((fd)op) {                                                           \
        fprintf(stderr, "%s :: Fail [solution: %s]\n", #func, #final_stmt); \
        final_stmt;                                                         \
    };

#define DEFINE_EC(fd, func, op, final_stmt) __typeof__(func) ASSIGN_EC(fd, func, op, final_stmt);

#define ERR_CHECK(func, op, final_stmt)                                         \
    {                                                                           \
        int temp;                                                               \
        if ((temp = (func)op)) {                                                \
            fprintf(stderr, "%s :: Fail [solution: %s]\n", #func, #final_stmt); \
            final_stmt;                                                         \
        };                                                                      \
    }
