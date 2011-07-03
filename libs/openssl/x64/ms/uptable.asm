_DATA	SEGMENT
PUBLIC	OPENSSL_UplinkTable
OPENSSL_UplinkTable	DQ	22
	DQ	$lazy1
	DQ	$lazy2
	DQ	$lazy3
	DQ	$lazy4
	DQ	$lazy5
	DQ	$lazy6
	DQ	$lazy7
	DQ	$lazy8
	DQ	$lazy9
	DQ	$lazy10
	DQ	$lazy11
	DQ	$lazy12
	DQ	$lazy13
	DQ	$lazy14
	DQ	$lazy15
	DQ	$lazy16
	DQ	$lazy17
	DQ	$lazy18
	DQ	$lazy19
	DQ	$lazy20
	DQ	$lazy21
	DQ	$lazy22
_DATA	ENDS

_TEXT	SEGMENT
EXTERN	OPENSSL_Uplink:PROC
ALIGN	4
$lazy1	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,1
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*1
$lazy1	ENDP
ALIGN	4
$lazy2	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,2
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*2
$lazy2	ENDP
ALIGN	4
$lazy3	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,3
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*3
$lazy3	ENDP
ALIGN	4
$lazy4	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,4
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*4
$lazy4	ENDP
ALIGN	4
$lazy5	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,5
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*5
$lazy5	ENDP
ALIGN	4
$lazy6	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,6
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*6
$lazy6	ENDP
ALIGN	4
$lazy7	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,7
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*7
$lazy7	ENDP
ALIGN	4
$lazy8	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,8
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*8
$lazy8	ENDP
ALIGN	4
$lazy9	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,9
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*9
$lazy9	ENDP
ALIGN	4
$lazy10	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,10
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*10
$lazy10	ENDP
ALIGN	4
$lazy11	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,11
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*11
$lazy11	ENDP
ALIGN	4
$lazy12	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,12
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*12
$lazy12	ENDP
ALIGN	4
$lazy13	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,13
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*13
$lazy13	ENDP
ALIGN	4
$lazy14	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,14
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*14
$lazy14	ENDP
ALIGN	4
$lazy15	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,15
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*15
$lazy15	ENDP
ALIGN	4
$lazy16	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,16
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*16
$lazy16	ENDP
ALIGN	4
$lazy17	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,17
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*17
$lazy17	ENDP
ALIGN	4
$lazy18	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,18
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*18
$lazy18	ENDP
ALIGN	4
$lazy19	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,19
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*19
$lazy19	ENDP
ALIGN	4
$lazy20	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,20
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*20
$lazy20	ENDP
ALIGN	4
$lazy21	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,21
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*21
$lazy21	ENDP
ALIGN	4
$lazy22	PROC
	push	r9
	push	r8
	push	rdx
	push	rcx
	sub	rsp,40
	lea	rcx,OFFSET OPENSSL_UplinkTable
	mov	rdx,22
	call	OPENSSL_Uplink
	add	rsp,40
	pop	rcx
	pop	rdx
	pop	r8
	pop	r9
	jmp	QWORD PTR OPENSSL_UplinkTable+8*22
$lazy22	ENDP
_TEXT	ENDS
END
