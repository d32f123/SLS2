#include "multibyte.h"

int readUTFchar(char * src, wint_t * dest)
{
    return mbrtowc(dest, src, 4, NULL);
}

/* returns how many more chars are needed */
int is_corrupt_UTF(char * src, int available_size)
{
    union UTFchar_t temp;
    int byteCount = 1;
    temp.fullChar = 0;
    temp.partChar[0] = *src;
    if (temp.partChar[0] >= 192)
    {
        ++byteCount;
        if (temp.partChar[0] >= 224)
        {
            ++byteCount;
            if (temp.partChar[0] >= 240)
                ++byteCount;
        }
    }
    if (byteCount > available_size)
    {
        return byteCount - available_size;
    }
    return 0;
}