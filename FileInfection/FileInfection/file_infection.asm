.386							;build for x86 
.model flat
option casemap:none

.data


.code

_main proc

start_code:
	call delta
delta:
	mov eax, 50



	ret

_main endp



end _main