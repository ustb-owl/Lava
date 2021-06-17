; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

@n = global i32 0
@a = global [10000000 x i32] zeroinitializer
@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%c\00", align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"%d:\00", align 1
@.str.3 = private unnamed_addr constant [4 x i8] c" %d\00", align 1

define i32 @meanless_calculation(i32 %x, i32 %y) {
entry0:
  %x.addr = alloca i32, align 4
  %y.addr = alloca i32, align 4
  store i32 %x, i32* %x.addr, align 4
  %retval = alloca i32, align 4
  store i32 %y, i32* %y.addr, align 4
  %0 = alloca i32, align 4
  %1 = alloca i32, align 4
  %2 = alloca i1, align 1
  store i32 0, i32* %0, align 4
  store i32 0, i32* %1, align 4
  br label %while.cond0

while.cond0:                                      ; preds = %loop.body0, %entry0
  %3 = load i32, i32* %0, align 4
  %4 = load i32, i32* %x.addr, align 4
  %5 = icmp slt i32 %3, %4
  br i1 %5, label %lhs.true0, label %lhs.false1

loop.body0:                                       ; preds = %land.end2
  %6 = load i32, i32* %0, align 4
  %7 = add i32 %6, 1
  store i32 %7, i32* %0, align 4
  %8 = load i32, i32* %1, align 4
  %9 = load i32, i32* %x.addr, align 4
  %10 = add i32 %8, %9
  %11 = load i32, i32* %0, align 4
  %12 = add i32 %10, %11
  store i32 %12, i32* %1, align 4
  br label %while.cond0

while.end0:                                       ; preds = %land.end2
  %13 = load i32, i32* %1, align 4
  store i32 %13, i32* %retval, align 4
  %14 = load i32, i32* %retval, align 4
  ret i32 %14

lhs.true0:                                        ; preds = %while.cond0
  %15 = load i32, i32* %0, align 4
  %16 = load i32, i32* %y.addr, align 4
  %17 = icmp slt i32 %15, %16
  %18 = and i1 %5, %17
  store i1 %18, i1* %2, align 1
  br label %land.end2

lhs.false1:                                       ; preds = %while.cond0
  store i1 %5, i1* %2, align 1
  br label %land.end2

land.end2:                                        ; preds = %lhs.false1, %lhs.true0
  %19 = load i1, i1* %2, align 1
  %20 = icmp ne i1 false, %19
  br i1 %20, label %loop.body0, label %while.end0
}

define i32 @swap(i32* %arr, i32 %l, i32 %r) {
entry0:
  %0 = alloca i32*, align 8
  store i32* %arr, i32** %0, align 8
  %l.addr = alloca i32, align 4
  %1 = load i32*, i32** %0, align 8
  %r.addr = alloca i32, align 4
  store i32 %l, i32* %l.addr, align 4
  %retval = alloca i32, align 4
  store i32 %r, i32* %r.addr, align 4
  %2 = alloca i32, align 4
  %3 = load i32, i32* %l.addr, align 4
  %4 = getelementptr inbounds i32, i32* %1, i32 %3
  %5 = load i32, i32* %4, align 4
  store i32 %5, i32* %2, align 4
  %6 = load i32, i32* %l.addr, align 4
  %7 = getelementptr inbounds i32, i32* %1, i32 %6
  %8 = load i32, i32* %r.addr, align 4
  %9 = getelementptr inbounds i32, i32* %1, i32 %8
  %10 = load i32, i32* %9, align 4
  store i32 %10, i32* %7, align 4
  %11 = load i32, i32* %r.addr, align 4
  %12 = getelementptr inbounds i32, i32* %1, i32 %11
  %13 = load i32, i32* %2, align 4
  store i32 %13, i32* %12, align 4
  %14 = load i32, i32* %l.addr, align 4
  %15 = load i32, i32* %r.addr, align 4
  %16 = call i32 @meanless_calculation(i32 %14, i32 %15)
  store i32 %16, i32* %retval, align 4
  %17 = load i32, i32* %retval, align 4
  ret i32 %17
}

