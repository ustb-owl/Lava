#define ATTRIBUTE_UNUSED __attribute__((__unused__))

// make sure rule linked to executable
extern int HelloXY;
extern int BlockMerge;
extern int DirtyArrayConvert;
extern int DirtyFunctionConvert;
extern int DeadGlobalCodeElimination;
extern int Mem2Reg;
extern int TailRecursion;
extern int GlobalValueNumbering;
extern int DeadCodeElimination;
extern int LoopUnrolling;
extern int GlobalConstPropagation;

// analysis
extern int Dominance;
extern int PostDominance;
extern int FunctionInfo;
extern int LoopInfo;
extern int NeedGcm;

int HelloLinked          = HelloXY;
int BlockMergeLinked     = BlockMerge;
int DirtyArrayLinked     = DirtyArrayConvert;
int DirtyFunctionLinked  = DirtyFunctionConvert;
int DeadGlobalCodeLinked = DeadGlobalCodeElimination;
int Mem2RegLinked        = Mem2Reg;
int TailRecursionLinked  = TailRecursion;
int GVNGCMLinked         = GlobalValueNumbering;
int DeadCodeLinked       = DeadCodeElimination;
int LoopUnrollingLinked  = LoopUnrolling;
int GlbConstPropLinked   = GlobalConstPropagation;

int DominanceLinked      = Dominance;
int PostDominanceLinked  = PostDominance;
int FunctionInfoLinked   = FunctionInfo;
int LoopINfoLinked       = LoopInfo;
int NeedGcmLinked        = NeedGcm;

