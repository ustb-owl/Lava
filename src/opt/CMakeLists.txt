add_library(opt
        pass.cpp
        pass_manager.cpp
        blkwalker.cpp
        analysis/loopinfo.cpp
        analysis/dominance.cpp
        analysis/funcanalysis.cpp
        analysis/dominancebase.cpp
        analysis/postdominance.cpp
        transforms/dge.cpp
        transforms/dce.cpp
        transforms/hello.cpp
        transforms/mem2reg.cpp
        transforms/gvn_gcm.cpp
        transforms/blocksimp.cpp
        transforms/tailrecursion.cpp
        transforms/dirtyname_conv.cpp
        transforms/dirtyarray_conv.cpp
)

target_compile_features(opt PUBLIC cxx_std_17)
add_library(lava::opt ALIAS opt)