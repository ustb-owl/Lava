; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-macosx10.15.0"

@A = global [1048576 x i32] zeroinitializer
@B = global [1048576 x i32] zeroinitializer
@C = global [1048576 x i32] zeroinitializer
@.str = private unnamed_addr constant [3 x i8] c"%d\00", align 1
@.str.1 = private unnamed_addr constant [3 x i8] c"%c\00", align 1
@.str.2 = private unnamed_addr constant [4 x i8] c"%d:\00", align 1
@.str.3 = private unnamed_addr constant [4 x i8] c" %d\00", align 1

define void @mm(i32 %n, i32* %A, i32* %B, i32* %C) {
entry0:
  %n.addr = alloca i32, align 4
  %0 = alloca i32*, align 8
  store i32 %n, i32* %n.addr, align 4
  store i32* %A, i32** %0, align 8
  %1 = alloca i32*, align 8
  %2 = load i32*, i32** %0, align 8
  store i32* %B, i32** %1, align 8
  %3 = alloca i32*, align 8
  %4 = load i32*, i32** %1, align 8
  store i32* %C, i32** %3, align 8
  %5 = load i32*, i32** %3, align 8
  %6 = alloca i32, align 4
  %7 = alloca i32, align 4
  %8 = alloca i32, align 4
  store i32 0, i32* %6, align 4
  store i32 0, i32* %7, align 4
  br label %while.cond0

while.cond0:                                      ; preds = %while.end0, %entry0
  %9 = load i32, i32* %6, align 4
  %10 = load i32, i32* %n.addr, align 4
  %11 = icmp slt i32 %9, %10
  br i1 %11, label %loop.body0, label %while.end1

loop.body0:                                       ; preds = %while.cond0
  store i32 0, i32* %7, align 4
  br label %while.cond1

while.end1:                                       ; preds = %while.cond0
  store i32 0, i32* %6, align 4
  store i32 0, i32* %7, align 4
  store i32 0, i32* %8, align 4
  br label %while.cond2

while.cond1:                                      ; preds = %loop.body1, %loop.body0
  %12 = load i32, i32* %7, align 4
  %13 = load i32, i32* %n.addr, align 4
  %14 = icmp slt i32 %12, %13
  br i1 %14, label %loop.body1, label %while.end0

loop.body1:                                       ; preds = %while.cond1
  %15 = load i32, i32* %6, align 4
  %16 = mul i32 1024, %15
  %17 = getelementptr inbounds i32, i32* %5, i32 %16
  %18 = load i32, i32* %7, align 4
  %19 = getelementptr inbounds i32, i32* %17, i32 %18
  store i32 0, i32* %19, align 4
  %20 = load i32, i32* %7, align 4
  %21 = add i32 %20, 1
  store i32 %21, i32* %7, align 4
  br label %while.cond1

while.end0:                                       ; preds = %while.cond1
  %22 = load i32, i32* %6, align 4
  %23 = add i32 %22, 1
  store i32 %23, i32* %6, align 4
  br label %while.cond0

while.cond2:                                      ; preds = %while.end2, %while.end1
  %24 = load i32, i32* %8, align 4
  %25 = load i32, i32* %n.addr, align 4
  %26 = icmp slt i32 %24, %25
  br i1 %26, label %loop.body2, label %while.end3

loop.body2:                                       ; preds = %while.cond2
  store i32 0, i32* %6, align 4
  br label %while.cond3

while.end3:                                       ; preds = %while.cond2
  ret void

while.cond3:                                      ; preds = %while.end4, %if.then0, %loop.body2
  %27 = load i32, i32* %6, align 4
  %28 = load i32, i32* %n.addr, align 4
  %29 = icmp slt i32 %27, %28
  br i1 %29, label %loop.body3, label %while.end2

loop.body3:                                       ; preds = %while.cond3
  %30 = load i32, i32* %6, align 4
  %31 = mul i32 1024, %30
  %32 = getelementptr inbounds i32, i32* %2, i32 %31
  %33 = load i32, i32* %8, align 4
  %34 = getelementptr inbounds i32, i32* %32, i32 %33
  %35 = load i32, i32* %34, align 4
  %36 = icmp eq i32 %35, 0
  br i1 %36, label %if.then0, label %if.else0

while.end2:                                       ; preds = %while.cond3
  %37 = load i32, i32* %8, align 4
  %38 = add i32 %37, 1
  store i32 %38, i32* %8, align 4
  br label %while.cond2

if.then0:                                         ; preds = %loop.body3
  %39 = load i32, i32* %6, align 4
  %40 = add i32 %39, 1
  store i32 %40, i32* %6, align 4
  br label %while.cond3

if.else0:                                         ; preds = %loop.body3
  store i32 0, i32* %7, align 4
  br label %while.cond4

while.cond4:                                      ; preds = %loop.body4, %if.else0
  %41 = load i32, i32* %7, align 4
  %42 = load i32, i32* %n.addr, align 4
  %43 = icmp slt i32 %41, %42
  br i1 %43, label %loop.body4, label %while.end4

loop.body4:                                       ; preds = %while.cond4
  %44 = load i32, i32* %6, align 4
  %45 = mul i32 1024, %44
  %46 = getelementptr inbounds i32, i32* %5, i32 %45
  %47 = load i32, i32* %7, align 4
  %48 = getelementptr inbounds i32, i32* %46, i32 %47
  %49 = load i32, i32* %6, align 4
  %50 = mul i32 1024, %49
  %51 = getelementptr inbounds i32, i32* %5, i32 %50
  %52 = load i32, i32* %7, align 4
  %53 = getelementptr inbounds i32, i32* %51, i32 %52
  %54 = load i32, i32* %6, align 4
  %55 = mul i32 1024, %54
  %56 = getelementptr inbounds i32, i32* %2, i32 %55
  %57 = load i32, i32* %8, align 4
  %58 = getelementptr inbounds i32, i32* %56, i32 %57
  %59 = load i32, i32* %8, align 4
  %60 = mul i32 1024, %59
  %61 = getelementptr inbounds i32, i32* %4, i32 %60
  %62 = load i32, i32* %7, align 4
  %63 = getelementptr inbounds i32, i32* %61, i32 %62
  %64 = load i32, i32* %58, align 4
  %65 = load i32, i32* %63, align 4
  %66 = mul i32 %64, %65
  %67 = load i32, i32* %53, align 4
  %68 = add i32 %67, %66
  store i32 %68, i32* %48, align 4
  %69 = load i32, i32* %7, align 4
  %70 = add i32 %69, 1
  store i32 %70, i32* %7, align 4
  br label %while.cond4

while.end4:                                       ; preds = %while.cond4
  %71 = load i32, i32* %6, align 4
  %72 = add i32 %71, 1
  store i32 %72, i32* %6, align 4
  br label %while.cond3
}

