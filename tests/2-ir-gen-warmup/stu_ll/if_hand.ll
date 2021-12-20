; ModuleID = 'if.c'
source_filename = "if.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define dso_local i32 @main() #0 {
entry:
    %0 = alloca i32, align 4
    store i32 0,i32* %0,align 4 ;store 0
    %1 = alloca float, align 4
    store float 0x40163851E0000000,float* %1, align 4 ;store 5.555
    %2 = load float, float* %1, align 4
    %X = sitofp i32 1 to float ; To use fcmp instrucyion,we need make 1(type i32) be type float
    %fcmp = fcmp ugt float %2, %X ;fcmp=(a>1)?true:false
    br i1 %fcmp,label %if.then,label %3 ;fcmp=ture,execute module if.then;else execute module 3
if.then:
    ret i32 233

3:
    ret i32 0

}




attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1 "}
