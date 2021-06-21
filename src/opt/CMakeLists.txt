add_library(opt
        pass.cpp
        pass_manager.cpp
        blkwalker.cpp
        analysis/dominance.cpp
        transforms/dge.cpp
        transforms/dce.cpp
        transforms/hello.cpp
        transforms/blockmerge.cpp
        transforms/dirtyarray_conv.cpp
)

target_compile_features(opt PUBLIC cxx_std_17)
add_library(lava::opt ALIAS opt)