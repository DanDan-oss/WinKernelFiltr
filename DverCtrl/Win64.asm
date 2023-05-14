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
end