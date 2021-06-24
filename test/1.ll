; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@matrix = global [20000000 x i32] zeroinitializer
@a = global [100000 x i32] zeroinitializer
@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%c\00", align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"%d:\00", align 1
@.str.3 = private unnamed_addr constant [4 x i8] c" %d\00", align 1

define i32 @transpose(i32 %n, i32* %matrix, i32 %rowsize) {
entry0:
  %n.addr = alloca i32
  %0 = alloca i32*
  store i32 %n, i32* %n.addr
  store i32* %matrix, i32** %0
  %rowsize.addr = alloca i32
  %1 = load i32*, i32** %0
  %retval = alloca i32
  store i32 %rowsize, i32* %rowsize.addr
  %2 = alloca i32
  %3 = alloca i32
  %4 = alloca i32
  %5 = alloca i32
  br label %body1

body1:                                            ; preds = %entry0
  %6 = load i32, i32* %n.addr
  %7 = load i32, i32* %rowsize.addr
  %8 = sdiv i32 %6, %7
  store i32 %8, i32* %2
  store i32 0, i32* %3
  store i32 0, i32* %4
  br label %while.cond0

while.cond0:                                      ; preds = %while.end0, %body1
  %9 = load i32, i32* %3
  %10 = load i32, i32* %2
  %11 = icmp slt i32 %9, %10
  br i1 %11, label %loop.body0, label %while.end1

loop.body0:                                       ; preds = %while.cond0
  br label %block2

while.end1:                                       ; preds = %while.cond0
  %12 = sub i32 0, 1
  store i32 %12, i32* %retval
  br label %func_exit3

block2:                                           ; preds = %loop.body0
  store i32 0, i32* %4
  br label %while.cond1

while.cond1:                                      ; preds = %block4, %if.end0, %block2
  %13 = load i32, i32* %4
  %14 = load i32, i32* %rowsize.addr
  %15 = icmp slt i32 %13, %14
  br i1 %15, label %loop.body1, label %while.end0

loop.body1:                                       ; preds = %while.cond1
  br label %block5

while.end0:                                       ; preds = %while.cond1
  %16 = load i32, i32* %3
  %17 = add i32 %16, 1
  store i32 %17, i32* %3
  br label %while.cond0

block5:                                           ; preds = %loop.body1
  %18 = load i32, i32* %3
  %19 = load i32, i32* %4
  %20 = icmp slt i32 %18, %19
  br i1 %20, label %if.then0, label %if.else0

if.then0:                                         ; preds = %block5
  br label %block4

if.else0:                                         ; preds = %block5
  br label %if.end0

if.end0:                                          ; preds = %block6, %if.else0
  %21 = load i32, i32* %3
  %22 = load i32, i32* %rowsize.addr
  %23 = mul i32 %21, %22
  %24 = load i32, i32* %4
  %25 = add i32 %23, %24
  %26 = getelementptr inbounds i32, i32* %1, i32 %25
  %27 = load i32, i32* %26
  store i32 %27, i32* %5
  %28 = load i32, i32* %4
  %29 = load i32, i32* %2
  %30 = mul i32 %28, %29
  %31 = load i32, i32* %3
  %32 = add i32 %30, %31
  %33 = getelementptr inbounds i32, i32* %1, i32 %32
  %34 = load i32, i32* %3
  %35 = load i32, i32* %rowsize.addr
  %36 = mul i32 %34, %35
  %37 = load i32, i32* %4
  %38 = add i32 %36, %37
  %39 = getelementptr inbounds i32, i32* %1, i32 %38
  %40 = load i32, i32* %39
  store i32 %40, i32* %33
  %41 = load i32, i32* %3
  %42 = load i32, i32* %rowsize.addr
  %43 = mul i32 %41, %42
  %44 = load i32, i32* %4
  %45 = add i32 %43, %44
  %46 = getelementptr inbounds i32, i32* %1, i32 %45
  %47 = load i32, i32* %5
  store i32 %47, i32* %46
  %48 = load i32, i32* %4
  %49 = add i32 %48, 1
  store i32 %49, i32* %4
  br label %while.cond1

block4:                                           ; preds = %if.then0
  %50 = load i32, i32* %4
  %51 = add i32 %50, 1
  store i32 %51, i32* %4
  br label %while.cond1

block6:                                           ; No predecessors!
  br label %if.end0

block7:                                           ; No predecessors!
  br label %func_exit3

func_exit3:                                       ; preds = %block7, %while.end1
  %52 = load i32, i32* %retval
  ret i32 %52
}

