; ModuleID = 'while.c'
source_filename = "while.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

define dso_local i32 @main() #0 {
entry:
    %0 = alloca i32, align 4
    store i32 0,i32* %0,align 4 ;store 0
    %1 = alloca i32,align 4 ;int a
    %2 = alloca i32,align 4 ;int i
    store i32 10,i32* %1,align 4 ;a=10
    store i32 0,i32* %2,align 4 ;i=10
    br label %while.cond
while.cond:
    %3 = load i32,i32* %2,align 4
    %cmp = icmp slt i32 %3,10 ;cmp=(i<10)?true:false
    br i1 %cmp, label %while.body,label %while.end ;fcmp=ture,execute module while.body;else execute module while.end

while.body:
    %4 = load i32,i32* %2,align 4
    %5 = add i32 %4,1
    store i32 %5,i32* %2,align 4 ;i=i+1
    %6 = load i32,i32* %2,align 4 ;%6=i
    %7 = load i32,i32* %1,align 4 ;%7=a
    %8 = add i32 %6,%7 ;%8=a+i
    store i32 %8,i32* %1,align 4 ;store %8 in the address of a
    br label %while.cond

while.end:
    %9 = load i32,i32* %1,align 4
    ret i32 %9 ;return a

}


attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 10.0.0-4ubuntu1 "}

