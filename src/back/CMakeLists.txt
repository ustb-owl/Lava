add_library(back
        slot.cpp
        codegen.cpp
        arch/arm/module.cpp
        arch/arm/instdef.cpp
        passes/dme.cpp
        passes/spill.cpp
        passes/funcfix.cpp
        passes/peephole.cpp
        passes/liveness.cpp
        passes/linearscan.cpp
        passes/fastalloc.cpp
        passes/blkrearrange.cpp
        passes/tailrecursion.cpp
)

target_compile_features(back PUBLIC cxx_std_17)
add_library(lava::back ALIAS back)
