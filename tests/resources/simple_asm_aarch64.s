.global _start

    .text
_start:
    mov     x8, 64         // SYS_write
    mov     x0, 1          // fd param = stdout
    ldr     x1, =strHello  // str param = &strHello
    mov     x2, 22         // len param = 22
    svc     0              // SYS_write()

    mov     x8, 93   // SYS_exit
    mov     x0, 42   // retcode
    svc     0        // SYS_exit()

/// sum
///   sums two numbers and returns the result
///   Parameters:
///     x0 (u64) - the first number
///     x1 (u64) - the second number
///   Result:
///     x0 (u64) - x0 + x1
sum:
    add    x0, x0, x1    // x0 += x1
    ret

/// write_to
///   sums two numbers and writes the result to stdout. Overflow may occur.
///   Parameters:
///     x0 (const char*) - string to write
///     x1 (int) - the fd to write to
///     x2 (size_t) - length of the string
///   Result:
///     length bytes of string written to fd
write_to:
    mov     x3, x0   # save x0 (str) into x3

    mov     x8, 64         // SYS_write
    mov     x0, x1         // fd param = fd
    mov     x1, x3         // str param = string
    // len param [already set by param]
    svc     0              // SYS_write

    ret

/// sum_and_write_fd
///   sums two numbers and writes the result to specified fd. Overflow may occur.
///   Parameters:
///     rdi (u64) - the first number
///     rsi (u64) - the second number
///     rdx (int) - the fd to write to
///   Result:
///     rdi + rsi written to fd as 8 bytes.
sum_and_write_fd:
    add    rsi, rdi    # rsi += rdi
    push   rsi

    mov     rax, 1     # SYS_write
    mov     rdi, rdx   # fd
    lea     rsi, [rsp] # rsi = stack addr - 8 (sum result)
    mov     rdx, 8     # rdx (len) = 8
    syscall            # SYS_write

    pop     rsi        # pop rsi off the stack
    ret

/// This subroutine will timeout in an infinitely recurring loop
timeout_fn:
    b timeout_fn

/// This subroutine will simply call SYS_exit with retcode=42
///   Parameters:
///     x0 (int) - the exit status
exiting_fn:
    mov     x8, 93   // SYS_exit
    // status (x0) is passed as param
    svc     0

/// This subroutine will segfault by continuing execution into the next section
segfaulting_fn:

    .data
strHello:       .asciz "Hello, from assembly!\n"
strGoodbye:     .asciz "Goodbye, :(\n"
