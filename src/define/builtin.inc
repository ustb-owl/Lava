
#ifndef BT_FUNCTIONS
#define BT_FUNCTIONS(Type, Name, Args)
#endif

BT_FUNCTIONS(Type::Int32, "getint",                            "")
BT_FUNCTIONS(Type::Int32, "getch",                             "")
BT_FUNCTIONS(Type::Int32, "getarray",                   "int a[]")
BT_FUNCTIONS(Type::Void,  "putint",                       "int a")
BT_FUNCTIONS(Type::Void,  "putch",                         "inta")
BT_FUNCTIONS(Type::Void,  "putarray",            "int n, int a[]")
BT_FUNCTIONS(Type::Void,  "starttime",                         "")
BT_FUNCTIONS(Type::Void,  "stoptime",                          "")
BT_FUNCTIONS(Type::Void,  "memcpy",    "int *a, int *b, int size")
BT_FUNCTIONS(Type::Void,  "memset",    "int *a, int  b, int size")

#undef BT_FUNCTIONS