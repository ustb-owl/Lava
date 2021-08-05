#define ATTRIBUTE_UNUSED __attribute__((__unused__))

// make sure rule linked to executable
extern int HelloXY;
extern int BlockMerge;
extern int DirtyArrayConvert;
extern int DirtyFunctionConvert;
extern int DeadGlobalCodeElimination;
extern int Mem2Reg;
extern int GlobalValueNumbering;
extern int DeadCodeElimination;

// analysis
extern int Dominance;
extern int PostDominance;

int HelloLinked          = HelloXY;
int BlockMergeLinked     = BlockMerge;
int DirtyArrayLinked     = DirtyArrayConvert;
int DirtyFunctionLinked  = DirtyFunctionConvert;
int DeadGlobalCodeLinked = DeadGlobalCodeElimination;
int Mem2RegLinked        = Mem2Reg;
int GVNGCMLinked         = GlobalValueNumbering;
int DeadCodeLinked       = DeadCodeElimination;

int DominanceLinked      = Dominance;
int PostDominanceLinked  = PostDominance;

