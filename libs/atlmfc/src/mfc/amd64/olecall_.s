; This is a part of the Microsoft Foundation Classes C++ library.
; Copyright (C) 1992-1997 Microsoft Corporation
; All rights reserved.
;
; This source code is only intended as a supplement to the
; Microsoft Foundation Classes Reference and related
; electronic documentation provided with the library.
; See these sources for detailed information regarding the
; Microsoft Foundation Classes product.

PUBLIC	_AfxDispatchCall

_TEXT	SEGMENT

;_AfxDispatchCall(AFX_PMSG /*pfn*/, void* /*pArgs*/, UINT /*nSizeArgs*/)

_AfxDispatchCall PROC

	; at this point RCX contains value of pfn, RDX contains value of pArgs 
	; and R8 contains value of nSizeArgs.
	
	; get the return address
	mov rax, qword ptr [rsp]

	; save the return address
	mov qword ptr [rdx-8], rax

	; set the new stack pointer
	lea rsp, qword ptr [rdx-8]

	; save the pfn
	mov rax, rcx

	; set the first four float/double arguments
	movsd xmm0, qword ptr [rdx]
	movsd xmm1, qword ptr [rdx+8]
	movsd xmm2, qword ptr [rdx+16]
	movsd xmm3, qword ptr [rdx+24]

	; set the first four integer arguments [except for RDX]
	mov rcx, qword ptr [rdx]
	mov r8,  qword ptr [rdx+16]
	mov r9,  qword ptr [rdx+24]

	; Finally load up RDX
	mov rdx, qword ptr [rdx+8]

	; jump to the function
	jmp rax

_AfxDispatchCall ENDP

_TEXT	ENDS

END