define i32 @median(i32* %arr, i32 %begin, i32 %end, i32 %pos) {
entry0:
  %0 = alloca i32*, align 8
  store i32* %arr, i32** %0, align 8
  %begin.addr = alloca i32, align 4
  %1 = load i32*, i32** %0, align 8
  %end.addr = alloca i32, align 4
  store i32 %begin, i32* %begin.addr, align 4
  %pos.addr = alloca i32, align 4
  store i32 %end, i32* %end.addr, align 4
  %retval = alloca i32, align 4
  store i32 %pos, i32* %pos.addr, align 4
  %2 = alloca i32, align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = load i32, i32* %begin.addr, align 4
  %7 = getelementptr inbounds i32, i32* %1, i32 %6
  %8 = load i32, i32* %7, align 4
  store i32 %8, i32* %2, align 4
  %9 = load i32, i32* %begin.addr, align 4
  store i32 %9, i32* %3, align 4
  %10 = load i32, i32* %end.addr, align 4
  %11 = add i32 %10, 1
  store i32 %11, i32* %4, align 4
  store i32 0, i32* %5, align 4
  br label %while.cond0

while.cond0:                                      ; preds = %if.else0, %entry0
  %12 = icmp eq i32 1, 1
  br i1 %12, label %loop.body0, label %while.end0

loop.body0:                                       ; preds = %while.cond0
  br label %while.cond1

while.end0:                                       ; preds = %if.then0, %while.cond0
  %13 = load i32, i32* %begin.addr, align 4
  %14 = getelementptr inbounds i32, i32* %1, i32 %13
  %15 = load i32, i32* %2, align 4
  store i32 %15, i32* %14, align 4
  %16 = load i32, i32* %begin.addr, align 4
  %17 = load i32, i32* %3, align 4
  %18 = call i32 @swap(i32* %1, i32 %16, i32 %17)
  %19 = load i32, i32* %3, align 4
  %20 = load i32, i32* %pos.addr, align 4
  %21 = icmp sgt i32 %19, %20
  br i1 %21, label %if.then1, label %if.else1

while.cond1:                                      ; preds = %if.else2, %loop.body0
  %22 = load i32, i32* %3, align 4
  %23 = load i32, i32* %4, align 4
  %24 = icmp slt i32 %22, %23
  br i1 %24, label %loop.body1, label %while.end1

loop.body1:                                       ; preds = %while.cond1
  %25 = load i32, i32* %4, align 4
  %26 = sub i32 %25, 1
  store i32 %26, i32* %4, align 4
  %27 = load i32, i32* %4, align 4
  %28 = getelementptr inbounds i32, i32* %1, i32 %27
  %29 = load i32, i32* %28, align 4
  %30 = load i32, i32* %2, align 4
  %31 = icmp slt i32 %29, %30
  br i1 %31, label %if.then2, label %if.else2

while.end1:                                       ; preds = %if.then2, %while.cond1
  br label %while.cond2

if.then2:                                         ; preds = %loop.body1
  br label %while.end1

if.else2:                                         ; preds = %loop.body1
  %32 = load i32, i32* %5, align 4
  %33 = add i32 %32, 1
  store i32 %33, i32* %5, align 4
  br label %while.cond1

while.cond2:                                      ; preds = %if.else3, %while.end1
  %34 = load i32, i32* %3, align 4
  %35 = load i32, i32* %4, align 4
  %36 = icmp slt i32 %34, %35
  br i1 %36, label %loop.body2, label %while.end2

loop.body2:                                       ; preds = %while.cond2
  %37 = load i32, i32* %3, align 4
  %38 = add i32 %37, 1
  store i32 %38, i32* %3, align 4
  %39 = load i32, i32* %3, align 4
  %40 = getelementptr inbounds i32, i32* %1, i32 %39
  %41 = load i32, i32* %40, align 4
  %42 = load i32, i32* %2, align 4
  %43 = icmp sge i32 %41, %42
  br i1 %43, label %if.then3, label %if.else3

while.end2:                                       ; preds = %if.then3, %while.cond2
  %44 = load i32, i32* %3, align 4
  %45 = load i32, i32* %4, align 4
  %46 = icmp eq i32 %44, %45
  br i1 %46, label %if.then0, label %if.else0

if.then3:                                         ; preds = %loop.body2
  br label %while.end2

if.else3:                                         ; preds = %loop.body2
  %47 = load i32, i32* %5, align 4
  %48 = sub i32 %47, 1
  store i32 %48, i32* %5, align 4
  br label %while.cond2

if.then0:                                         ; preds = %while.end2
  br label %while.end0

if.else0:                                         ; preds = %while.end2
  %49 = load i32, i32* %3, align 4
  %50 = load i32, i32* %4, align 4
  %51 = call i32 @swap(i32* %1, i32 %49, i32 %50)
  br label %while.cond0

if.then1:                                         ; preds = %while.end0
  %52 = load i32, i32* %begin.addr, align 4
  %53 = load i32, i32* %3, align 4
  %54 = load i32, i32* %pos.addr, align 4
  %55 = call i32 @median(i32* %1, i32 %52, i32 %53, i32 %54)
  store i32 %55, i32* %retval, align 4
  br label %func_exit1

if.else1:                                         ; preds = %while.end0
  %56 = load i32, i32* %3, align 4
  %57 = load i32, i32* %pos.addr, align 4
  %58 = icmp slt i32 %56, %57
  br i1 %58, label %if.then4, label %if.else4

if.then4:                                         ; preds = %if.else1
  %59 = load i32, i32* %3, align 4
  %60 = add i32 %59, 1
  %61 = load i32, i32* %end.addr, align 4
  %62 = load i32, i32* %pos.addr, align 4
  %63 = call i32 @median(i32* %1, i32 %60, i32 %61, i32 %62)
  store i32 %63, i32* %retval, align 4
  br label %func_exit1

if.else4:                                         ; preds = %if.else1
  %64 = load i32, i32* %5, align 4
  store i32 %64, i32* %retval, align 4
  br label %func_exit1

func_exit1:                                       ; preds = %if.else4, %if.then4, %if.then1
  %65 = load i32, i32* %retval, align 4
  ret i32 %65
}

