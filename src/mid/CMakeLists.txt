add_library(mid
        ir/ssa.cpp
        ir/module.cpp
        ../common/idmanager.cpp
        ir/usedef/use.cpp
        ir/usedef/value.cpp
        walker/analyzer/sema.cpp
        walker/analyzer/eval.cpp
        walker/analyzer/analyzer.cpp
        walker/irbuilder/irbuilder.cpp
        )

target_compile_features(mid PUBLIC cxx_std_17)
add_library(lava::mid ALIAS mid)