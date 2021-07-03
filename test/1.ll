; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

@x = global [100010 x i32] zeroinitializer
@y = global [3000000 x i32] zeroinitializer
@v = global [3000000 x i32] zeroinitializer
@a = global [100010 x i32] zeroinitializer
@b = global [100010 x i32] zeroinitializer
@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%c\00", align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"%d:\00", align 1
@.str.3 = private unnamed_addr constant [4 x i8] c" %d\00", align 1

define void @spmv(i32 %n, i32* %xptr, i32* %yidx, i32* %vals, i32* %b, i32* %x) {
entry0:
  %n.addr = alloca i32, align 4
  %0 = alloca i32*, align 8
  store i32 %n, i32* %n.addr, align 4
  store i32* %xptr, i32** %0, align 8
  %1 = alloca i32*, align 8
  %2 = load i32*, i32** %0, align 8
  store i32* %yidx, i32** %1, align 8
  %3 = alloca i32*, align 8
  %4 = load i32*, i32** %1, align 8
  store i32* %vals, i32** %3, align 8
  %5 = alloca i32*, align 8
  %6 = load i32*, i32** %3, align 8
  store i32* %b, i32** %5, align 8
  %7 = alloca i32*, align 8
  %8 = load i32*, i32** %5, align 8
  store i32* %x, i32** %7, align 8
  %9 = load i32*, i32** %7, align 8
  %10 = alloca i32, align 4
  %11 = alloca i32, align 4
  %12 = alloca i32, align 4
  store i32 0, i32* %10, align 4
  br label %while.cond0

while.cond0:                                      ; preds = %loop.body0, %entry0
  %13 = load i32, i32* %10, align 4
  %14 = load i32, i32* %n.addr, align 4
  %15 = icmp slt i32 %13, %14
  br i1 %15, label %loop.body0, label %while.end0

loop.body0:                                       ; preds = %while.cond0
  %16 = load i32, i32* %10, align 4
  %17 = getelementptr inbounds i32, i32* %9, i32 %16
  store i32 0, i32* %17, align 4
  %18 = load i32, i32* %10, align 4
  %19 = add i32 %18, 1
  store i32 %19, i32* %10, align 4
  br label %while.cond0

while.end0:                                       ; preds = %while.cond0
  store i32 0, i32* %10, align 4
  br label %while.cond1

while.cond1:                                      ; preds = %while.end1, %while.end0
  %20 = load i32, i32* %10, align 4
  %21 = load i32, i32* %n.addr, align 4
  %22 = icmp slt i32 %20, %21
  br i1 %22, label %loop.body1, label %while.end2

loop.body1:                                       ; preds = %while.cond1
  %23 = load i32, i32* %10, align 4
  %24 = getelementptr inbounds i32, i32* %2, i32 %23
  %25 = load i32, i32* %24, align 4
  store i32 %25, i32* %11, align 4
  br label %while.cond2

while.end2:                                       ; preds = %while.cond1
  ret void

while.cond2:                                      ; preds = %loop.body2, %loop.body1
  %26 = load i32, i32* %10, align 4
  %27 = add i32 %26, 1
  %28 = getelementptr inbounds i32, i32* %2, i32 %27
  %29 = load i32, i32* %11, align 4
  %30 = load i32, i32* %28, align 4
  %31 = icmp slt i32 %29, %30
  br i1 %31, label %loop.body2, label %while.end3

loop.body2:                                       ; preds = %while.cond2
  %32 = load i32, i32* %11, align 4
  %33 = getelementptr inbounds i32, i32* %4, i32 %32
  %34 = load i32, i32* %33, align 4
  %35 = getelementptr inbounds i32, i32* %9, i32 %34
  %36 = load i32, i32* %11, align 4
  %37 = getelementptr inbounds i32, i32* %4, i32 %36
  %38 = load i32, i32* %37, align 4
  %39 = getelementptr inbounds i32, i32* %9, i32 %38
  %40 = load i32, i32* %11, align 4
  %41 = getelementptr inbounds i32, i32* %6, i32 %40
  %42 = load i32, i32* %39, align 4
  %43 = load i32, i32* %41, align 4
  %44 = add i32 %42, %43
  store i32 %44, i32* %35, align 4
  %45 = load i32, i32* %11, align 4
  %46 = add i32 %45, 1
  store i32 %46, i32* %11, align 4
  br label %while.cond2

while.end3:                                       ; preds = %while.cond2
  %47 = load i32, i32* %10, align 4
  %48 = getelementptr inbounds i32, i32* %2, i32 %47
  %49 = load i32, i32* %48, align 4
  store i32 %49, i32* %11, align 4
  br label %while.cond3

while.cond3:                                      ; preds = %loop.body3, %while.end3
  %50 = load i32, i32* %10, align 4
  %51 = add i32 %50, 1
  %52 = getelementptr inbounds i32, i32* %2, i32 %51
  %53 = load i32, i32* %11, align 4
  %54 = load i32, i32* %52, align 4
  %55 = icmp slt i32 %53, %54
  br i1 %55, label %loop.body3, label %while.end1

loop.body3:                                       ; preds = %while.cond3
  %56 = load i32, i32* %11, align 4
  %57 = getelementptr inbounds i32, i32* %4, i32 %56
  %58 = load i32, i32* %57, align 4
  %59 = getelementptr inbounds i32, i32* %9, i32 %58
  %60 = load i32, i32* %11, align 4
  %61 = getelementptr inbounds i32, i32* %4, i32 %60
  %62 = load i32, i32* %61, align 4
  %63 = getelementptr inbounds i32, i32* %9, i32 %62
  %64 = load i32, i32* %11, align 4
  %65 = getelementptr inbounds i32, i32* %6, i32 %64
  %66 = load i32, i32* %10, align 4
  %67 = getelementptr inbounds i32, i32* %8, i32 %66
  %68 = load i32, i32* %67, align 4
  %69 = sub i32 %68, 1
  %70 = load i32, i32* %65, align 4
  %71 = mul i32 %70, %69
  %72 = load i32, i32* %63, align 4
  %73 = add i32 %72, %71
  store i32 %73, i32* %59, align 4
  %74 = load i32, i32* %11, align 4
  %75 = add i32 %74, 1
  store i32 %75, i32* %11, align 4
  br label %while.cond3

while.end1:                                       ; preds = %while.cond3
  %76 = load i32, i32* %10, align 4
  %77 = add i32 %76, 1
  store i32 %77, i32* %10, align 4
  br label %while.cond1
}