define i32 @main() {
entry0:
  %retval = alloca i32
  %0 = alloca i32
  %1 = alloca i32
  %2 = getelementptr inbounds [100000 x i32], [100000 x i32]* @a, i32 0, i32 0
  %3 = alloca i32
  %4 = getelementptr inbounds [20000000 x i32], [20000000 x i32]* @matrix, i32 0, i32 0
  %5 = alloca i32
  br label %body1

body1:                                            ; preds = %entry0
  %6 = call i32 @getint()
  store i32 %6, i32* %0
  %7 = call i32 @getarray(i32* %2)
  store i32 %7, i32* %1
  call void bitcast (void (i32)* @starttime to void ()*)()
  store i32 0, i32* %3
  br label %while.cond0

while.cond0:                                      ; preds = %block2, %body1
  %8 = load i32, i32* %3
  %9 = load i32, i32* %0
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %loop.body0, label %while.end0

loop.body0:                                       ; preds = %while.cond0
  br label %block2

while.end0:                                       ; preds = %while.cond0
  store i32 0, i32* %3
  br label %while.cond1

block2:                                           ; preds = %loop.body0
  %11 = load i32, i32* %3
  %12 = getelementptr inbounds i32, i32* %4, i32 %11
  %13 = load i32, i32* %3
  store i32 %13, i32* %12
  %14 = load i32, i32* %3
  %15 = add i32 %14, 1
  store i32 %15, i32* %3
  br label %while.cond0

while.cond1:                                      ; preds = %block3, %while.end0
  %16 = load i32, i32* %3
  %17 = load i32, i32* %1
  %18 = icmp slt i32 %16, %17
  br i1 %18, label %loop.body1, label %while.end1

loop.body1:                                       ; preds = %while.cond1
  br label %block3

while.end1:                                       ; preds = %while.cond1
  store i32 0, i32* %5
  store i32 0, i32* %3
  br label %while.cond2

block3:                                           ; preds = %loop.body1
  %19 = load i32, i32* %3
  %20 = getelementptr inbounds i32, i32* %2, i32 %19
  %21 = load i32, i32* %0
  %22 = load i32, i32* %20
  %23 = call i32 @transpose(i32 %21, i32* %4, i32 %22)
  %24 = load i32, i32* %3
  %25 = add i32 %24, 1
  store i32 %25, i32* %3
  br label %while.cond1

while.cond2:                                      ; preds = %block4, %while.end1
  %26 = load i32, i32* %3
  %27 = load i32, i32* %1
  %28 = icmp slt i32 %26, %27
  br i1 %28, label %loop.body2, label %while.end2

loop.body2:                                       ; preds = %while.cond2
  br label %block4

while.end2:                                       ; preds = %while.cond2
  %29 = load i32, i32* %5
  %30 = icmp slt i32 %29, 0
  br i1 %30, label %if.then0, label %if.else0

block4:                                           ; preds = %loop.body2
  %31 = load i32, i32* %3
  %32 = load i32, i32* %3
  %33 = mul i32 %31, %32
  %34 = load i32, i32* %3
  %35 = getelementptr inbounds i32, i32* %4, i32 %34
  %36 = load i32, i32* %35
  %37 = mul i32 %33, %36
  %38 = load i32, i32* %5
  %39 = add i32 %38, %37
  store i32 %39, i32* %5
  %40 = load i32, i32* %3
  %41 = add i32 %40, 1
  store i32 %41, i32* %3
  br label %while.cond2

if.then0:                                         ; preds = %while.end2
  %42 = load i32, i32* %5
  %43 = sub i32 0, %42
  store i32 %43, i32* %5
  br label %if.end0

if.else0:                                         ; preds = %while.end2
  br label %if.end0

if.end0:                                          ; preds = %if.else0, %if.then0
  call void bitcast (void (i32)* @stoptime to void ()*)()
  %44 = load i32, i32* %5
  call void @putint(i32 %44)
  call void @putch(i32 10)
  store i32 0, i32* %retval
  br label %func_exit5

block6:                                           ; No predecessors!
  br label %func_exit5

func_exit5:                                       ; preds = %block6, %if.end0
  %45 = load i32, i32* %retval
  ret i32 %45
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @getint() #0 {
  %1 = alloca i32, align 4
  %2 = call i32 (i8*, ...) @__isoc99_scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i64 0, i64 0), i32* %1)
  %3 = load i32, i32* %1, align 4
  ret i32 %3
}

