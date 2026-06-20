add_library(test_common STATIC common.c)

file(GLOB TEST_SRCS "*.c")

foreach(TEST_SRC ${TEST_SRCS})
    get_filename_component(SRC_NAME ${TEST_SRC} NAME)
    if(SRC_NAME STREQUAL "common.c")
        continue()
    endif()

    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)

    set(PREPROCESSED_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.i")
    set(ASM_FILE "${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}.s")
    set(EXE_TARGET "test_${TEST_NAME}_exe")

    add_custom_command(
        OUTPUT ${PREPROCESSED_FILE}
        COMMAND ${CMAKE_C_COMPILER} -E -P -C ${TEST_SRC} -o ${PREPROCESSED_FILE}
        DEPENDS ${TEST_SRC}
        COMMENT "[TEST] Preprocessing ${SRC_NAME} with system compiler..."
    )

    add_custom_command(
        OUTPUT ${ASM_FILE}
        COMMAND minicc -o ${ASM_FILE} ${PREPROCESSED_FILE}
        DEPENDS minicc ${PREPROCESSED_FILE}
        COMMENT "[TEST] Compiling ${TEST_NAME}.i to assembly with minicc..."
    )

    add_executable(${EXE_TARGET})
    target_sources(${EXE_TARGET} PRIVATE ${ASM_FILE})
    set_source_files_properties(${ASM_FILE} PROPERTIES GENERATED TRUE)
    set_source_files_properties(${ASM_FILE} PROPERTIES LANGUAGE ASM)
    set_target_properties(${EXE_TARGET} PROPERTIES LINKER_LANGUAGE C)
    target_link_libraries(${EXE_TARGET} PRIVATE test_common)

    add_test(NAME "test_${TEST_NAME}" COMMAND ${EXE_TARGET})

endforeach()