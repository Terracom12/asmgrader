.intel_syntax noprefix

.global _start

_start:
    mov     rax, 1     # SYS_write
    mov     rdi, 1     # stdout
    lea     rsi, [rip + strHello]   # rsi = str
    mov     rdx, 22    # rdx (len) = strlen return
    syscall            # SYS_write

    mov     rax, 60  # SYS_exit
    mov     rdi, 42  # retcode
    syscall


/// sum
///   sums two numbers and returns the result
///   Parameters:
///     rdi (u64) - the first number
///     rsi (u64) - the second number
///   Result:
///     rax (u64) - rdi + rsi
sum:
    add    rsi, rdi    # rsi += rdi
    mov    rax, rsi    # rax = rsi
    ret

/// sum_and_write
///   sums two numbers and writes the result to stdout. Overflow may occur.
///   Parameters:
///     rdi (u64) - the first number
///     rsi (u64) - the second number
///   Result:
///     rdi + rsi written to stdout as 8 bytes.
sum_and_write:
    add    rsi, rdi    # rsi += rdi
    push   rsi

    mov     rax, 1     # SYS_write
    mov     rdi, 1     # stdout
    lea     rsi, [rsp] # rsi = stack addr - 8 (sum result)
    mov     rdx, 8     # rdx (len) = 8
    syscall            # SYS_write

    pop     rsi        # pop rsi off the stack
    ret

/// This subroutine will timeout in an infinitely recurring loop
timeout_fn:
    jmp timeout_fn

/// This subroutine will simply call SYS_exit with retcode=42
///   Parameters:
///     rdi (int) - the exit status
exiting_fn:
    mov     rax, 60  # SYS_exit
    # status (rdi) is passed as param
    syscall

/// This subroutine will segfault by continuing execution into the next section
segfaulting_fn:

    .data
strHello:    .asciz "Hello, from assembly!\n"
strGoodbye:  .asciz "Goodbye, :(\n"
