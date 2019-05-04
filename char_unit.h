#ifndef _WWS_CHAR_UNIT_H__
#define _WWS_CHAR_UNIT_H__

#include <stdio.h>
namespace char_unit{

struct CharUnit{
    wchar_t c;
    int x,y,w,h;
};

void write_file(CharUnit *cu,FILE *f)
{
    fwrite((void *)&(cu->c),sizeof(wchar_t),1,f);
    fwrite((void *)&(cu->x),sizeof(int),1,f);
    fwrite((void *)&(cu->y),sizeof(int),1,f);
    fwrite((void *)&(cu->w),sizeof(int),1,f);
    fwrite((void *)&(cu->h),sizeof(int),1,f);
}

    template<typename T>
    size_t read(FILE *f,T *t)
    {
        assert(t != nullptr);

        constexpr size_t Size = std::is_same_v<T,wchar_t> ? 2 : sizeof(T);

        *t = static_cast<T>(0);

        char src_data[Size] = {0};
        size_t read_size = fread(src_data,Size,1,f);
        if (read_size == 0)
            return  0;
//#ifdef WIN32

        memcpy(t,src_data,Size);
        return read_size;

//#else
//        char *dest_ptr = reinterpret_cast<char *>(t);
//        int i2 = 0;
//        for(int i = static_cast<int>(Size - 1);i >=0 ;--i)
//        {
//            memcpy(dest_ptr + i2,src_data + i,1);
//            i2++;
//        }
//        return  read_size;
//#endif

    }

int read_file(CharUnit *cu,FILE *f)
{
    size_t v = 0;
    v = read<wchar_t>(f,&(cu->c));
    if(v != 1) return -1;
    v = read<int>(f,&(cu->x));
    if(v != 1) return -1;
    v = read<int>(f,&(cu->y));
    if(v != 1) return -1;
    v = read<int>(f,&(cu->w));
    if(v != 1) return -1;
    v = read<int>(f,&(cu->h));
    if(v != 1) return -1;
    return 0;
}

}

#endif //_WWS_CHAR_UNIT_H__