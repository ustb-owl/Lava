add_library(definelib
        ast.cpp
        type.cpp
        )

target_compile_features(definelib PUBLIC cxx_std_23)
add_library(lava::definelib ALIAS definelib)
