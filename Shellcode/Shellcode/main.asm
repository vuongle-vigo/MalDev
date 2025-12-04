.386
.model flat
option casemap:none    

.code
_main proc
	push ebp
	mov ebp, esp
	sub esp, 300h				;prelog 
	xor eax, eax
	mov [ebp - 04h], eax		;*PEB
	mov [ebp - 08h], eax		;*LDR

	assume fs:nothing
	mov eax, fs:[30h]
	assume fs:error
	mov [ebp - 04h], eax
	add eax, 0ch
	mov [ebp - 08h], eax

	mov esp, ebp
	pop ebp
	ret
_main endp

end _main