define i32 @main() {
entry0:
  %retval = alloca i32, align 4
  %0 = alloca i32, align 4
  %1 = alloca i32, align 4
  %2 = alloca i32, align 4
  %3 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @A, i32 0, i32 0
  %4 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @B, i32 0, i32 0
  %5 = getelementptr inbounds [1048576 x i32], [1048576 x i32]* @C, i32 0, i32 0
  %6 = alloca i32, align 4
  %7 = call i32 @getint()
  store i32 %7, i32* %0, align 4
  store i32 0, i32* %1, align 4
  store i32 0, i32* %2, align 4
  br label %while.cond0

while.cond0:                                      ; preds = %while.end0, %entry0
  %8 = load i32, i32* %1, align 4
  %9 = load i32, i32* %0, align 4
  %10 = icmp slt i32 %8, %9
  br i1 %10, label %loop.body0, label %while.end1

loop.body0:                                       ; preds = %while.cond0
  store i32 0, i32* %2, align 4
  br label %while.cond1

while.end1:                                       ; preds = %while.cond0
  store i32 0, i32* %1, align 4
  store i32 0, i32* %2, align 4
  br label %while.cond2

while.cond1:                                      ; preds = %loop.body1, %loop.body0
  %11 = load i32, i32* %2, align 4
  %12 = load i32, i32* %0, align 4
  %13 = icmp slt i32 %11, %12
  br i1 %13, label %loop.body1, label %while.end0

loop.body1:                                       ; preds = %while.cond1
  %14 = load i32, i32* %1, align 4
  %15 = mul i32 1024, %14
  %16 = getelementptr inbounds i32, i32* %3, i32 %15
  %17 = load i32, i32* %2, align 4
  %18 = getelementptr inbounds i32, i32* %16, i32 %17
  %19 = call i32 @getint()
  store i32 %19, i32* %18, align 4
  %20 = load i32, i32* %2, align 4
  %21 = add i32 %20, 1
  store i32 %21, i32* %2, align 4
  br label %while.cond1

while.end0:                                       ; preds = %while.cond1
  %22 = load i32, i32* %1, align 4
  %23 = add i32 %22, 1
  store i32 %23, i32* %1, align 4
  br label %while.cond0

while.cond2:                                      ; preds = %while.end2, %while.end1
  %24 = load i32, i32* %1, align 4
  %25 = load i32, i32* %0, align 4
  %26 = icmp slt i32 %24, %25
  br i1 %26, label %loop.body2, label %while.end3

loop.body2:                                       ; preds = %while.cond2
  store i32 0, i32* %2, align 4
  br label %while.cond3

while.end3:                                       ; preds = %while.cond2
  call void bitcast (void (i32)* @starttime to void ()*)()
  store i32 0, i32* %1, align 4
  br label %while.cond4

while.cond3:                                      ; preds = %loop.body3, %loop.body2
  %27 = load i32, i32* %2, align 4
  %28 = load i32, i32* %0, align 4
  %29 = icmp slt i32 %27, %28
  br i1 %29, label %loop.body3, label %while.end2

loop.body3:                                       ; preds = %while.cond3
  %30 = load i32, i32* %1, align 4
  %31 = mul i32 1024, %30
  %32 = getelementptr inbounds i32, i32* %4, i32 %31
  %33 = load i32, i32* %2, align 4
  %34 = getelementptr inbounds i32, i32* %32, i32 %33
  %35 = call i32 @getint()
  store i32 %35, i32* %34, align 4
  %36 = load i32, i32* %2, align 4
  %37 = add i32 %36, 1
  store i32 %37, i32* %2, align 4
  br label %while.cond3

while.end2:                                       ; preds = %while.cond3
  %38 = load i32, i32* %1, align 4
  %39 = add i32 %38, 1
  store i32 %39, i32* %1, align 4
  br label %while.cond2

while.cond4:                                      ; preds = %loop.body4, %while.end3
  %40 = load i32, i32* %1, align 4
  %41 = icmp slt i32 %40, 5
  br i1 %41, label %loop.body4, label %while.end4

loop.body4:                                       ; preds = %while.cond4
  %42 = load i32, i32* %0, align 4
  call void @mm(i32 %42, i32* %3, i32* %4, i32* %5)
  %43 = load i32, i32* %0, align 4
  call void @mm(i32 %43, i32* %3, i32* %5, i32* %4)
  %44 = load i32, i32* %1, align 4
  %45 = add i32 %44, 1
  store i32 %45, i32* %1, align 4
  br label %while.cond4

while.end4:                                       ; preds = %while.cond4
  store i32 0, i32* %6, align 4
  store i32 0, i32* %1, align 4
  br label %while.cond5

while.cond5:                                      ; preds = %while.end5, %while.end4
  %46 = load i32, i32* %1, align 4
  %47 = load i32, i32* %0, align 4
  %48 = icmp slt i32 %46, %47
  br i1 %48, label %loop.body5, label %while.end6

loop.body5:                                       ; preds = %while.cond5
  store i32 0, i32* %2, align 4
  br label %while.cond6

while.end6:                                       ; preds = %while.cond5
  call void bitcast (void (i32)* @stoptime to void ()*)()
  %49 = load i32, i32* %6, align 4
  call void @putint(i32 %49)
  call void @putch(i32 10)
  store i32 0, i32* %retval, align 4
  %50 = load i32, i32* %retval, align 4
  ret i32 %50

while.cond6:                                      ; preds = %loop.body6, %loop.body5
  %51 = load i32, i32* %2, align 4
  %52 = load i32, i32* %0, align 4
  %53 = icmp slt i32 %51, %52
  br i1 %53, label %loop.body6, label %while.end5

loop.body6:                                       ; preds = %while.cond6
  %54 = load i32, i32* %1, align 4
  %55 = mul i32 1024, %54
  %56 = getelementptr inbounds i32, i32* %4, i32 %55
  %57 = load i32, i32* %2, align 4
  %58 = getelementptr inbounds i32, i32* %56, i32 %57
  %59 = load i32, i32* %6, align 4
  %60 = load i32, i32* %58, align 4
  %61 = add i32 %59, %60
  store i32 %61, i32* %6, align 4
  %62 = load i32, i32* %2, align 4
  %63 = add i32 %62, 1
  store i32 %63, i32* %2, align 4
  br label %while.cond6

while.end5:                                       ; preds = %while.cond6
  %64 = load i32, i32* %1, align 4
  %65 = add i32 %64, 1
  store i32 %65, i32* %1, align 4
  br label %while.cond5
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
