;;
;; ChessCore (c)2008-2013 Andy Duplain <andy@trojanfoe.com>
;;
;; AsmX86.asm: Windows x86 assembler functions.
;;
;; See: http://msdn.microsoft.com/en-us/library/6xa169sk.aspx
;;
;; x86 __cdecl calling convention:
;; Parameters: pushed on the stack (right-to-left).
;; Return: EAX.  If returning 64-bit the EDX is also used.
;; Work registers: EAX, ECX and EDX.
;; Stack cleaned-up by called function.
;;

.686
.xmm
.model flat

OPTION LANGUAGE:C
OPTION PROLOGUE:NONE
OPTION EPILOGUE:NONE

.code

;;
;; Determine if the CPU supports the POPCNT instruction.
;;
PUBLIC x86HasPopcnt
x86HasPopcnt PROC
    mov eax, 01h
    cpuid                   ; ecx=feature info 1, edx=feature info 2
    xor eax, eax            ; prepare return value
    test ecx, 1 SHL 23
    jz not_popcnt
    mov eax, 1
not_popcnt:
    ret
x86HasPopcnt ENDP

;;
;; Count the number of bits set in a uint64_t
;;
;; [ESP+4], [ESP+8]: bb
;;
PUBLIC x86Popcnt
x86Popcnt PROC
    mov edx, dword ptr [esp+4]
    popcnt ecx, edx
    mov edx, dword ptr [esp+8]
    popcnt eax, edx
    add eax, ecx
    ret
x86Popcnt ENDP

;;
;; Return the offset of the lowest set bit in the bitmask
;;
;; [ESP+4], [ESP+8]: bb
;;
PUBLIC x86Lsb
x86Lsb PROC
    mov edx, dword ptr [esp+4]
    bsf eax, edx
    jz lsb_z1
    ret
lsb_z1:
    mov edx, dword ptr [esp+8]
    bsf eax, edx
    jz lsb_z2
    add eax, 32
    ret
lsb_z2:
    mov eax, 64         ; bb is empty
    ret
x86Lsb ENDP

;;
;; Return the offset of the lowest set bit in the bitmask and clear
;; the bit.  Also return the bit cleared.
;;
;; [ESP+4]: &bb
;; [ESP+8]: &bit
;;
PUBLIC x86Lsb2
x86Lsb2 PROC
    push ebx
    push ecx
    mov ebx, dword ptr [esp+12] ; &bb (+8 for pushes)
    mov edx, dword ptr [esp+16] ; &bit (+8 for pushes)
    mov ecx, dword ptr [ebx+0]  ; bb (low)
    bsf eax, ecx
    jz lsb2_z1
    push eax
    mov cl, al
    mov eax, 1
    shl eax, cl
    mov dword ptr [edx+0], eax  ; bit (low) found
    mov dword ptr [edx+4], 0    ; bit (high) found
    not eax
    mov ecx, dword ptr [ebx+0]  ; bb (low)
    and dword ptr [ebx+0], eax  ; clear bb (low) bit
    pop eax
    pop ecx
    pop ebx
    ret
lsb2_z1:
    mov ecx, dword ptr [ebx+4]  ; bb (high)
    bsf eax, ecx
    jz lsb2_z2
    push eax
    mov cl, al
    mov eax, 1
    shl eax, cl
    mov dword ptr [edx+0], 0    ; bit (low) found
    mov dword ptr [edx+4], eax  ; bit (high) found
    not eax
    and dword ptr [ebx+4], eax  ; clear bb (high) bit
    pop eax
    pop ecx
    pop ebx
    add eax, 32
    ret
lsb2_z2:                        ; bb is empty
    mov eax, 64
    mov dword ptr [edx+0], 0    ; bit (low)
    mov dword ptr [edx+4], 0    ; bit (high)
    pop ecx
    pop ebx
    ret
x86Lsb2 ENDP

;;
;; Byteswap for 16-bit values
;;
;; [ESP+4]: value
;;
PUBLIC x86Bswap16
x86Bswap16 PROC
    mov eax, dword ptr [esp+4]
    mov cl, 8
    rol ax, cl
    ret
x86Bswap16 ENDP

;;
;; Byteswap for 32-bit values
;;
;; [ESP+4]: value
;;
PUBLIC x86Bswap32
x86Bswap32 PROC
    mov eax, dword ptr [esp+4]
    bswap eax
    ret
x86Bswap32 ENDP

;;
;; Byteswap for 64-bit values
;;
;; [ESP+4], [ESP+8]: value
;;
PUBLIC x86BSwap64
x86Bswap64 PROC
    mov edx, dword ptr [esp+4]
    mov eax, dword ptr [esp+8]
    bswap eax
    bswap edx
    ret
x86Bswap64 ENDP

END
