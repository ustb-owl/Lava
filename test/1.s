	.section	__TEXT,__text,regular,pure_instructions
	.build_version macos, 10, 15	sdk_version 10, 15, 4
	.globl	_getNumPos                      ## -- Begin function getNumPos
	.p2align	4, 0x90
_getNumPos:                             ## @getNumPos
	.cfi_startproc
## %bb.0:                               ## %entry
	movl	%edi, -20(%rsp)
	movl	%esi, -12(%rsp)
	movl	$1, -8(%rsp)
	movl	$0, -16(%rsp)
	.p2align	4, 0x90
LBB0_1:                                 ## %while.cond0
                                        ## =>This Inner Loop Header: Depth=1
	movl	-16(%rsp), %eax
	cmpl	-12(%rsp), %eax
	jge	LBB0_3
## %bb.2:                               ## %loop.body0
                                        ##   in Loop: Header=BB0_1 Depth=1
	movl	-20(%rsp), %eax
	cltd
	idivl	_base(%rip)
	movl	%eax, -20(%rsp)
	incl	-16(%rsp)
	jmp	LBB0_1
LBB0_3:                                 ## %while.end0
	movl	-20(%rsp), %eax
	cltd
	idivl	_base(%rip)
	movl	%edx, %eax
	movl	%edx, -4(%rsp)
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_radixSort                      ## -- Begin function radixSort
	.p2align	4, 0x90
_radixSort:                             ## @radixSort
	.cfi_startproc
## %bb.0:                               ## %entry
	pushq	%rbp
	.cfi_def_cfa_offset 16
	pushq	%r15
	.cfi_def_cfa_offset 24
	pushq	%r14
	.cfi_def_cfa_offset 32
	pushq	%r13
	.cfi_def_cfa_offset 40
	pushq	%r12
	.cfi_def_cfa_offset 48
	pushq	%rbx
	.cfi_def_cfa_offset 56
	subq	$40, %rsp
	.cfi_def_cfa_offset 96
	.cfi_offset %rbx, -56
	.cfi_offset %r12, -48
	.cfi_offset %r13, -40
	.cfi_offset %r14, -32
	.cfi_offset %r15, -24
	.cfi_offset %rbp, -16
	movl	%edi, 8(%rsp)
	movq	%rsi, 32(%rsp)
	movl	%edx, 16(%rsp)
	movl	%ecx, 28(%rsp)
	cmpl	$-1, %edi
	sete	%al
	incl	%edx
	cmpl	%ecx, %edx
	setge	%cl
	orb	%al, %cl
	je	LBB1_1
LBB1_19:                                ## %func_exit
	addq	$40, %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	retq
LBB1_1:                                 ## %if.else0
	movq	%rsi, %r14
	leaq	___const.radixSort.0(%rip), %rbp
	leaq	___const.radixSort.1(%rip), %r15
	leaq	___const.radixSort.2(%rip), %r12
	movl	16(%rsp), %eax
	movl	%eax, 4(%rsp)
	.p2align	4, 0x90
LBB1_2:                                 ## %while.cond0
                                        ## =>This Inner Loop Header: Depth=1
	movl	4(%rsp), %eax
	cmpl	28(%rsp), %eax
	jge	LBB1_4
## %bb.3:                               ## %loop.body0
                                        ##   in Loop: Header=BB1_2 Depth=1
	movslq	4(%rsp), %rax
	movl	(%r14,%rax,4), %edi
	movl	8(%rsp), %esi
	callq	_getNumPos
	movslq	%eax, %r13
	movslq	4(%rsp), %rax
	movl	(%r14,%rax,4), %edi
	movl	8(%rsp), %esi
	callq	_getNumPos
	cltq
	movl	(%r12,%rax,4), %eax
	incl	%eax
	movl	%eax, (%r12,%r13,4)
	incl	4(%rsp)
	jmp	LBB1_2
LBB1_4:                                 ## %while.end0
	movl	16(%rsp), %eax
	movl	%eax, (%rbp)
	movl	16(%rsp), %eax
	addl	(%r12), %eax
	movl	%eax, (%r15)
	movl	$1, 4(%rsp)
	.p2align	4, 0x90
