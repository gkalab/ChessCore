;;
;; ChessLib (c)2008-2011 Andy Duplain <andy@trojanfoe.com>
;;
;; AsmX64.asm: Windows x86-64-assembler functions.
;;
;; See: http://msdn.microsoft.com/en-us/library/ms235286.aspx
;;
;; Parameters: (left-to-right) RCX, RDX, R8, and R9.
;; Return: RAX.
;; Work registers: RAX, RCX, RDX, R8, R9, R10, and R11.
;;

.data

C55             QWORD   05555555555555555h
C33             QWORD   03333333333333333h
C0F             QWORD   00F0F0F0F0F0F0F0Fh
C01             QWORD   00101010101010101h
CFE             QWORD   0fefefefefefefefeh
C7F             QWORD   07f7f7f7f7f7f7f7fh

.code

;;
;; Determine if the CPU supports the POPCNT instruction.
;;
x64HasPopcnt PROC
    mov eax, 01h
    cpuid                   ; ecx=feature info 1, edx=feature info 2
    xor eax, eax            ; prepare return value
    test ecx, 1 SHL 23
    jz not_popcnt
    mov eax, 1
not_popcnt:
    ret 0
x64HasPopcnt ENDP

;;
;; Count the number of bits set in a uint64
;;
;; RCX: bb
;;
x64Popcnt PROC
    popcnt rax, rcx
    ret 0
x64Popcnt ENDP

;;
;; Return the offset of the lowest set bit in the bitmask
;;
;; RCX: bb
;;
x64Lsb PROC
    bsf rax, rcx
    jz lsb_empty
    ret 0
lsb_empty:
    mov rax, 64         ; bb was empty
    ret 0
x64Lsb ENDP

;;
;; Return the offset of the lowest set bit in the bitmask and clear
;; the bit.  Also return the bit cleared.
;;
;; RCX: &bb
;; RDX: &bit
;;
x64Lsb2 PROC
    mov r10, rcx                ; save address of bb (we need to use cl)
    mov r8, qword ptr [rcx]     ; r8 = bb
    bsf rax, r8                 ; rax = lsb(bb)
    jz lsb2_empty
    mov r9, 1
    mov cl, al                  ; offset to shift
    shl r9, cl                  ; r9 = bit found
    mov qword ptr [rdx], r9     ; return bit found
    not r9                      ; r9 = ~r9
    and r8, r9                  ; bb &= ~bit
    mov qword ptr [r10], r8     ; return bb with bit clear
    ret 0
lsb2_empty:
    mov rax, 64                 ; bb was empty
    mov qword ptr [rdx], 0
    ret 0
x64Lsb2 ENDP

;;
;; Byteswap for 16-bit values
;;
;; RCX: value
;;
PUBLIC x64Bswap16
x64Bswap16 PROC
    mov rax, rcx
    mov cl, 8
    rol ax, cl
    ret
x64Bswap16 ENDP

;;
;; Byteswap for 32-bit values
;;
;; RCX: value
;;
PUBLIC x64Bswap32
x64Bswap32 PROC
    mov rax, rcx
    bswap eax
    ret
x64Bswap32 ENDP

;;
;; Byteswap for 64-bit values
;;
;; RCX: value
;;
PUBLIC x64BSwap64
x64Bswap64 PROC
    mov rax, rcx
    bswap rax
    ret
x64Bswap64 ENDP

END
