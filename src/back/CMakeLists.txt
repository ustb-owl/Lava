add_library(back
        codegen.cpp
        arch/arm/instdef.cpp
        arch/arm/module.cpp
)

target_compile_features(back PUBLIC cxx_std_17)
add_library(lava::back ALIAS back)
