.arch armv7ve
.section .text

.global EightWhile
	.type  EightWhile, %function
EightWhile:
	sub	sp, sp, #288
	ldr	r12, =f
	str	r12, [sp, #20]
	ldr	r12, =h
	str	r12, [sp, #24]
	ldr	r12, =g
	str	r12, [sp, #28]
	ldr	r12, =e
	str	r12, [sp, #32]
	add	r12, sp, #0
	str	r12, [sp, #36]
	add	r12, sp, #4
	str	r12, [sp, #40]
	add	r12, sp, #8
	str	r12, [sp, #44]
	add	r12, sp, #12
	str	r12, [sp, #48]
	add	r12, sp, #16
	str	r12, [sp, #52]
	mov	r12, #5
	str	r12, [sp, #56]
	ldr	r12, [sp, #56]
	ldr	r3, [sp, #40]
	str	r12, [r3]
	mov	r12, #6
	str	r12, [sp, #60]
	ldr	r12, [sp, #60]
	ldr	r3, [sp, #44]
	str	r12, [r3]
	mov	r12, #7
	str	r12, [sp, #64]
	ldr	r12, [sp, #64]
	ldr	r3, [sp, #48]
	str	r12, [r3]
	mov	r12, #10
	str	r12, [sp, #68]
	ldr	r12, [sp, #68]
	ldr	r3, [sp, #52]
	str	r12, [r3]
	b	.while_cond0

.while_cond0:
	ldr	r12, [sp, #40]
	ldr	r12, [r12]
	str	r12, [sp, #72]
	ldr	r12, [sp, #72]
	cmp	r12, #20
	blt	.loop_body0
	b	.while_end0

.loop_body0:
	ldr	r12, [sp, #40]
	ldr	r12, [r12]
	str	r12, [sp, #76]
@ --- binary here ---
	ldr	r12, [sp, #76]
	add	r12, r12, #3
	str	r12, [sp, #80]
	ldr	r12, [sp, #80]
	ldr	r3, [sp, #40]
	str	r12, [r3]
	b	.while_cond1

.while_end0:
	ldr	r12, [sp, #44]
	ldr	r12, [r12]
	str	r12, [sp, #84]
	ldr	r12, [sp, #52]
	ldr	r12, [r12]
	str	r12, [sp, #88]
@ --- binary here ---
	ldr	r12, [sp, #84]
	ldr	r3, [sp, #88]
	add	r12, r12, r3
	str	r12, [sp, #92]
	ldr	r12, [sp, #40]
	ldr	r12, [r12]
	str	r12, [sp, #96]
@ --- binary here ---
	ldr	r12, [sp, #96]
	ldr	r3, [sp, #92]
	add	r12, r12, r3
	str	r12, [sp, #100]
	ldr	r12, [sp, #48]
	ldr	r12, [r12]
	str	r12, [sp, #104]
@ --- binary here ---
	ldr	r12, [sp, #100]
	ldr	r3, [sp, #104]
	add	r12, r12, r3
	str	r12, [sp, #108]
	ldr	r12, [sp, #32]
	ldr	r12, [r12]
	str	r12, [sp, #112]
	ldr	r12, [sp, #52]
	ldr	r12, [r12]
	str	r12, [sp, #116]
@ --- binary here ---
	ldr	r12, [sp, #112]
	ldr	r3, [sp, #116]
	add	r12, r12, r3
	str	r12, [sp, #120]
	ldr	r12, [sp, #28]
	ldr	r12, [r12]
	str	r12, [sp, #124]
@ --- binary here ---
	ldr	r12, [sp, #120]
	ldr	r3, [sp, #124]
	sub	r12, r12, r3
	str	r12, [sp, #128]
	ldr	r12, [sp, #24]
	ldr	r12, [r12]
	str	r12, [sp, #132]
@ --- binary here ---
	ldr	r12, [sp, #128]
	ldr	r3, [sp, #132]
	add	r12, r12, r3
	str	r12, [sp, #136]
@ --- binary here ---
	ldr	r12, [sp, #108]
	ldr	r3, [sp, #136]
	sub	r12, r12, r3
	str	r12, [sp, #140]
	ldr	r12, [sp, #140]
	ldr	r3, [sp, #36]
	str	r12, [r3]
	ldr	r12, [sp, #36]
	ldr	r12, [r12]
	str	r12, [sp, #144]
	ldr	r0, [sp, #144]
	add	sp, sp, #288
	bx	lr

.while_cond1:
	ldr	r12, [sp, #44]
	ldr	r12, [r12]
	str	r12, [sp, #148]
	ldr	r12, [sp, #148]
	cmp	r12, #10
	blt	.loop_body1
	b	.while_end1

.loop_body1:
	ldr	r12, [sp, #44]
	ldr	r12, [r12]
	str	r12, [sp, #152]
@ --- binary here ---
	ldr	r12, [sp, #152]
	add	r12, r12, #1
	str	r12, [sp, #156]
	ldr	r12, [sp, #156]
	ldr	r3, [sp, #44]
	str	r12, [r3]
	b	.while_cond2

.while_end1:
	ldr	r12, [sp, #44]
	ldr	r12, [r12]
	str	r12, [sp, #160]
@ --- binary here ---
	ldr	r12, [sp, #160]
	sub	r12, r12, #2
	str	r12, [sp, #164]
	ldr	r12, [sp, #164]
	ldr	r3, [sp, #44]
	str	r12, [r3]
	b	.while_cond0

.while_cond2:
	ldr	r12, [sp, #48]
	ldr	r12, [r12]
	str	r12, [sp, #168]
	ldr	r12, [sp, #168]
	cmp	r12, #7
	beq	.loop_body2
	b	.while_end2

.loop_body2:
	ldr	r12, [sp, #48]
	ldr	r12, [r12]
	str	r12, [sp, #172]
@ --- binary here ---
	ldr	r12, [sp, #172]
	sub	r12, r12, #1
	str	r12, [sp, #176]
	ldr	r12, [sp, #176]
	ldr	r3, [sp, #48]
	str	r12, [r3]
	b	.while_cond3

.while_end2:
	ldr	r12, [sp, #48]
	ldr	r12, [r12]
	str	r12, [sp, #180]
@ --- binary here ---
	ldr	r12, [sp, #180]
	add	r12, r12, #1
	str	r12, [sp, #184]
	ldr	r12, [sp, #184]
	ldr	r3, [sp, #48]
	str	r12, [r3]
	b	.while_cond1

.while_cond3:
	ldr	r12, [sp, #52]
	ldr	r12, [r12]
	str	r12, [sp, #188]
	ldr	r12, [sp, #188]
	cmp	r12, #20
	blt	.loop_body3
	b	.while_end3

.loop_body3:
	ldr	r12, [sp, #52]
	ldr	r12, [r12]
	str	r12, [sp, #192]
@ --- binary here ---
	ldr	r12, [sp, #192]
	add	r12, r12, #3
	str	r12, [sp, #196]
	ldr	r12, [sp, #196]
	ldr	r3, [sp, #52]
	str	r12, [r3]
	b	.while_cond4

.while_end3:
	ldr	r12, [sp, #52]
	ldr	r12, [r12]
	str	r12, [sp, #200]
@ --- binary here ---
	ldr	r12, [sp, #200]
	sub	r12, r12, #1
	str	r12, [sp, #204]
	ldr	r12, [sp, #204]
	ldr	r3, [sp, #52]
	str	r12, [r3]
	b	.while_cond2

.while_cond4:
	ldr	r12, [sp, #32]
	ldr	r12, [r12]
	str	r12, [sp, #208]
	ldr	r12, [sp, #208]
	cmp	r12, #1
	bgt	.loop_body4
	b	.while_end4

.loop_body4:
	ldr	r12, [sp, #32]
	ldr	r12, [r12]
	str	r12, [sp, #212]
@ --- binary here ---
	ldr	r12, [sp, #212]
	sub	r12, r12, #1
	str	r12, [sp, #216]
	ldr	r12, [sp, #216]
	ldr	r3, [sp, #32]
	str	r12, [r3]
	b	.while_cond5

.while_end4:
	ldr	r12, [sp, #32]
	ldr	r12, [r12]
	str	r12, [sp, #220]
@ --- binary here ---
	ldr	r12, [sp, #220]
	add	r12, r12, #1
	str	r12, [sp, #224]
	ldr	r12, [sp, #224]
	ldr	r3, [sp, #32]
	str	r12, [r3]
	b	.while_cond3

.while_cond5:
	ldr	r12, [sp, #20]
	ldr	r12, [r12]
	str	r12, [sp, #228]
	ldr	r12, [sp, #228]
	cmp	r12, #2
	bgt	.loop_body5
	b	.while_end5

.loop_body5:
	ldr	r12, [sp, #20]
	ldr	r12, [r12]
	str	r12, [sp, #232]
@ --- binary here ---
	ldr	r12, [sp, #232]
	sub	r12, r12, #2
	str	r12, [sp, #236]
	ldr	r12, [sp, #236]
	ldr	r3, [sp, #20]
	str	r12, [r3]
	b	.while_cond6

.while_end5:
	ldr	r12, [sp, #20]
	ldr	r12, [r12]
	str	r12, [sp, #240]
@ --- binary here ---
	ldr	r12, [sp, #240]
	add	r12, r12, #1
	str	r12, [sp, #244]
	ldr	r12, [sp, #244]
	ldr	r3, [sp, #20]
	str	r12, [r3]
	b	.while_cond4

.while_cond6:
	ldr	r12, [sp, #28]
	ldr	r12, [r12]
	str	r12, [sp, #248]
	ldr	r12, [sp, #248]
	cmp	r12, #3
	blt	.loop_body6
	b	.while_end6

.loop_body6:
	ldr	r12, [sp, #28]
	ldr	r12, [r12]
	str	r12, [sp, #252]
@ --- binary here ---
	ldr	r12, [sp, #252]
	add	r12, r12, #10
	str	r12, [sp, #256]
	ldr	r12, [sp, #256]
	ldr	r3, [sp, #28]
	str	r12, [r3]
	b	.while_cond7

.while_end6:
	ldr	r12, [sp, #28]
	ldr	r12, [r12]
	str	r12, [sp, #260]
@ --- binary here ---
	ldr	r12, [sp, #260]
	sub	r12, r12, #8
	str	r12, [sp, #264]
	ldr	r12, [sp, #264]
	ldr	r3, [sp, #28]
	str	r12, [r3]
	b	.while_cond5

.while_cond7:
	ldr	r12, [sp, #24]
	ldr	r12, [r12]
	str	r12, [sp, #268]
	ldr	r12, [sp, #268]
	cmp	r12, #10
	blt	.loop_body7
	b	.while_end7

.loop_body7:
	ldr	r12, [sp, #24]
	ldr	r12, [r12]
	str	r12, [sp, #272]
@ --- binary here ---
	ldr	r12, [sp, #272]
	add	r12, r12, #8
	str	r12, [sp, #276]
	ldr	r12, [sp, #276]
	ldr	r3, [sp, #24]
	str	r12, [r3]
	b	.while_cond7

.while_end7:
	ldr	r12, [sp, #24]
	ldr	r12, [r12]
	str	r12, [sp, #280]
@ --- binary here ---
	ldr	r12, [sp, #280]
	sub	r12, r12, #1
	str	r12, [sp, #284]
	ldr	r12, [sp, #284]
	ldr	r3, [sp, #24]
	str	r12, [r3]
	b	.while_cond6


.global main
	.type  main, %function
main:
	push	{ lr }
	sub	sp, sp, #32
	add	r12, sp, #0
	str	r12, [sp, #4]
	mov	r12, #1
	str	r12, [sp, #8]
	ldr	r12, [sp, #8]
	ldr	r3, [sp, #28]
	str	r12, [r3]
	mov	r12, #2
	str	r12, [sp, #12]
	ldr	r12, [sp, #12]
	ldr	r3, [sp, #24]
	str	r12, [r3]
	mov	r12, #4
	str	r12, [sp, #16]
	ldr	r12, [sp, #16]
	ldr	r3, [sp, #32]
	str	r12, [r3]
	mov	r12, #6
	str	r12, [sp, #20]
	ldr	r12, [sp, #20]
	ldr	r3, [sp, #20]
	str	r12, [r3]
	bl	EightWhile
	mov	r12, r0
	str	r12, [sp, #24]
	ldr	r12, [sp, #24]
	ldr	r3, [sp, #4]
	str	r12, [r3]
	ldr	r12, [sp, #4]
	ldr	r12, [r12]
	str	r12, [sp, #28]
	ldr	r0, [sp, #28]
	add	sp, sp, #32
	pop	{ lr }
	bx	lr


.section .data
.align 4
.global g
	.type	g, %object
g:
	.zero	4

.global h
	.type	h, %object
h:
	.zero	4

.global f
	.type	f, %object
f:
	.zero	4

.global e
	.type	e, %object
e:
	.zero	4

