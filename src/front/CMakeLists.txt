add_library(front
        lexer.cpp
        parser.cpp
        logger.cpp
        )

target_compile_features(front PUBLIC cxx_std_23)
add_library(lava::front ALIAS front)