declare dso_local i32 @__isoc99_scanf(i8*, ...) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @getch() #0 {
  %1 = alloca i8, align 1
  %2 = call i32 (i8*, ...) @__isoc99_scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.1, i64 0, i64 0), i8* %1)
  %3 = load i8, i8* %1, align 1
  %4 = sext i8 %3 to i32
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @getarray(i32* %0) #0 {
  %2 = alloca i32*, align 8
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32* %0, i32** %2, align 8
  %5 = call i32 (i8*, ...) @__isoc99_scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i64 0, i64 0), i32* %3)
  store i32 0, i32* %4, align 4
  br label %6

6:                                                ; preds = %16, %1
  %7 = load i32, i32* %4, align 4
  %8 = load i32, i32* %3, align 4
  %9 = icmp slt i32 %7, %8
  br i1 %9, label %10, label %19

10:                                               ; preds = %6
  %11 = load i32*, i32** %2, align 8
  %12 = load i32, i32* %4, align 4
  %13 = sext i32 %12 to i64
  %14 = getelementptr inbounds i32, i32* %11, i64 %13
  %15 = call i32 (i8*, ...) @__isoc99_scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i64 0, i64 0), i32* %14)
  br label %16

16:                                               ; preds = %10
  %17 = load i32, i32* %4, align 4
  %18 = add nsw i32 %17, 1
  store i32 %18, i32* %4, align 4
  br label %6

19:                                               ; preds = %6
  %20 = load i32, i32* %3, align 4
  ret i32 %20
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @putint(i32 %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  %4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i64 0, i64 0), i32 %3)
  ret void
}

declare dso_local i32 @printf(i8*, ...) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @putch(i32 %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  %4 = call i32 @putchar(i32 %3)
  ret void
}

declare dso_local i32 @putchar(i32) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @putarray(i32 %0, i32* %1) #0 {
  %3 = alloca i32, align 4
  %4 = alloca i32*, align 8
  %5 = alloca i32, align 4
  store i32 %0, i32* %3, align 4
  store i32* %1, i32** %4, align 8
  %6 = load i32, i32* %3, align 4
  %7 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.2, i64 0, i64 0), i32 %6)
  store i32 0, i32* %5, align 4
  br label %8

8:                                                ; preds = %19, %2
  %9 = load i32, i32* %5, align 4
  %10 = load i32, i32* %3, align 4
  %11 = icmp slt i32 %9, %10
  br i1 %11, label %12, label %22

12:                                               ; preds = %8
  %13 = load i32*, i32** %4, align 8
  %14 = load i32, i32* %5, align 4
  %15 = sext i32 %14 to i64
  %16 = getelementptr inbounds i32, i32* %13, i64 %15
  %17 = load i32, i32* %16, align 4
  %18 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([4 x i8], [4 x i8]* @.str.3, i64 0, i64 0), i32 %17)
  br label %19

19:                                               ; preds = %12
  %20 = load i32, i32* %5, align 4
  %21 = add nsw i32 %20, 1
  store i32 %21, i32* %5, align 4
  br label %8

22:                                               ; preds = %8
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @starttime(i32 %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @stoptime(i32 %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}
!llvm.module.flags = !{!1}

!0 = !{!"clang version 10.0.0-4ubuntu1 "}
!1 = !{i32 1, !"wchar_size", i32 4}
