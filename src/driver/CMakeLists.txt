add_library(driver
        compiler.cpp
        )

target_compile_features(driver PUBLIC cxx_std_23)
add_library(lava::driver ALIAS driver)
