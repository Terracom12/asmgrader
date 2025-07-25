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

/// sum_and_write
///   sums two numbers and writes the result to stdout. Overflow may occur.
///   Parameters:
///     x0 (u64) - the first number
///     x1 (u64) - the second number
///   Result:
///     x0 + x1 written to stdout as 8 bytes.
sum_and_write:
    add     x0, x0, x1     // x0 += x1
    STR     x0, [sp, -8]!

    mov     x8, 64         // SYS_write
    mov     x0, 1          // fd param = stdout
    mov     x1, sp         // str param = stack addr (sum result)
    mov     x2, 8          // len param = 8
    svc     0              // SYS_write

    add     sp, sp, 8      // pop x0 off of the stack
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
