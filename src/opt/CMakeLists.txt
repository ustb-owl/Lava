add_library(opt
        pass.cpp
        pass_manager.cpp
        transforms/hello.cpp
        transforms/blockmerge.cpp
)

target_compile_features(opt PUBLIC cxx_std_17)
add_library(lava::opt ALIAS opt)