; ModuleID = 'pic16f87xa-hal/src/epiccc/pic16_irq_dispatch_serial_epiccc.c'
source_filename = "pic16f87xa-hal/src/epiccc/pic16_irq_dispatch_serial_epiccc.c"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

; Function Attrs: nounwind
define dso_local void @epic_dispatch_all_irqs() local_unnamed_addr #0 {
  %1 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  %2 = zext i8 %1 to i16
  %3 = and i16 %2, 4
  %4 = icmp eq i16 %3, 0
  br i1 %4, label %8, label %5

5:                                                ; preds = %0
  %6 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  %7 = and i8 %6, -5
  store volatile i8 %7, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  br label %8

8:                                                ; preds = %5, %0
  %9 = and i16 %2, 1
  %10 = icmp eq i16 %9, 0
  br i1 %10, label %14, label %11

11:                                               ; preds = %8
  %12 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  %13 = and i8 %12, -2
  store volatile i8 %13, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  br label %14

14:                                               ; preds = %11, %8
  %15 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  %16 = zext i8 %15 to i16
  %17 = and i16 %16, 1
  %18 = icmp eq i16 %17, 0
  br i1 %18, label %22, label %19

19:                                               ; preds = %14
  %20 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  %21 = and i8 %20, -2
  store volatile i8 %21, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  br label %22

22:                                               ; preds = %19, %14
  %23 = and i16 %16, 2
  %24 = icmp eq i16 %23, 0
  br i1 %24, label %28, label %25

25:                                               ; preds = %22
  %26 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  %27 = and i8 %26, -3
  store volatile i8 %27, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  br label %28

28:                                               ; preds = %25, %22
  %29 = and i16 %16, 4
  %30 = icmp eq i16 %29, 0
  br i1 %30, label %34, label %31

31:                                               ; preds = %28
  %32 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  %33 = and i8 %32, -5
  store volatile i8 %33, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  br label %34

34:                                               ; preds = %31, %28
  %35 = and i16 %16, 8
  %36 = icmp eq i16 %35, 0
  br i1 %36, label %40, label %37

37:                                               ; preds = %34
  %38 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  %39 = and i8 %38, -9
  store volatile i8 %39, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  br label %40

40:                                               ; preds = %37, %34
  %41 = and i16 %16, 32
  %42 = icmp eq i16 %41, 0
  br i1 %42, label %44, label %43

43:                                               ; preds = %40
  tail call void @USART_RX_IRQHandler() #2
  br label %44

44:                                               ; preds = %43, %40
  %45 = and i16 %16, 16
  %46 = icmp eq i16 %45, 0
  br i1 %46, label %55, label %47

47:                                               ; preds = %44
  %48 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !2
  %49 = and i8 %48, 16
  %50 = icmp eq i8 %49, 0
  br i1 %50, label %52, label %51

51:                                               ; preds = %47
  tail call void @USART_TX_IRQHandler() #2
  br label %55

52:                                               ; preds = %47
  %53 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  %54 = and i8 %53, -17
  store volatile i8 %54, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  br label %55

55:                                               ; preds = %51, %52, %44
  %56 = and i16 %16, 64
  %57 = icmp eq i16 %56, 0
  br i1 %57, label %61, label %58

58:                                               ; preds = %55
  %59 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  %60 = and i8 %59, -65
  store volatile i8 %60, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  br label %61

61:                                               ; preds = %58, %55
  %62 = icmp sgt i8 %15, -1
  br i1 %62, label %66, label %63

63:                                               ; preds = %61
  %64 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  %65 = and i8 %64, 127
  store volatile i8 %65, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !2
  br label %66

66:                                               ; preds = %63, %61
  %67 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !2
  %68 = zext i8 %67 to i16
  %69 = and i16 %68, 1
  %70 = icmp eq i16 %69, 0
  br i1 %70, label %74, label %71

71:                                               ; preds = %66
  %72 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !2
  %73 = and i8 %72, -2
  store volatile i8 %73, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !2
  br label %74

74:                                               ; preds = %71, %66
  %75 = and i16 %68, 8
  %76 = icmp eq i16 %75, 0
  br i1 %76, label %80, label %77

77:                                               ; preds = %74
  %78 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !2
  %79 = and i8 %78, -9
  store volatile i8 %79, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !2
  br label %80

80:                                               ; preds = %77, %74
  %81 = and i16 %68, 16
  %82 = icmp eq i16 %81, 0
  br i1 %82, label %90, label %83

83:                                               ; preds = %80
  %84 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !2
  %85 = and i8 %84, 16
  %86 = icmp eq i8 %85, 0
  br i1 %86, label %87, label %90

87:                                               ; preds = %83
  %88 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !2
  %89 = and i8 %88, -17
  store volatile i8 %89, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !2
  br label %90

90:                                               ; preds = %83, %87, %80
  %91 = and i16 %68, 64
  %92 = icmp eq i16 %91, 0
  br i1 %92, label %96, label %93

93:                                               ; preds = %90
  %94 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !2
  %95 = and i8 %94, -65
  store volatile i8 %95, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !2
  br label %96

96:                                               ; preds = %93, %90
  ret void
}

declare dso_local void @USART_RX_IRQHandler() local_unnamed_addr #1

declare dso_local void @USART_TX_IRQHandler() local_unnamed_addr #1

attributes #0 = { nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { nobuiltin nounwind "no-builtins" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{!"clang version 20.1.8"}
!2 = !{!3, !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C/C++ TBAA"}
