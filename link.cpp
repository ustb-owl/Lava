#define ATTRIBUTE_UNUSED __attribute__((__unused__))

// make sure rule linked to executable
extern int HelloXY;
extern int BlockMerge;
extern int DirtyArrayConvert;
extern int DeadGlobalCodeElimination;


int HelloLinked          = HelloXY;
int BlockMergeLinked     = BlockMerge;
int DirtyArrayLinked     = DirtyArrayConvert;
int DeadGlobalCodeLinked = DeadGlobalCodeElimination;

