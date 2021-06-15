; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%c\00", align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"%d:\00", align 1
@.str.3 = private unnamed_addr constant [4 x i8] c" %d\00", align 1

define i32 @main() {
entry0:
  %retval = alloca i32, align 4
  %0 = alloca i32, align 4
  %1 = alloca i32, align 4
  %2 = alloca [10 x i32], align 4
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  %5 = alloca i32, align 4
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i1, align 1
  %9 = getelementptr inbounds [10 x i32], [10 x i32]* %2, i32 0, i32 0
  store i32 0, i32* %1, align 4
  store i32 0, i32* %0, align 4
  br label %while.cond0

while.cond0:                                      ; preds = %loop.body0, %entry0
  %10 = load i32, i32* %0, align 4
  %11 = icmp slt i32 %10, 10
  br i1 %11, label %loop.body0, label %while.end0

loop.body0:                                       ; preds = %while.cond0
  %12 = load i32, i32* %0, align 4
  %13 = getelementptr inbounds i32, i32* %9, i32 %12
  %14 = load i32, i32* %0, align 4
  %15 = add i32 %14, 1
  store i32 %15, i32* %13, align 4
  %16 = load i32, i32* %0, align 4
  %17 = add i32 %16, 1
  store i32 %17, i32* %0, align 4
  br label %while.cond0

while.end0:                                       ; preds = %while.cond0
  store i32 10, i32* %7, align 4
  %18 = call i32 @getint()
  store i32 %18, i32* %3, align 4
  %19 = load i32, i32* %7, align 4
  %20 = sub i32 %19, 1
  store i32 %20, i32* %4, align 4
  store i32 0, i32* %5, align 4
  %21 = load i32, i32* %4, align 4
  %22 = load i32, i32* %5, align 4
  %23 = add i32 %21, %22
  %24 = sdiv i32 %23, 2
  store i32 %24, i32* %6, align 4
  br label %while.cond1

while.cond1:                                      ; preds = %if.end0, %while.end0
  %25 = load i32, i32* %6, align 4
  %26 = getelementptr inbounds i32, i32* %9, i32 %25
  %27 = load i32, i32* %26, align 4
  %28 = load i32, i32* %3, align 4
  %29 = icmp ne i32 %27, %28
  br i1 %29, label %lhs.true0, label %lhs.false1

loop.body1:                                       ; preds = %land.end2
  %30 = load i32, i32* %4, align 4
  %31 = load i32, i32* %5, align 4
  %32 = add i32 %30, %31
  %33 = sdiv i32 %32, 2
  store i32 %33, i32* %6, align 4
  %34 = load i32, i32* %6, align 4
  %35 = getelementptr inbounds i32, i32* %9, i32 %34
  %36 = load i32, i32* %3, align 4
  %37 = load i32, i32* %35, align 4
  %38 = icmp slt i32 %36, %37
  br i1 %38, label %if.then0, label %if.else0

while.end1:                                       ; preds = %land.end2
  %39 = load i32, i32* %6, align 4
  %40 = getelementptr inbounds i32, i32* %9, i32 %39
  %41 = load i32, i32* %3, align 4
  %42 = load i32, i32* %40, align 4
  %43 = icmp eq i32 %41, %42
  br i1 %43, label %if.then1, label %if.else1

lhs.true0:                                        ; preds = %while.cond1
  %44 = load i32, i32* %5, align 4
  %45 = load i32, i32* %4, align 4
  %46 = icmp slt i32 %44, %45
  %47 = and i1 %29, %46
  store i1 %47, i1* %8, align 1
  br label %land.end2

lhs.false1:                                       ; preds = %while.cond1
  store i1 %29, i1* %8, align 1
  br label %land.end2

land.end2:                                        ; preds = %lhs.false1, %lhs.true0
  %48 = load i1, i1* %8, align 1
  %49 = icmp ne i1 false, %48
  br i1 %49, label %loop.body1, label %while.end1

if.then0:                                         ; preds = %loop.body1
  %50 = load i32, i32* %6, align 4
  %51 = sub i32 %50, 1
  store i32 %51, i32* %4, align 4
  br label %if.end0

if.else0:                                         ; preds = %loop.body1
  %52 = load i32, i32* %6, align 4
  %53 = add i32 %52, 1
  store i32 %53, i32* %5, align 4
  br label %if.end0

if.end0:                                          ; preds = %if.else0, %if.then0
  br label %while.cond1

if.then1:                                         ; preds = %while.end1
  %54 = load i32, i32* %3, align 4
  call void @putint(i32 %54)
  br label %if.end1

if.else1:                                         ; preds = %while.end1
  store i32 0, i32* %3, align 4
  %55 = load i32, i32* %3, align 4
  call void @putint(i32 %55)
  br label %if.end1

if.end1:                                          ; preds = %if.else1, %if.then1
  store i32 10, i32* %3, align 4
  %56 = load i32, i32* %3, align 4
  call void @putch(i32 %56)
  store i32 0, i32* %retval, align 4
  %57 = load i32, i32* %retval, align 4
  ret i32 %57
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
