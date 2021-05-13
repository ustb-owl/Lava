#define ATTRIBUTE_UNUSED __attribute__((__unused__))

// make sure rule linked to executable
extern int HelloXY;
extern int BlockMerge;

int HelloLinked          = HelloXY;
int BlockMergeLinked     = BlockMerge;

