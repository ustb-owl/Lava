.arch armv7ve
.section .text

.global main
	.type  main, %function
main:
	push	{ lr }
	sub	sp, sp, #344
	add	r12, sp, #0
	str	r12, [sp, #76]
	add	r12, sp, #4
	str	r12, [sp, #80]
	add	r12, sp, #8
	str	r12, [sp, #84]
	add	r12, sp, #12
	str	r12, [sp, #88]
	add	r12, sp, #52
	str	r12, [sp, #92]
	add	r12, sp, #56
	str	r12, [sp, #96]
	add	r12, sp, #60
	str	r12, [sp, #100]
	add	r12, sp, #64
	str	r12, [sp, #104]
	add	r12, sp, #68
	str	r12, [sp, #108]
	add	r12, sp, #72
	str	r12, [sp, #112]
@ begin getelementptr
	ldr	r12, [sp, #88]
	str	r12, [sp, #116]
@ create getelementptr end
	mov	r12, #0
	str	r12, [sp, #120]
	ldr	r12, [sp, #120]
	ldr	r3, [sp, #84]
	str	r12, [r3]
	mov	r12, #0
	str	r12, [sp, #124]
	ldr	r12, [sp, #124]
	ldr	r3, [sp, #80]
	str	r12, [r3]
	b	.while_cond0

.while_cond0:
	ldr	r12, [sp, #80]
	ldr	r12, [r12]
	str	r12, [sp, #128]
	ldr	r12, [sp, #128]
	cmp	r12, #10
	blt	.loop_body0
	b	.while_end0

.loop_body0:
	ldr	r12, [sp, #80]
	ldr	r12, [r12]
	str	r12, [sp, #132]
@ begin getelementptr
	ldr	r12, [sp, #116]
	ldr	r3, [sp, #132]
	add	r12, r12, r3, lsl #2
	str	r12, [sp, #136]
@ create getelementptr end
	ldr	r12, [sp, #80]
	ldr	r12, [r12]
	str	r12, [sp, #140]
@ --- binary here ---
	ldr	r12, [sp, #140]
	add	r12, r12, #1
	str	r12, [sp, #144]
	ldr	r12, [sp, #144]
	ldr	r3, [sp, #136]
	str	r12, [r3]
	ldr	r12, [sp, #80]
	ldr	r12, [r12]
	str	r12, [sp, #148]
@ --- binary here ---
	ldr	r12, [sp, #148]
	add	r12, r12, #1
	str	r12, [sp, #152]
	ldr	r12, [sp, #152]
	ldr	r3, [sp, #80]
	str	r12, [r3]
	b	.while_cond0

.while_end0:
	mov	r12, #10
	str	r12, [sp, #156]
	ldr	r12, [sp, #156]
	ldr	r3, [sp, #108]
	str	r12, [r3]
	bl	getint
	mov	r12, r0
	str	r12, [sp, #160]
	ldr	r12, [sp, #160]
	ldr	r3, [sp, #92]
	str	r12, [r3]
	ldr	r12, [sp, #108]
	ldr	r12, [r12]
	str	r12, [sp, #164]
@ --- binary here ---
	ldr	r12, [sp, #164]
	sub	r12, r12, #1
	str	r12, [sp, #168]
	ldr	r12, [sp, #168]
	ldr	r3, [sp, #96]
	str	r12, [r3]
	mov	r12, #0
	str	r12, [sp, #172]
	ldr	r12, [sp, #172]
	ldr	r3, [sp, #100]
	str	r12, [r3]
	ldr	r12, [sp, #96]
	ldr	r12, [r12]
	str	r12, [sp, #176]
	ldr	r12, [sp, #100]
	ldr	r12, [r12]
	str	r12, [sp, #180]
@ --- binary here ---
	ldr	r12, [sp, #176]
	ldr	r3, [sp, #180]
	add	r12, r12, r3
	str	r12, [sp, #184]
@ --- binary here ---
	mov	r12, #2
	str	r12, [sp, #188]
	ldr	r12, [sp, #184]
	ldr	r3, [sp, #188]
	sdiv	r12, r12, r3
	str	r12, [sp, #192]
	ldr	r12, [sp, #192]
	ldr	r3, [sp, #104]
	str	r12, [r3]
	b	.while_cond1

.while_cond1:
	ldr	r12, [sp, #104]
	ldr	r12, [r12]
	str	r12, [sp, #196]
@ begin getelementptr
	ldr	r12, [sp, #116]
	ldr	r3, [sp, #196]
	add	r12, r12, r3, lsl #2
	str	r12, [sp, #200]
@ create getelementptr end
	ldr	r12, [sp, #200]
	ldr	r12, [r12]
	str	r12, [sp, #204]
	ldr	r12, [sp, #92]
	ldr	r12, [r12]
	str	r12, [sp, #208]
	ldr	r12, [sp, #204]
	ldr	r3, [sp, #208]
	cmp	r12, r3
	movne	r12, #1
	str	r12, [sp, #216]
	moveq	r12, #0
	str	r12, [sp, #216]
	beq	.lhs_true0
	b	.lhs_false0

.loop_body1:
	ldr	r12, [sp, #96]
	ldr	r12, [r12]
	str	r12, [sp, #220]
	ldr	r12, [sp, #100]
	ldr	r12, [r12]
	str	r12, [sp, #224]
@ --- binary here ---
	ldr	r12, [sp, #220]
	ldr	r3, [sp, #224]
	add	r12, r12, r3
	str	r12, [sp, #228]
@ --- binary here ---
	mov	r12, #2
	str	r12, [sp, #232]
	ldr	r12, [sp, #228]
	ldr	r3, [sp, #232]
	sdiv	r12, r12, r3
	str	r12, [sp, #236]
	ldr	r12, [sp, #236]
	ldr	r3, [sp, #104]
	str	r12, [r3]
	ldr	r12, [sp, #104]
	ldr	r12, [r12]
	str	r12, [sp, #240]
@ begin getelementptr
	ldr	r12, [sp, #116]
	ldr	r3, [sp, #240]
	add	r12, r12, r3, lsl #2
	str	r12, [sp, #244]
@ create getelementptr end
	ldr	r12, [sp, #92]
	ldr	r12, [r12]
	str	r12, [sp, #248]
	ldr	r12, [sp, #244]
	ldr	r12, [r12]
	str	r12, [sp, #252]
	ldr	r12, [sp, #248]
	ldr	r3, [sp, #252]
	cmp	r12, r3
	blt	.if_then0
	b	.if_else0

.while_end1:
	ldr	r12, [sp, #104]
	ldr	r12, [r12]
	str	r12, [sp, #256]
@ begin getelementptr
	ldr	r12, [sp, #116]
	ldr	r3, [sp, #256]
	add	r12, r12, r3, lsl #2
	str	r12, [sp, #260]
@ create getelementptr end
	ldr	r12, [sp, #92]
	ldr	r12, [r12]
	str	r12, [sp, #264]
	ldr	r12, [sp, #260]
	ldr	r12, [r12]
	str	r12, [sp, #268]
	ldr	r12, [sp, #264]
	ldr	r3, [sp, #268]
	cmp	r12, r3
	beq	.if_then1
	b	.if_else1

.lhs_true0:
	ldr	r12, [sp, #100]
	ldr	r12, [r12]
	str	r12, [sp, #272]
	ldr	r12, [sp, #96]
	ldr	r12, [r12]
	str	r12, [sp, #276]
	ldr	r12, [sp, #272]
	ldr	r3, [sp, #276]
	cmp	r12, r3
	movlt	r12, #1
	str	r12, [sp, #284]
	movge	r12, #0
	str	r12, [sp, #284]
@ --- binary here ---
	ldr	r12, [sp, #216]
	ldr	r3, [sp, #284]
	and	r12, r12, r3
	str	r12, [sp, #288]
	ldr	r12, [sp, #288]
	ldr	r3, [sp, #112]
	str	r12, [r3]
	b	.land_end0

.lhs_false0:
	ldr	r12, [sp, #216]
	ldr	r3, [sp, #112]
	str	r12, [r3]
	b	.land_end0

.land_end0:
	ldr	r12, [sp, #112]
	ldr	r12, [r12]
	str	r12, [sp, #292]
	mov	r12, #0
	str	r12, [sp, #296]
	ldr	r12, [sp, #296]
	ldr	r3, [sp, #292]
	cmp	r12, r3
	bne	.loop_body1
	b	.while_end1

.if_then0:
	ldr	r12, [sp, #104]
	ldr	r12, [r12]
	str	r12, [sp, #300]
@ --- binary here ---
	ldr	r12, [sp, #300]
	sub	r12, r12, #1
	str	r12, [sp, #304]
	ldr	r12, [sp, #304]
	ldr	r3, [sp, #96]
	str	r12, [r3]
	b	.while_cond1

.if_else0:
	ldr	r12, [sp, #104]
	ldr	r12, [r12]
	str	r12, [sp, #308]
@ --- binary here ---
	ldr	r12, [sp, #308]
	add	r12, r12, #1
	str	r12, [sp, #312]
	ldr	r12, [sp, #312]
	ldr	r3, [sp, #100]
	str	r12, [r3]
	b	.while_cond1

.if_then1:
	ldr	r12, [sp, #92]
	ldr	r12, [r12]
	str	r12, [sp, #316]
	ldr	r0, [sp, #316]
	bl	putint
	b	.if_end0

.if_else1:
	mov	r12, #0
	str	r12, [sp, #320]
	ldr	r12, [sp, #320]
	ldr	r3, [sp, #92]
	str	r12, [r3]
	ldr	r12, [sp, #92]
	ldr	r12, [r12]
	str	r12, [sp, #324]
	ldr	r0, [sp, #324]
	bl	putint
	b	.if_end0

.if_end0:
	mov	r12, #10
	str	r12, [sp, #328]
	ldr	r12, [sp, #328]
	ldr	r3, [sp, #92]
	str	r12, [r3]
	ldr	r12, [sp, #92]
	ldr	r12, [r12]
	str	r12, [sp, #332]
	ldr	r0, [sp, #332]
	bl	putch
	mov	r12, #0
	str	r12, [sp, #336]
	ldr	r12, [sp, #336]
	ldr	r3, [sp, #76]
	str	r12, [r3]
	ldr	r12, [sp, #76]
	ldr	r12, [r12]
	str	r12, [sp, #340]
	ldr	r0, [sp, #340]
	add	sp, sp, #344
	pop	{ lr }
	bx	lr


