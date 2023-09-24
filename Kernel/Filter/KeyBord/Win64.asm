; Íâ²¿Cº¯Êý
p2cUserFilter PROTO C

.data
	BaseSetLastNTError qword 0
.const

.code

; HANDLE (__stdcall *OpenProcess)(DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwProcessId)
ccOpenProcess proc
	mov r11,rsp
	sub rsp,68h
	and qword ptr [r11-40h],0
	lea r9,qword ptr[r11-48h]
	movsxd rax,r8d
	xorps xmm0,xmm0
	mov dword ptr [rsp+30h],30
	lea r8,qword ptr [r11-38h]
	and qword ptr [r11-30h],0
	neg edx
	mov qword ptr [r11-48h],rax
	mov edx,ecx
	lea rcx,qword ptr[r11+20h]
	sbb eax,eax
	and eax,2
	mov dword ptr [rsp+48h],eax
	and qword ptr [r11-28h],0
	movdqu xmmword ptr [rsp+50h],xmm0
	call ccNtOpenProcess
	nop dword ptr [rax+rax+00h]
	test eax,eax
	js ss2
	mov rax,qword ptr [rsp+88h]

ss3:
	add rsp,68h
	ret

ss2:
	mov ecx,eax
	mov rax, BaseSetLastNTError
	call rax
	xor eax,eax
	jmp ss3

ccOpenProcess endp

; NTSTATUS NtOpenProcess(PHANDLE ProcessHandle,ACCESS_MASK DesiredAccess,
;  POBJECT_ATTRIBUTES ObjectAttributes, PCLIENT_ID ClientId);
ccNtOpenProcess proc
	mov r10,rcx
	mov eax,26h
	test byte ptr [7FFE0308h],1
	jne xd1
	syscall
	ret 
xd1:
	int 2Eh
	ret 
ccNtOpenProcess endp

;void* p2cGetIdt(int offset)£»
ccP2cGetIdt proc
	push rbp
	sub rsp, 032h

	lea rcx, qword ptr [rbp+08h]
	sidt qword ptr [rcx]
	mov	rax, qword ptr [rcx+08h]

	add rsp, 032h
	pop rbp
ccP2cGetIdt endp

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
p2cInterrupProc endp

end