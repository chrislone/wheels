#include <stdio.h>
#include "zlib.h"

#ifndef ZPIPE_HPP
#define ZPIPE_HPP

struct Zpipe
{
public:
    static const int CHUNK = 1024;
    int def(FILE *src, FILE *dest, int level);
    int inf(FILE *src, FILE *dest);
    int ret;
    static void zerr(int ret);
};

#endif
