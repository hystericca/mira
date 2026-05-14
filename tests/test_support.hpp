#pragma once

#include <cstdio>
#include <cstdlib>

#define MIRA_TEST(condition)                                                                       \
    do {                                                                                           \
        if (!(condition)) {                                                                        \
            std::fprintf(stderr, "%s:%d: test failed: %s\n", __FILE__, __LINE__, #condition);      \
            std::abort();                                                                          \
        }                                                                                          \
    } while (false)
