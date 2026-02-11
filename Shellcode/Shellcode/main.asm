.386
.model flat
option casemap:none    

.code
_main proc
	call start_code
start_code:
	pop ebx
	sub ebx, start_code

	push ebp
	mov ebp, esp
	sub esp, 300h			;prelog 
	xor eax, eax
	mov [ebp - 4], eax		;*PEB
	mov [ebp - 8], eax		;*LDR
	mov [ebp - 12], eax		;LIST_ENTRY InMemoryOrderModuleList
	mov [ebp - 16], eax		;Base Kernel32.dll
	mov [ebp - 20], eax		;LoadLibrary
	mov [ebp - 24], eax		;HMODULE User32.dll
	mov [ebp - 28], eax		;MessageBoxA

	assume fs:nothing
	mov eax, fs:[30h]		;Get PEB
	assume fs:error

	mov eax, [eax + 0ch]		;*LDR
	mov eax, [eax + 14h]		;LIST_ENTRY InMemoryOrderModuleList
	mov [ebp - 0ch], eax

	lea edx, [ebx + offset wszKernel32]
	push edx
	push eax
	call GetModuleBase
	add esp, 8
	mov [ebp - 16], eax		;store kernel32 base

	lea edx, [ebx + offset sLoadLibraryA]
	push edx
	push eax
	call GetFuncAddr
	mov [ebp - 20], eax

	lea edx, [ebx + offset sUser32]
	push edx
	call eax		;LoadLibraryA

	test eax, eax
	jz Exit

	mov [ebp - 24], eax	;Store HMODULE User32
	lea edx, [ebx + offset sMessageBoxA]
	push edx
	push eax
	call GetFuncAddr
	mov [ebp - 28], eax

	push 0
	push 0
	push 0
	push 0
	call eax	;MessageBoxA

Exit:
	mov esp, ebp
	pop ebp
	ret
_main endp

GetModuleBase proc      ; arg1: LIST_ENTRY InMemoryOrderModuleList; arg2: dll name; return base dll
	push ebp
	mov ebp, esp
	sub esp, 50h
	mov esi, [ebp + 8]
	mov edi, [ebp + 12]
	xor ecx, ecx
LoopGetModuleBase:
	mov esi, [esi]		;pointer to flink
	mov ecx, esi		
	sub ecx, 8h			;pointer to _LDR_DATA_TABLE_ENTRY 
	add ecx, 24h		;pointer to UNICODE_STRING FullDllName; 
	mov ecx, [ecx + 4h]	;pointer to PWSTR  Buffer;	
		
	push ecx
	push edi
	call CompareUnicodeString
	add esp, 8

	test eax, eax
	jz LoopGetModuleBase
	mov eax, [esi - 8h + 18h]	;Pointer to PVOID DllBase;
	mov esp, ebp
	pop ebp
	ret
GetModuleBase endp

GetFuncAddr proc	;arg1: base dll ;arg2: winapiname ;return address of api
	push ebp
	mov ebp, esp
	sub esp, 50h
	xor eax, eax

	mov [ebp - 4], eax		;base dll
	mov [ebp - 8], eax		;RVA to AddressOfFunctions
	mov [ebp - 12], eax		;RVA to AddressOfNameOridinal

	mov esi, [ebp + 8]		;base dll
	mov [ebp - 4], esi
	add esi, [esi + 3ch]	;pointer to nt header
	mov esi, [esi + 78h]
	mov eax, [ebp - 4]
	add esi, eax			;pointer to _IMAGE_EXPORT_DIRECTORY 
	lea eax, [esi + 20h]	;RVA to AddressOfNames array
	lea edx, [esi + 1ch]	;RVA to AddressOfFunctions
	mov [ebp - 8], edx
	lea edx, [esi + 24h]	;RVA to AddressOfNameOridinal
	mov [ebp - 12], edx	

	mov esi, [eax]			;RVA AddressOfNames[0]
	add esi, [ebp - 4]		;VA AddressOfNames[0]
	xor ecx, ecx

	mov edi, [ebp + 12]		;arg2 winapi name
