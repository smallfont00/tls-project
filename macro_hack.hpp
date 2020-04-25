#pragma once

#define ASSIGN_EC(fd, func, op, final_stmt) \
    fd = func;                              \
    if (fd op) {                            \
        perror(#func " :");                 \
        final_stmt;                         \
    };

#define DEFINE_EC(fd, func, op, final_stmt) \
    int ASSIGN_EC(fd, func, op, final_stmt);

#define TEMP_EC(func, op, final_stmt) \
    { DEFINE_EC(tmp, func, op, final_stmt) }

#define max(a, b) ((a) > (b) ? (a) : (b))