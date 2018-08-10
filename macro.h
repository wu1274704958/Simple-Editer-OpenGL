#define BuildStr(NS,name,str) \
namespace  _##NS { const char * _##name = #str ; }