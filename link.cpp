#define ATTRIBUTE_UNUSED __attribute__((__unused__))

// make sure rule linked to executable
extern int HelloXY;
extern int BlockMerge;
extern int DirtyArrayConvert;
extern int DeadGlobalCodeElimination;
extern int DeadCodeElimination;

// analysis
extern int Dominance;

int HelloLinked          = HelloXY;
int BlockMergeLinked     = BlockMerge;
int DirtyArrayLinked     = DirtyArrayConvert;
int DeadGlobalCodeLinked = DeadGlobalCodeElimination;
int DeadCodeLinked       = DeadCodeElimination;

int DominanceLinked      = Dominance;