LoopFindNameFunc:
	mov eax, [esi]
	add eax, [ebp - 4]
	
	push edi
	push eax
	call CompareString
	add esp, 8

	add esi, 4				;AddressOfNames[i + 1]
	inc ecx					;inc index
	test eax, eax			;Check result compare
	jz LoopFindNameFunc

	mov edx, [ebp - 12]		;RVA to AddressOfNameOridinal
	mov eax, [ebp - 4]		;base dll
	add eax, [edx]			;VA AddressOfNameOridinal

	lea edx, [ecx * 2]		;calc index of oridinals - word
	add eax, edx
	xor edx, edx
	mov dx, [eax]			;store oridinal to dx

	dec edx				;dec index
	lea edx, [edx * 4]	;calc offset (dword)
	mov eax, [ebp - 8]	;RVA address func
	mov eax, [eax]
	add eax, [ebp - 4]	;VA address func
	add eax, edx		;VA function find

	mov eax, [eax]		;RVA function find
	add eax, [ebp - 4]	;VA function

	mov esp, ebp
	pop ebp
	ret
GetFuncAddr endp

CompareUnicodeString proc	;arg1: unicode string 1		;arg2: unicode string 2
	push ebp
	mov ebp, esp

	push esi
	push edi

	mov esi, [ebp + 8]	;arg1
	mov edi, [ebp + 12] ;arg2

	xor ecx, ecx
LoopCompare:
	mov al, [esi + ecx]
	mov dl, [edi + ecx]
	add ecx, 2
	push eax
	call ToLower
	push eax		;store char1
	push edx
	call ToLower
	add esp, 4		;restore stack
	mov dl, al		;store result to dl
	pop eax			;restore char1
	test al, al
	jnz Compare
	test dl, dl
	jnz Compare
	mov eax, 1
	jmp ExitCompare

Compare:
	cmp al, dl
	je LoopCompare
	xor eax, eax
	jmp ExitCompare

ExitCompare:

	pop edi
	pop esi
	
	mov esp, ebp
	pop ebp
	ret
CompareUnicodeString endp

CompareString proc	;arg1: string 1 ;arg2: string 2
	push ebp
	mov ebp, esp
	
	push esi
	push edi

	mov esi, [ebp + 8]	;arg1
	mov edi, [ebp + 12]	;arg2
LoopCompareString:
	mov al, byte ptr [esi]
	mov dl, byte ptr [edi]
	inc esi
	inc edi

	push eax
	call ToLower
	add esp, 4

	push eax
	push edx
	call ToLower
	add esp, 4
	mov dl, al
	pop eax

	test al, al
	jnz CompareByte
	test dl, dl
	jnz CompareByte
	mov eax, 1
	jmp ExitCompareString
CompareByte:
	cmp al, dl
	je LoopCompareString
	xor eax, eax
ExitCompareString:
	
	pop edi
	pop esi

	mov esp, ebp
	pop ebp
	ret
CompareString endp

ToLower proc	;arg1: char compare
	push ebp
	mov ebp, esp
	mov al, byte ptr [ebp + 8]	;char
	cmp al, 65  ;'A'
	jb EndToLower
	cmp al, 90  ;'Z'
	ja EndToLower
	add al, 32  ;to lower
EndToLower:
	mov esp, ebp
	pop ebp
	ret
ToLower endp

	wszKernel32 dw 'c',':','\','w','i','n','d','o','w','s','\','s','y','s','t','e','m','3','2','\','k','e','r','n','e','l','3','2','.', 'd','l','l', 0
	sLoadLibraryA db "LoadLibraryA", 0
	sMessageBoxA db "MessageBoxA", 0
	sUser32 db "User32.dll", 0
end _main