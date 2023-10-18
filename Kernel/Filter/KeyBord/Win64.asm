;https://blog.csdn.net/m0_37599645/article/details/111773180
; Íâ²¿Cº¯Êý
p2cUserFilter PROTO C

.data
	BaseSetLastNTError qword 0
.const

.code

;VOID* GetIdtBase(PP2C_IDTR64 idtr);
GetIdtBase proc
	push rbp
	sub rsp, 010h

	sidt qword ptr [rsp]

	mov ax, word ptr [rsp]
	mov word ptr[rcx], ax
	mov rax, qword ptr[rsp+2h]
	mov qword ptr[rcx+2], rax
	mov rax, rcx

	add	rsp, 010h
	pop rbp
	ret
GetIdtBase endp

p2cInterrupProc proc uses rax, 
	g_p2c_old:qword
	;pushad	
	;pushfd 
	push rax
	push rcx
	push rdx
	push rbx
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	push rbp
	push rsp
	pushfq
		
	call p2cUserFilter

	;popfd
	;pushad 
	popfq
	pop rsp
	pop rbp
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rbx
	pop rdx
	pop rcx
	pop rax
	jmp g_p2c_old
	ret
p2cInterrupProc endp


end