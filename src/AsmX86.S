/*
 * ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
 *
 * AsmX86.S: UNIX x86/x64 assembler functions.
 */

#ifdef __APPLE__
#define NAME(name) _##name
#else
#define NAME(name) name
#endif

// This assembler file deliberately avoids storing and using data as it becomes a
// huge pain in the arse to make this work with Position-Independent-Code (-fPIC),
// especially on the x86.

.text
.align 8

#if defined(__x86_64__)

/***********************************************************************************************
 *
 * x64 assembler.
 *
 * Calling convention:
 * Parameters: %rdi, %rsi, %rdx, and %rcx
 * Return: %rax.
 *
 * Additional Apple Mac details (see http://www.x86-64.org/documentation/abi-0.99.pdf):
 * Registers %rbp, %rbx and %r12 through %r15 “belong” to the calling function and the
 * called function is required to preserve their values.
 *
 ***********************************************************************************************/

/*
 * Test if the CPU has the popcnt instruction.
 */
.globl NAME(x64HasPopcnt);
NAME(x64HasPopcnt):
    pushq %rbx

    movl $1, %eax
    cpuid                   // ecx=feature info 1, edx=feature info 2

    xorq %rax, %rax

    testl $1 << 23, %ecx
    jz 1f
    movq $1, %rax

1:
    popq %rbx
    ret

/*
 * Count the number of bits set in the bitboard.
 *
 * %rdi: bb
 */
.globl NAME(x64Popcnt);
NAME(x64Popcnt):
    popcnt %rdi, %rax
    ret

/*
 * Return the offset of the lowest set bit in the bitboard.
 *
 * %rdi: bb
 */
.globl NAME(x64Lsb);
NAME(x64Lsb):
    bsfq %rdi, %rax
    jz 1f
    ret
1:
    movq $64, %rax         // 64=none
    ret

/*
 * Return the offset of the lowest set bit in the bitmask and clear
 * the bit.  Also return the bit cleared.
 *
 * %rdi: &bb
 * %rsi: &bit
 */
.globl NAME(x64Lsb2);
NAME(x64Lsb2):
    movq (%rdi), %rdx           // rdx = bb
    bsfq %rdx, %rax             // rax = lsb(bb)
    jz 1f
    movq $1, %rdx
    movb %al, %cl               // offset to shift
    shlq %cl, %rdx              // rdx = bit found
    movq %rdx, (%rsi)           // return bit found
    notq %rdx                   // rdx = ~rdx
    andq %rdx, (%rdi)           // bb &= ~bit
    ret
1:
    movl $64, %eax              // 64=none
    movq $0, (%rsi)
    ret

//
// Byteswap for 16-bit values
//
// %rdi: value
//
.globl NAME(x64Bswap16)
NAME(x64Bswap16):
    mov %rdi, %rax
    mov $8, %cl
    rolw %cl, %ax
    ret

//
// Byteswap for 32-bit values
//
// %rdi: value
//
.global NAME(x64Bswap32)
NAME(x64Bswap32):
    mov %rdi, %rax
    bswapl %eax
    ret

//
// Byteswap for 64-bit values
//
// %rdi: value
//
.globl NAME(x64Bswap64)
NAME(x64Bswap64):
    mov %rdi, %rax
    bswap %rax
    ret

#elif defined(__i386__)

/**********************************************************************************************
 *
 * x86 assembler.
 *
 * __cdecl calling convention:
 * Parameters: pushed on the stack (right-to-left).
 * Return: %eax.  If returning 64-bit the %edx is also used.
 * Work registers: %eax, %ecx, %edx.
 *
 * Apple details (see http://developer.apple.com/library/mac/#documentation/DeveloperTools/
 *                                Conceptual/LowLevelABI/000-Introduction/introduction.html)
 *
 **********************************************************************************************/

/*
 * Test if the CPU has the popcnt instruction.
 */
.globl NAME(x86HasPopcnt);
NAME(x86HasPopcnt):
    pushl %ebx

    movl $1, %eax
    cpuid                   // ecx=feature info 1, edx=feature info 2

    xorl %eax, %eax         // prepare return value

    testl $1 << 23, %ecx
    jz 1f
    movl $1, %eax

1:
    popl %ebx
    ret

/*
 * Count the number of bits set in a bitboard.
 *
 * 4(%esp): bb (low)
 * 8(%esp): bb (high)
 */
.global NAME(x86Popcnt);
NAME(x86Popcnt):
    popcnt 4(%esp), %edx
    popcnt 8(%esp), %eax
    add %edx, %eax
    ret

/*
 * Find the lowest set bit in a bitboard.
 *
 * 4(%esp): bb (low)
 * 8(%esp): bb (high)
 */
.globl NAME(x86Lsb);
NAME(x86Lsb):
    movl 4(%esp), %edx          // bb (low)
    bsf %edx, %eax
    jz 1f
    ret
1:
    movl 8(%esp), %edx          // bb (high)
    bsf %edx, %eax
    jz 2f
    add $32, %eax
    ret
2:
    movl $64, %eax
    ret

/*
 * Return the offset of the lowest set bit in the bitmask and clear
 * the bit.  Also return the bit cleared.
 *
 * 4(%esp): &bb
 * 8(%esp): &bit
 */
.globl NAME(x86Lsb2);
NAME(x86Lsb2):
    push %ebx
    push %ecx
    movl 12(%esp), %ebx         // &bb (+8 for pushes)
    movl 16(%esp), %edx         // &bit (+8 for pushes)
    movl 0(%ebx), %ecx          // bb (low)
    bsf %ecx, %eax
    jz 1f
    push %eax
    movb %al, %cl
    movl $1, %eax
    shl %cl, %eax
    movl %eax, 0(%edx)          // bit (low) found
    movl $0, 4(%edx)            // bit (high) found
    not %eax
    movl (%ebx), %ecx           // bb (low)
    and %eax, 0(%ebx)           // clear bb (low) bit
    pop %eax
    pop %ecx
    pop %ebx
    ret
1:
    movl 4(%ebx), %ecx          // bb (high)
    bsf %ecx, %eax
    jz 2f
    push %eax
    movb %al, %cl
    movl $1, %eax
    shl %cl, %eax
    movl $0, 0(%edx)            // bit (low) found
    movl %eax, 4(%edx)          // bit (high) found
    not %eax
    andl %eax, 4(%ebx)          // clear bb (high) bit
    pop %eax
    pop %ecx
    pop %ebx
    add $32, %eax
    ret
2:                              // bb is empty
    movl $64, %eax
    movl $0, 0(%edx)            // bit (low)
    movl $0, 4(%edx)            // bit (high)
    pop %ecx
    pop %ebx
    ret

//
// Byteswap for 16-bit values
//
// 4(%esp): value
//
.globl NAME(x86Bswap16)
NAME(x86Bswap16):
    movl 4(%esp), %eax
    mov $8, %cl
    rolw %cl, %ax
    ret

//
// Byteswap for 32-bit values
//
// 4(%esp): value
//
.global NAME(x86Bswap32)
NAME(x86Bswap32):
    movl 4(%esp), %eax
    bswap %eax
    ret

//
// Byteswap for 64-bit values
//
// 4(%esp), 8(%esp): value
//
.globl NAME(x86Bswap64)
NAME(x86Bswap64):
    mov 4(%esp), %edx
    mov 8(%esp), %eax
    bswap %eax
    bswap %edx
    ret

#endif // __x86_64__