LBB1_5:                                 ## %while.cond1
                                        ## =>This Inner Loop Header: Depth=1
	movl	4(%rsp), %eax
	cmpl	_base(%rip), %eax
	jge	LBB1_7
## %bb.6:                               ## %loop.body1
                                        ##   in Loop: Header=BB1_5 Depth=1
	movslq	4(%rsp), %rax
	leal	-1(%rax), %ecx
	movslq	%ecx, %rcx
	movl	(%r15,%rcx,4), %ecx
	movl	%ecx, (%rbp,%rax,4)
	movslq	4(%rsp), %rax
	movl	(%rbp,%rax,4), %ecx
	addl	(%r12,%rax,4), %ecx
	movl	%ecx, (%r15,%rax,4)
	incl	4(%rsp)
	jmp	LBB1_5
LBB1_7:                                 ## %while.end1
	movl	$0, 4(%rsp)
	jmp	LBB1_8
	.p2align	4, 0x90
LBB1_20:                                ## %while.end2
                                        ##   in Loop: Header=BB1_8 Depth=1
	incl	4(%rsp)
LBB1_8:                                 ## %while.cond2
                                        ## =>This Loop Header: Depth=1
                                        ##     Child Loop BB1_15 Depth 2
                                        ##       Child Loop BB1_17 Depth 3
	movl	4(%rsp), %eax
	cmpl	_base(%rip), %eax
	jl	LBB1_15
	jmp	LBB1_9
	.p2align	4, 0x90
LBB1_14:                                ## %while.end4
                                        ##   in Loop: Header=BB1_15 Depth=2
	movslq	4(%rsp), %rax
	movslq	(%rbp,%rax,4), %rax
	movl	24(%rsp), %ecx
	movl	%ecx, (%r14,%rax,4)
	movslq	4(%rsp), %rax
	incl	(%rbp,%rax,4)
LBB1_15:                                ## %while.cond3
                                        ##   Parent Loop BB1_8 Depth=1
                                        ## =>  This Loop Header: Depth=2
                                        ##       Child Loop BB1_17 Depth 3
	movslq	4(%rsp), %rax
	movl	(%rbp,%rax,4), %ecx
	cmpl	(%r15,%rax,4), %ecx
	jge	LBB1_20
## %bb.16:                              ## %loop.body3
                                        ##   in Loop: Header=BB1_15 Depth=2
	movslq	4(%rsp), %rax
	movslq	(%rbp,%rax,4), %rax
	movl	(%r14,%rax,4), %eax
	movl	%eax, 24(%rsp)
	.p2align	4, 0x90
LBB1_17:                                ## %while.cond5
                                        ##   Parent Loop BB1_8 Depth=1
                                        ##     Parent Loop BB1_15 Depth=2
                                        ## =>    This Inner Loop Header: Depth=3
	movl	24(%rsp), %edi
	movl	8(%rsp), %esi
	callq	_getNumPos
	cmpl	4(%rsp), %eax
	je	LBB1_14
## %bb.18:                              ## %loop.body4
                                        ##   in Loop: Header=BB1_17 Depth=3
	movl	24(%rsp), %edi
	movl	%edi, 20(%rsp)
	movl	8(%rsp), %esi
	callq	_getNumPos
	cltq
	movslq	(%rbp,%rax,4), %rax
	movl	(%r14,%rax,4), %eax
	movl	%eax, 24(%rsp)
	movl	20(%rsp), %edi
	movl	8(%rsp), %esi
	callq	_getNumPos
	cltq
	movslq	(%rbp,%rax,4), %rax
	movl	20(%rsp), %ecx
	movl	%ecx, (%r14,%rax,4)
	movl	20(%rsp), %edi
	movl	8(%rsp), %esi
	callq	_getNumPos
	movslq	%eax, %rbx
	movl	20(%rsp), %edi
	movl	8(%rsp), %esi
	callq	_getNumPos
	cltq
	movl	(%rbp,%rax,4), %eax
	incl	%eax
	movl	%eax, (%rbp,%rbx,4)
	jmp	LBB1_17
