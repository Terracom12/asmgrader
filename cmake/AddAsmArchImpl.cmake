# Usage:
#   add_asm_arch_specific_*(target_name <target_stem>)
# For example:
#   add_asm_arch_specific_*(lab4-1 driver4-1)


# Helper functions
function(_get_asm_src_name target_stem)
    # Normalize system arch
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_ARCH)

    if(SYSTEM_ARCH MATCHES "^(x86_64|amd64)$")
        set(ARCH_SUFFIX "x86_64")
    elseif(SYSTEM_ARCH MATCHES "^(aarch64|arm64)$")
        set(ARCH_SUFFIX "aarch64")
    else()
        message(FATAL_ERROR "Unsupported architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()

    set(ASM_SRC "${CMAKE_CURRENT_SOURCE_DIR}/${target_stem}_${ARCH_SUFFIX}.s")
    if(NOT EXISTS "${ASM_SRC}")
        message(FATAL_ERROR "Missing architecture-specific source file: ${ASM_SRC}")
    else()
        set_source_files_properties(${ASM_SRC} PROPERTIES LANGUAGE ASM)
        set(ASM_SRC ${ASM_SRC} PARENT_SCOPE)
    endif()
endfunction()

function(add_asm_arch_specific_lib target_name target_stem)
    _get_asm_src_name(${target_stem})

    add_library(${target_name} OBJECT ${ASM_SRC})

    # Our assembly programs are bare-bones, so we don't want all the libraries
    target_compile_options(
        ${target_name} 
        PRIVATE 
        -g -static -nolibc -nostartfiles -nostdlib)
    target_link_options(
        ${target_name} 
        PRIVATE 
        -g -static -nolibc -nostartfiles -nostdlib)

endfunction()

function(add_asm_arch_specific_impl target_name target_stem)
    _get_asm_src_name(${target_stem})

    add_executable(${target_name} ${ASM_SRC})

    # Our assembly programs are bare-bones, so we don't want all the libraries
    target_compile_options(
        ${target_name} 
        PRIVATE 
        -g -static -nolibc -nostartfiles -nostdlib)
    target_link_options(
        ${target_name} 
        PRIVATE 
        -g -static -nolibc -nostartfiles -nostdlib)

endfunction()
