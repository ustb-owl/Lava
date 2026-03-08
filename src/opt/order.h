#define DIRTY_ARRAY_CONV     1
#define DIRTY_FUNCTION_CONV  2
#define BLOCK_SIMPLIFICATION 3

#define LOOP_UNROLL                  4
#define MEMORY_TO_REGISTER           5
#define GLOBAL_CONST_PROP            6
#define TAIL_RECURSION               7
#define INST_SIMPLIFY                8
#define STRENGTH_REDUCTION           9
#define LOCAL_VALUE_NUMBERING        10
#define LOOP_INVARIANT_HOIST         11
#define LOCAL_MEM_PROP               12
#define DEAD_CODE_ELIMINATION        13
#define FUNCTION_INLINING            14
#define FUNCTION_CLEANUP             15
#define DEAD_GLOBAL_CODE_ELIMINATION 16
#define LOOP_UNROLLING               17
#define SANITIZE_IR                  18

// analysis
#define DOMINANCE_INFO      50
#define POST_DOMINANCE_INFO 51
#define NEED_GCM            52
#define FUNCTION_INFO       53
#define LOOP_INFO           54