define i32 @main() {
entry0:
  %retval = alloca i32, align 4
  %0 = getelementptr inbounds [10000000 x i32], [10000000 x i32]* @a, i32 0, i32 0
  %1 = call i32 @getarray(i32* %0)
  store i32 %1, i32* @n, align 4
  call void bitcast (void (i32)* @starttime to void ()*)()
  %2 = load i32, i32* @n, align 4
  %3 = sub i32 %2, 1
  %4 = load i32, i32* @n, align 4
  %5 = sdiv i32 %4, 2
  %6 = call i32 @median(i32* %0, i32 0, i32 %3, i32 %5)
  call void bitcast (void (i32)* @stoptime to void ()*)()
  %7 = load i32, i32* @n, align 4
  call void @putarray(i32 %7, i32* %0)
  %8 = load i32, i32* @n, align 4
  %9 = sdiv i32 %8, 2
  %10 = getelementptr inbounds i32, i32* %0, i32 %9
  %11 = load i32, i32* %10, align 4
  %12 = srem i32 %11, 256
  store i32 %12, i32* %retval, align 4
  %13 = load i32, i32* %retval, align 4
  ret i32 %13
}

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @getint() #0 {
  %1 = alloca i32, align 4
  %2 = call i32 (i8*, ...) @scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i64 0, i64 0), i32* %1)
  %3 = load i32, i32* %1, align 4
  ret i32 %3
}

declare i32 @scanf(i8*, ...) #1

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @getch() #0 {
  %1 = alloca i8, align 1
  %2 = call i32 (i8*, ...) @scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.1, i64 0, i64 0), i8* %1)
  %3 = load i8, i8* %1, align 1
  %4 = sext i8 %3 to i32
  ret i32 %4
}

; Function Attrs: noinline nounwind optnone ssp uwtable
define i32 @getarray(i32* %0) #0 {
  %2 = alloca i32*, align 8
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32* %0, i32** %2, align 8
  %5 = call i32 (i8*, ...) @scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i64 0, i64 0), i32* %3)
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
  %15 = call i32 (i8*, ...) @scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i64 0, i64 0), i32* %14)
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

; Function Attrs: noinline nounwind optnone ssp uwtable
define void @putint(i32 %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  %4 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str, i64 0, i64 0), i32 %3)
  ret void
}

declare i32 @printf(i8*, ...) #1

; Function Attrs: noinline nounwind optnone ssp uwtable
define void @putch(i32 %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  %4 = call i32 @putchar(i32 %3)
  ret void
}

declare i32 @putchar(i32) #1

; Function Attrs: noinline nounwind optnone ssp uwtable
define void @putarray(i32 %0, i32* %1) #0 {
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

; Function Attrs: noinline nounwind optnone ssp uwtable
define void @starttime(i32 %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  ret void
}

; Function Attrs: noinline nounwind optnone ssp uwtable
define void @stoptime(i32 %0) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  ret void
}

attributes #0 = { noinline nounwind optnone ssp uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "darwin-stkchk-strong-link" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "probe-stack"="___chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "darwin-stkchk-strong-link" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "probe-stack"="___chkstk_darwin" "stack-protector-buffer-size"="8" "target-cpu"="penryn" "target-features"="+cx16,+cx8,+fxsr,+mmx,+sahf,+sse,+sse2,+sse3,+sse4.1,+ssse3,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}
!llvm.module.flags = !{!1, !2, !3}

!0 = !{!"Apple clang version 11.0.3 (clang-1103.0.32.62)"}
!1 = !{i32 2, !"SDK Version", [3 x i32] [i32 10, i32 15, i32 4]}
!2 = !{i32 1, !"wchar_size", i32 4}
!3 = !{i32 7, !"PIC Level", i32 2}
