add_library(opt
        pass.cpp
        pass_manager.cpp
        register.cpp
        blkwalker.cpp
        analysis/identiy.cpp
        analysis/loopinfo.cpp
        analysis/dominance.cpp
        analysis/funcanalysis.cpp
        analysis/dominancebase.cpp
        analysis/postdominance.cpp
        transforms/dge.cpp
        transforms/dce.cpp
        transforms/hello.cpp
        transforms/inline.cpp
        transforms/mem2reg.cpp
        transforms/strength_reduction.cpp
        transforms/gvn_gcm.cpp
        transforms/loop_invariant_hoist.cpp
        transforms/blocksimp.cpp
        transforms/localmemprop.cpp
        transforms/tailrecursion.cpp
        transforms/simpunrolling.cpp
        transforms/sanitize_ir.cpp
        transforms/dirtyname_conv.cpp
        transforms/globalconstprop.cpp
        transforms/dirtyarray_conv.cpp
)

target_compile_features(opt PUBLIC cxx_std_23)
add_library(lava::opt ALIAS opt)