LBB1_9:                                 ## %while.end3
	movl	16(%rsp), %eax
	movl	%eax, 12(%rsp)
	movl	%eax, (%rbp)
	movl	16(%rsp), %eax
	addl	(%r12), %eax
	movl	%eax, (%r15)
	movl	$0, 12(%rsp)
	jmp	LBB1_10
	.p2align	4, 0x90
LBB1_13:                                ## %if.end0
                                        ##   in Loop: Header=BB1_10 Depth=1
	movl	8(%rsp), %edi
	decl	%edi
	movslq	12(%rsp), %rax
	movl	(%rbp,%rax,4), %edx
	movl	(%r15,%rax,4), %ecx
	movq	%r14, %rsi
	callq	_radixSort
	incl	12(%rsp)
LBB1_10:                                ## %while.cond4
                                        ## =>This Inner Loop Header: Depth=1
	movl	12(%rsp), %eax
	cmpl	_base(%rip), %eax
	jge	LBB1_19
## %bb.11:                              ## %loop.body5
                                        ##   in Loop: Header=BB1_10 Depth=1
	cmpl	$0, 12(%rsp)
	jle	LBB1_13
## %bb.12:                              ## %if.then1
                                        ##   in Loop: Header=BB1_10 Depth=1
	movslq	12(%rsp), %rax
	leal	-1(%rax), %ecx
	movslq	%ecx, %rcx
	movl	(%r15,%rcx,4), %ecx
	movl	%ecx, (%rbp,%rax,4)
	movslq	12(%rsp), %rax
	movl	(%rbp,%rax,4), %ecx
	addl	(%r12,%rax,4), %ecx
	movl	%ecx, (%r15,%rax,4)
	jmp	LBB1_13
	.cfi_endproc
                                        ## -- End function
	.globl	_main                           ## -- Begin function main
	.p2align	4, 0x90
_main:                                  ## @main
	.cfi_startproc
## %bb.0:                               ## %entry
	pushq	%rbx
	.cfi_def_cfa_offset 16
	subq	$16, %rsp
	.cfi_def_cfa_offset 32
	.cfi_offset %rbx, -16
	leaq	_a(%rip), %rbx
	movq	%rbx, %rdi
	callq	_getarray
	movl	%eax, 8(%rsp)
	callq	_starttime
	movl	8(%rsp), %ecx
	movl	$8, %edi
	movq	%rbx, %rsi
	xorl	%edx, %edx
	callq	_radixSort
	movl	$0, 4(%rsp)
	.p2align	4, 0x90
LBB2_1:                                 ## %while.cond0
                                        ## =>This Inner Loop Header: Depth=1
	movl	4(%rsp), %eax
	cmpl	8(%rsp), %eax
	jge	LBB2_3
## %bb.2:                               ## %loop.body0
                                        ##   in Loop: Header=BB2_1 Depth=1
	movslq	4(%rsp), %rcx
	leal	2(%rcx), %esi
	movl	(%rbx,%rcx,4), %eax
	cltd
	idivl	%esi
	imull	%ecx, %edx
	addl	%edx, _ans(%rip)
	leal	1(%rcx), %eax
	movl	%eax, 4(%rsp)
	jmp	LBB2_1
LBB2_3:                                 ## %while.end0
	cmpl	$0, _ans(%rip)
	jns	LBB2_5
## %bb.4:                               ## %if.then0
	negl	_ans(%rip)
LBB2_5:                                 ## %if.end0
	callq	_stoptime
	movl	_ans(%rip), %edi
	callq	_putint
	movl	$10, %edi
	callq	_putch
	movl	$0, 12(%rsp)
	xorl	%eax, %eax
	addq	$16, %rsp
	popq	%rbx
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_getint                         ## -- Begin function getint
	.p2align	4, 0x90
_getint:                                ## @getint
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	leaq	L_.str(%rip), %rdi
	leaq	-4(%rbp), %rsi
	movb	$0, %al
	callq	_scanf
	movl	-4(%rbp), %eax
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_getch                          ## -- Begin function getch
	.p2align	4, 0x90
_getch:                                 ## @getch
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	leaq	L_.str.1(%rip), %rdi
	leaq	-1(%rbp), %rsi
	movb	$0, %al
	callq	_scanf
	movsbl	-1(%rbp), %eax
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_getarray                       ## -- Begin function getarray
	.p2align	4, 0x90
