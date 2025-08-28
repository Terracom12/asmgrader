# !!DISCLAIMER!!
# This was adapted from the following source
#   cpp-best-practices/cmake_template
#   https://github.com/cpp-best-practices/cmake_template/tree/1015c6b88410df411c0cc0413e3e64c33d7a8331
#   Courtesy of Jason Turner

function(
    asmgrader_enable_sanitizers
    target_name
    ENABLE_SANITIZER_ADDRESS
    ENABLE_SANITIZER_LEAK
    ENABLE_SANITIZER_UNDEFINED_BEHAVIOR
    ENABLE_SANITIZER_THREAD
    ENABLE_SANITIZER_MEMORY
  )

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
        set(SANITIZERS "")

        if(${ENABLE_SANITIZER_ADDRESS})
            list(APPEND SANITIZERS "address")
            # disable PIE, as ASLR messes with ASan pre-llvm17
            target_compile_options(${target_name} INTERFACE -no-pie)
            target_link_options(${target_name} INTERFACE -no-pie)
        endif()

        if(${ENABLE_SANITIZER_LEAK})
            list(APPEND SANITIZERS "leak")
        endif()

        if(${ENABLE_SANITIZER_UNDEFINED_BEHAVIOR})
            list(APPEND SANITIZERS "undefined")
        endif()

        if(${ENABLE_SANITIZER_THREAD})
            if("address" IN_LIST SANITIZERS OR "leak" IN_LIST SANITIZERS)
                message(WARNING "Thread sanitizer does not work with Address and Leak sanitizer enabled")
            else()
                list(APPEND SANITIZERS "thread")
            endif()
        endif()

        if(${ENABLE_SANITIZER_MEMORY} AND CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
            message(
                WARNING
                "Memory sanitizer requires all the code (including libc++) to be MSan-instrumented otherwise it reports false positives"
            )
            if("address" IN_LIST SANITIZERS
                OR "thread" IN_LIST SANITIZERS
                OR "leak" IN_LIST SANITIZERS
            )
                message(WARNING "Memory sanitizer does not work with Address, Thread or Leak sanitizer enabled")
            else()
                list(APPEND SANITIZERS "memory")
            endif()
        endif()
    endif()

    list(JOIN SANITIZERS "," LIST_OF_SANITIZERS)

    if(LIST_OF_SANITIZERS)
        if(NOT "${LIST_OF_SANITIZERS}" STREQUAL "")
            target_compile_options(${target_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
            target_link_options(${target_name} INTERFACE -fsanitize=${LIST_OF_SANITIZERS})
        endif()
    endif()
endfunction()
