#define ATTRIBUTE_UNUSED __attribute__((__unused__))

// make sure rule linked to executable
extern int HelloXY;
extern int BlockMerge;
extern int DirtyArrayConvert;
extern int DirtyFunctionConvert;
extern int DeadGlobalCodeElimination;
extern int DeadCodeElimination;

// analysis
extern int Dominance;
extern int PostDominance;

int HelloLinked          = HelloXY;
int BlockMergeLinked     = BlockMerge;
int DirtyArrayLinked     = DirtyArrayConvert;
int DirtyFunctionLinked  = DirtyFunctionConvert;
int DeadGlobalCodeLinked = DeadGlobalCodeElimination;
int DeadCodeLinked       = DeadCodeElimination;

int DominanceLinked      = Dominance;
int PostDominanceLinked  = PostDominance;

int LINK() {
  int res = 0;
  res += HelloLinked;
  res += BlockMergeLinked;
  res += DirtyArrayLinked;
  res += DeadGlobalCodeLinked;
  res += DeadCodeLinked;

  res += DominanceLinked;
  return res;
}