define i32 @main() {
entry0:
  %retval = alloca i32, align 4
  %0 = alloca i32, align 4
  %1 = getelementptr inbounds [100010 x i32], [100010 x i32]* @x, i32 0, i32 0
  %2 = alloca i32, align 4
  %3 = getelementptr inbounds [3000000 x i32], [3000000 x i32]* @y, i32 0, i32 0
  %4 = getelementptr inbounds [3000000 x i32], [3000000 x i32]* @v, i32 0, i32 0
  %5 = getelementptr inbounds [100010 x i32], [100010 x i32]* @a, i32 0, i32 0
  %6 = alloca i32, align 4
  %7 = getelementptr inbounds [100010 x i32], [100010 x i32]* @b, i32 0, i32 0
  %8 = call i32 @getarray(i32* %1)
  %9 = sub i32 %8, 1
  store i32 %9, i32* %0, align 4
  %10 = call i32 @getarray(i32* %3)
  store i32 %10, i32* %2, align 4
  %11 = call i32 @getarray(i32* %4)
  %12 = call i32 @getarray(i32* %5)
  call void bitcast (void (i32)* @starttime to void ()*)()
  store i32 0, i32* %6, align 4
  br label %while.cond0

while.cond0:                                      ; preds = %loop.body0, %entry0
  %13 = load i32, i32* %6, align 4
  %14 = icmp slt i32 %13, 100
  br i1 %14, label %loop.body0, label %while.end0

loop.body0:                                       ; preds = %while.cond0
  %15 = load i32, i32* %0, align 4
  call void @spmv(i32 %15, i32* %1, i32* %3, i32* %4, i32* %5, i32* %7)
  %16 = load i32, i32* %0, align 4
  call void @spmv(i32 %16, i32* %1, i32* %3, i32* %4, i32* %7, i32* %5)
  %17 = load i32, i32* %6, align 4
  %18 = add i32 %17, 1
  store i32 %18, i32* %6, align 4
  br label %while.cond0

while.end0:                                       ; preds = %while.cond0
  call void bitcast (void (i32)* @stoptime to void ()*)()
  %19 = load i32, i32* %0, align 4
  call void @putarray(i32 %19, i32* %7)
  store i32 0, i32* %retval, align 4
  %20 = load i32, i32* %retval, align 4
  ret i32 %20
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
