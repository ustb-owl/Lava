add_library(front
        lexer.cpp
        parser.cpp
        logger.cpp
        "${LAVA_SOURCE_DIR}/define/AST.cpp"
        "${LAVA_SOURCE_DIR}/define/type.cpp"
        "${LAVA_SOURCE_DIR}/define/builtin.cpp"
        )

target_compile_features(front PUBLIC cxx_std_17)
add_library(lava::front ALIAS front)