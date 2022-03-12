#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// 字符串转成小写
char *utils_str_to_lower_case(char *str) {
    char *rt_str = (char *)malloc(sizeof(char) * (strlen(str) + 1));
    char *local_ptr = str;
    char *ptr = rt_str;
    memset(rt_str, 0, sizeof(*rt_str));
    while (*local_ptr != '\0')
    {
        *ptr = tolower(*local_ptr);
        ptr++;
        local_ptr++;
    }
    return rt_str;
}
