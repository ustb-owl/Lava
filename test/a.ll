; ModuleID = 'a.c'
source_filename = "a.c"
target datalayout = "e-m:o-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-apple-darwin19.6.0"

; Function Attrs: noinline nounwind optnone
define i32 @main() #0 {
entry:
  %retval = alloca i32, align 4
  %a = alloca i32, align 4
  store i32 0, i32* %retval, align 4
  store i32 10, i32* %a, align 4
  %0 = load i32, i32* %a, align 4
  %tobool = icmp ne i32 %0, 0
  %lnot = xor i1 %tobool, true
  %lnot1 = xor i1 %lnot, true
  %lnot2 = xor i1 %lnot1, true
  %lnot.ext = zext i1 %lnot2 to i32
  %sub = sub nsw i32 0, %lnot.ext
  %tobool3 = icmp ne i32 %sub, 0
  br i1 %tobool3, label %if.then, label %if.else

if.then:                                          ; preds = %entry
  store i32 -1, i32* %a, align 4
  br label %if.end

if.else:                                          ; preds = %entry
  store i32 0, i32* %a, align 4
  br label %if.end

if.end:                                           ; preds = %if.else, %if.then
  %1 = load i32, i32* %a, align 4
  ret i32 %1
}

attributes #0 = { noinline nounwind optnone "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-features"="+cx8,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"Apple clang version 11.0.3 (clang-1103.0.32.62)"}