_getarray:                              ## @getarray
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movq	%rdi, -16(%rbp)
	leaq	L_.str(%rip), %rdi
	leaq	-8(%rbp), %rsi
	movb	$0, %al
	callq	_scanf
	movl	$0, -4(%rbp)
LBB5_1:                                 ## =>This Inner Loop Header: Depth=1
	movl	-4(%rbp), %eax
	cmpl	-8(%rbp), %eax
	jge	LBB5_4
## %bb.2:                               ##   in Loop: Header=BB5_1 Depth=1
	movq	-16(%rbp), %rsi
	movslq	-4(%rbp), %rax
	shlq	$2, %rax
	addq	%rax, %rsi
	leaq	L_.str(%rip), %rdi
	movb	$0, %al
	callq	_scanf
## %bb.3:                               ##   in Loop: Header=BB5_1 Depth=1
	movl	-4(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	LBB5_1
LBB5_4:
	movl	-8(%rbp), %eax
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_putint                         ## -- Begin function putint
	.p2align	4, 0x90
_putint:                                ## @putint
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %esi
	leaq	L_.str(%rip), %rdi
	movb	$0, %al
	callq	_printf
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_putch                          ## -- Begin function putch
	.p2align	4, 0x90
_putch:                                 ## @putch
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	%edi, -4(%rbp)
	movl	-4(%rbp), %edi
	callq	_putchar
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_putarray                       ## -- Begin function putarray
	.p2align	4, 0x90
_putarray:                              ## @putarray
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$16, %rsp
	movl	%edi, -8(%rbp)
	movq	%rsi, -16(%rbp)
	movl	-8(%rbp), %esi
	leaq	L_.str.2(%rip), %rdi
	movb	$0, %al
	callq	_printf
	movl	$0, -4(%rbp)
LBB8_1:                                 ## =>This Inner Loop Header: Depth=1
	movl	-4(%rbp), %eax
	cmpl	-8(%rbp), %eax
	jge	LBB8_4
## %bb.2:                               ##   in Loop: Header=BB8_1 Depth=1
	movq	-16(%rbp), %rax
	movslq	-4(%rbp), %rcx
	movl	(%rax,%rcx,4), %esi
	leaq	L_.str.3(%rip), %rdi
	movb	$0, %al
	callq	_printf
## %bb.3:                               ##   in Loop: Header=BB8_1 Depth=1
	movl	-4(%rbp), %eax
	addl	$1, %eax
	movl	%eax, -4(%rbp)
	jmp	LBB8_1
LBB8_4:
	addq	$16, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_starttime                      ## -- Begin function starttime
	.p2align	4, 0x90
_starttime:                             ## @starttime
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$4, %rsp
	movl	%edi, -4(%rbp)
	addq	$4, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.globl	_stoptime                       ## -- Begin function stoptime
	.p2align	4, 0x90
_stoptime:                              ## @stoptime
	.cfi_startproc
## %bb.0:
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	subq	$4, %rsp
	movl	%edi, -4(%rbp)
	addq	$4, %rsp
	popq	%rbp
	retq
	.cfi_endproc
                                        ## -- End function
	.section	__DATA,__data
	.globl	_base                           ## @base
	.p2align	2
_base:
	.long	16                              ## 0x10

	.globl	___const.radixSort.0            ## @__const.radixSort.0
.zerofill __DATA,__common,___const.radixSort.0,64,4
	.globl	___const.radixSort.1            ## @__const.radixSort.1
.zerofill __DATA,__common,___const.radixSort.1,64,4
	.globl	___const.radixSort.2            ## @__const.radixSort.2
.zerofill __DATA,__common,___const.radixSort.2,64,4
	.globl	_a                              ## @a
.zerofill __DATA,__common,_a,120000040,4
	.globl	_ans                            ## @ans
.zerofill __DATA,__common,_ans,4,2
	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ## @.str
	.asciz	"%d"

L_.str.1:                               ## @.str.1
	.asciz	"%c"

L_.str.2:                               ## @.str.2
	.asciz	"%d:"

L_.str.3:                               ## @.str.3
	.asciz	" %d"

.subsections_via_symbols
