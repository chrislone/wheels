#include "zpipe.h"
#include <stdio.h>
#include <iostream>

using std::cout;
using std::endl;
using std::string;

int main(int argc, char **argv)
{
    Zpipe zpipe;

    if (argc == 2)
    {
        const string argv1 = argv[1];
        const string argv_version = "-zlib_version";
        const string argv_d = "-d";

        // 打印 zlib 的版本
        if (argv1 == argv_version)
        {
            cout << "ZLIB_VERSION: " << ZLIB_VERSION << endl;
        }
        else if (argv1 == argv_d)
        {
            zpipe.ret = zpipe.inf(stdin, stdout);
            if (zpipe.ret != Z_OK)
                Zpipe::zerr(zpipe.ret);
            return zpipe.ret;
        }
    }
    else if (argc == 1)
    {
        zpipe.ret = zpipe.def(stdin, stdout, Z_DEFAULT_COMPRESSION);
        if (zpipe.ret != Z_OK)
            Zpipe::zerr(zpipe.ret);
        return zpipe.ret;
    }

    return 0;
}
