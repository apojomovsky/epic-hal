; ModuleID = 'pic16f87xa-hal/src/peripherals/pic16f87xa_usart.c'
source_filename = "pic16f87xa-hal/src/peripherals/pic16f87xa_usart.c"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

@g_usart = internal unnamed_addr global ptr null, align 2

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local zeroext range(i16 -1, 256) i16 @USART_ComputeSPBRG(i32 noundef %0, i32 noundef %1, i16 noundef %2, i16 noundef %3) local_unnamed_addr #0 {
  %5 = icmp eq i32 %1, 0
  br i1 %5, label %18, label %6

6:                                                ; preds = %4
  %7 = icmp eq i16 %2, 0
  %8 = icmp eq i16 %3, 1
  %9 = select i1 %8, i16 4, i16 6
  %10 = select i1 %7, i16 %9, i16 2
  %11 = zext nneg i16 %10 to i32
  %12 = shl i32 %1, %11
  %13 = udiv i32 %0, %12
  %14 = add i32 %13, -1
  %15 = icmp ugt i32 %14, 255
  %16 = trunc nuw nsw i32 %14 to i16
  %17 = select i1 %15, i16 -1, i16 %16
  br label %18

18:                                               ; preds = %4, %6
  %19 = phi i16 [ %17, %6 ], [ -1, %4 ]
  ret i16 %19
}

; Function Attrs: nounwind
define dso_local range(i16 0, 5) i16 @EPIC_USART_Init(ptr noundef %0) local_unnamed_addr #1 {
  %2 = icmp eq ptr %0, null
  br i1 %2, label %49, label %3

3:                                                ; preds = %1
  store ptr %0, ptr @g_usart, align 2, !tbaa !2
  %4 = getelementptr inbounds nuw i8, ptr %0, i16 8
  %5 = load i8, ptr %4, align 2, !tbaa !6
  store volatile i8 %5, ptr inttoptr (i16 153 to ptr), align 1, !tbaa !9
  %6 = load i16, ptr %0, align 2, !tbaa !10
  %7 = icmp eq i16 %6, 1
  %8 = select i1 %7, i8 18, i8 2
  br i1 %7, label %9, label %15

9:                                                ; preds = %3
  %10 = getelementptr inbounds nuw i8, ptr %0, i16 2
  %11 = load i16, ptr %10, align 2, !tbaa !11
  %12 = icmp eq i16 %11, 1
  %13 = or disjoint i8 %8, -128
  %14 = select i1 %12, i8 %13, i8 %8
  br label %15

15:                                               ; preds = %9, %3
  %16 = phi i8 [ %8, %3 ], [ %14, %9 ]
  %17 = getelementptr inbounds nuw i8, ptr %0, i16 4
  %18 = load i16, ptr %17, align 2, !tbaa !12
  %19 = icmp eq i16 %18, 1
  %20 = or i8 %16, 4
  %21 = select i1 %19, i8 %20, i8 %16
  %22 = getelementptr inbounds nuw i8, ptr %0, i16 6
  %23 = load i16, ptr %22, align 2, !tbaa !13
  %24 = icmp eq i16 %23, 1
  %25 = or i8 %21, 64
  %26 = select i1 %24, i8 %25, i8 %21
  %27 = getelementptr inbounds nuw i8, ptr %0, i16 10
  %28 = load ptr, ptr %27, align 2, !tbaa !14
  %29 = icmp eq ptr %28, null
  %30 = or i8 %26, 32
  %31 = select i1 %29, i8 %26, i8 %30
  store volatile i8 %31, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  %32 = load i16, ptr %22, align 2, !tbaa !13
  %33 = icmp eq i16 %32, 1
  %34 = select i1 %33, i8 -64, i8 -128
  %35 = getelementptr inbounds nuw i8, ptr %0, i16 12
  %36 = load ptr, ptr %35, align 2, !tbaa !15
  %37 = icmp eq ptr %36, null
  %38 = or disjoint i8 %34, 16
  %39 = select i1 %37, i8 %34, i8 %38
  store volatile i8 %39, ptr inttoptr (i16 24 to ptr), align 8, !tbaa !9
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 10) #5
  %40 = load ptr, ptr %27, align 2, !tbaa !14
  %41 = icmp eq ptr %40, null
  br i1 %41, label %43, label %42

42:                                               ; preds = %15
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  br label %44

43:                                               ; preds = %15
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #5
  br label %44

44:                                               ; preds = %43, %42
  %45 = load ptr, ptr %35, align 2, !tbaa !15
  %46 = icmp eq ptr %45, null
  br i1 %46, label %48, label %47

47:                                               ; preds = %44
  tail call void @EPIC_IRQ_Enable(i16 noundef 10) #5
  br label %49

48:                                               ; preds = %44
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 10) #5
  br label %49

49:                                               ; preds = %47, %48, %1
  %50 = phi i16 [ 4, %1 ], [ 0, %48 ], [ 0, %47 ]
  ret i16 %50
}

declare dso_local void @EPIC_IRQ_ClearFlag(i16 noundef) local_unnamed_addr #2

declare dso_local void @EPIC_IRQ_Enable(i16 noundef) local_unnamed_addr #2

