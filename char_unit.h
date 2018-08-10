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

int read_file(CharUnit *cu,FILE *f)
{
    int v = 0;
    v = fread(&(cu->c),sizeof(wchar_t),1,f);
    if(v != 1) return -1;
    v = fread(&(cu->x),sizeof(int),1,f);
    if(v != 1) return -1;
    v = fread(&(cu->y),sizeof(int),1,f);
    if(v != 1) return -1;
    v = fread(&(cu->w),sizeof(int),1,f);
    if(v != 1) return -1;
    v = fread(&(cu->h),sizeof(int),1,f);
    if(v != 1) return -1;
    return 0;
}

}

#endif //_WWS_CHAR_UNIT_H__