declare dso_local void @EPIC_IRQ_DisableSrc(i16 noundef) local_unnamed_addr #2

; Function Attrs: nounwind
define dso_local noundef i16 @EPIC_USART_DeInit() local_unnamed_addr #1 {
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #5
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 10) #5
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 9) #5
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 10) #5
  store volatile i8 0, ptr inttoptr (i16 24 to ptr), align 8, !tbaa !9
  store volatile i8 2, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  %1 = load volatile i8, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !9
  %2 = load volatile i8, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !9
  %3 = and i8 %2, -97
  %4 = or disjoint i8 %3, 32
  store volatile i8 %4, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !9
  store volatile i8 0, ptr inttoptr (i16 153 to ptr), align 1, !tbaa !9
  %5 = load volatile i8, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !9
  %6 = and i8 %5, -97
  %7 = and i8 %1, 96
  %8 = or disjoint i8 %6, %7
  store volatile i8 %8, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !9
  store ptr null, ptr @g_usart, align 2, !tbaa !2
  ret i16 0
}

; Function Attrs: nounwind
define dso_local void @EPIC_USART_Transmit(i8 noundef zeroext %0) local_unnamed_addr #1 {
  store volatile i8 %0, ptr inttoptr (i16 25 to ptr), align 1, !tbaa !9
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 9) #5
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_GetTX9D() local_unnamed_addr #3 {
  %1 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  %2 = and i8 %1, 1
  ret i8 %2
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_USART_SetTX9D(i8 noundef zeroext %0) local_unnamed_addr #4 {
  %2 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  %3 = icmp ne i8 %0, 0
  %4 = and i8 %2, -2
  %5 = zext i1 %3 to i8
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_IsTxShiftRegisterEmpty() local_unnamed_addr #3 {
  %1 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  %2 = lshr i8 %1, 1
  %3 = and i8 %2, 1
  ret i8 %3
}

; Function Attrs: nounwind
define dso_local zeroext i8 @EPIC_USART_Receive() local_unnamed_addr #1 {
  %1 = load volatile i8, ptr inttoptr (i16 26 to ptr), align 2, !tbaa !9
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 10) #5
  ret i8 %1
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_GetRX9D() local_unnamed_addr #3 {
  %1 = load volatile i8, ptr inttoptr (i16 24 to ptr), align 8, !tbaa !9
  %2 = and i8 %1, 1
  ret i8 %2
}

; Function Attrs: nounwind
define weak dso_local void @USART_TX_IRQHandler() local_unnamed_addr #1 {
  %1 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %2 = and i8 %1, 16
  %3 = icmp ne i8 %2, 0
  %4 = load ptr, ptr @g_usart, align 2
  %5 = icmp ne ptr %4, null
  %6 = select i1 %3, i1 %5, i1 false
  br i1 %6, label %7, label %12

7:                                                ; preds = %0
  %8 = getelementptr inbounds nuw i8, ptr %4, i16 10
  %9 = load ptr, ptr %8, align 2, !tbaa !14
  %10 = icmp eq ptr %9, null
  br i1 %10, label %12, label %11

11:                                               ; preds = %7
  tail call void %9() #5
  br label %12

12:                                               ; preds = %0, %11, %7
  ret void
}

; Function Attrs: nounwind
define weak dso_local void @USART_RX_IRQHandler() local_unnamed_addr #1 {
  %1 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %2 = and i8 %1, 32
  %3 = icmp eq i8 %2, 0
  br i1 %3, label %15, label %4

4:                                                ; preds = %0
  %5 = load volatile i8, ptr inttoptr (i16 26 to ptr), align 2, !tbaa !9
  %6 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %7 = and i8 %6, -33
  store volatile i8 %7, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %8 = load ptr, ptr @g_usart, align 2, !tbaa !2
  %9 = icmp eq ptr %8, null
  br i1 %9, label %15, label %10

10:                                               ; preds = %4
  %11 = getelementptr inbounds nuw i8, ptr %8, i16 12
  %12 = load ptr, ptr %11, align 2, !tbaa !15
  %13 = icmp eq ptr %12, null
  br i1 %13, label %15, label %14

14:                                               ; preds = %10
  tail call void %12(i8 noundef zeroext %5) #5
  br label %15

15:                                               ; preds = %4, %10, %14, %0
  ret void
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { mustprogress nofree norecurse nounwind willreturn "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #4 = { nofree norecurse nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #5 = { nobuiltin nounwind "no-builtins" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{!"clang version 20.1.8"}
!2 = !{!3, !3, i64 0}
!3 = !{!"any pointer", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!7, !4, i64 8}
!7 = !{!"", !8, i64 0, !8, i64 2, !8, i64 4, !8, i64 6, !4, i64 8, !3, i64 10, !3, i64 12}
!8 = !{!"int", !4, i64 0}
!9 = !{!4, !4, i64 0}
!10 = !{!7, !8, i64 0}
!11 = !{!7, !8, i64 2}
!12 = !{!7, !8, i64 4}
!13 = !{!7, !8, i64 6}
!14 = !{!7, !3, i64 10}
!15 = !{!7, !3, i64 12}
