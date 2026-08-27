; ModuleID = 'pic16f87xa-hal/src/core/pic16_irq.c'
source_filename = "pic16f87xa-hal/src/core/pic16_irq.c"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

%struct.irq_desc_t = type { i8, i8, i8, i8 }

@irq_table = internal unnamed_addr constant [15 x %struct.irq_desc_t] [%struct.irq_desc_t { i8 1, i8 8, i8 1, i8 0 }, %struct.irq_desc_t { i8 2, i8 16, i8 1, i8 0 }, %struct.irq_desc_t { i8 4, i8 32, i8 1, i8 0 }, %struct.irq_desc_t { i8 1, i8 1, i8 0, i8 0 }, %struct.irq_desc_t { i8 2, i8 2, i8 0, i8 0 }, %struct.irq_desc_t { i8 4, i8 4, i8 0, i8 0 }, %struct.irq_desc_t { i8 1, i8 1, i8 0, i8 1 }, %struct.irq_desc_t { i8 8, i8 8, i8 0, i8 0 }, %struct.irq_desc_t { i8 8, i8 8, i8 0, i8 1 }, %struct.irq_desc_t { i8 16, i8 16, i8 0, i8 0 }, %struct.irq_desc_t { i8 32, i8 32, i8 0, i8 0 }, %struct.irq_desc_t { i8 64, i8 64, i8 0, i8 0 }, %struct.irq_desc_t { i8 16, i8 16, i8 0, i8 1 }, %struct.irq_desc_t { i8 64, i8 64, i8 0, i8 1 }, %struct.irq_desc_t { i8 -128, i8 -128, i8 0, i8 0 }], align 1

; Function Attrs: nofree norecurse nounwind
define dso_local zeroext range(i8 0, 2) i8 @EPIC_IRQ_Disable() local_unnamed_addr #0 {
  %1 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  %2 = lshr i8 %1, 7
  %3 = and i8 %1, 127
  store volatile i8 %3, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  ret i8 %2
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_Restore(i8 noundef zeroext %0) local_unnamed_addr #0 {
  %2 = icmp eq i8 %0, 0
  %3 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  %4 = and i8 %3, 127
  %5 = select i1 %2, i8 0, i8 -128
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_Enable(i16 noundef %0) local_unnamed_addr #0 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %24, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !5
  %7 = getelementptr inbounds nuw i8, ptr %4, i16 1
  %8 = load i8, ptr %7, align 1, !tbaa !7
  %9 = icmp eq i8 %6, 0
  br i1 %9, label %10, label %20

10:                                               ; preds = %3
  %11 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %12 = load i8, ptr %11, align 1, !tbaa !8
  %13 = icmp eq i8 %12, 0
  br i1 %13, label %17, label %14

14:                                               ; preds = %10
  %15 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !2
  %16 = or i8 %15, %8
  store volatile i8 %16, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !2
  br label %20

17:                                               ; preds = %10
  %18 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !2
  %19 = or i8 %18, %8
  store volatile i8 %19, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !2
  br label %20

20:                                               ; preds = %14, %17, %3
  %21 = phi i8 [ %8, %3 ], [ 64, %17 ], [ 64, %14 ]
  %22 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  %23 = or i8 %22, %21
  store volatile i8 %23, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  br label %24

24:                                               ; preds = %20, %1
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_DisableSrc(i16 noundef %0) local_unnamed_addr #0 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %25, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !5
  %7 = getelementptr inbounds nuw i8, ptr %4, i16 1
  %8 = load i8, ptr %7, align 1, !tbaa !7
  %9 = icmp eq i8 %6, 0
  br i1 %9, label %14, label %10

10:                                               ; preds = %3
  %11 = xor i8 %8, -1
  %12 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  %13 = and i8 %12, %11
  store volatile i8 %13, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  br label %25

14:                                               ; preds = %3
  %15 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %16 = load i8, ptr %15, align 1, !tbaa !8
  %17 = icmp eq i8 %16, 0
  %18 = xor i8 %8, -1
  br i1 %17, label %22, label %19

19:                                               ; preds = %14
  %20 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !2
  %21 = and i8 %20, %18
  store volatile i8 %21, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !2
  br label %25

22:                                               ; preds = %14
  %23 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !2
  %24 = and i8 %23, %18
  store volatile i8 %24, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !2
  br label %25

25:                                               ; preds = %10, %22, %19, %1
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_ClearFlag(i16 noundef %0) local_unnamed_addr #0 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %21, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !5
  %7 = load i8, ptr %4, align 1, !tbaa !9
  %8 = icmp eq i8 %6, 0
  br i1 %8, label %13, label %9

9:                                                ; preds = %3
  %10 = xor i8 %7, -1
  %11 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  %12 = and i8 %11, %10
  store volatile i8 %12, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !2
  br label %21

13:                                               ; preds = %3
  %14 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %15 = load i8, ptr %14, align 1, !tbaa !8
  %16 = icmp eq i8 %15, 0
  %17 = select i1 %16, ptr inttoptr (i16 12 to ptr), ptr inttoptr (i16 13 to ptr)
  %18 = load volatile i8, ptr %17, align 1, !tbaa !2
  %19 = xor i8 %7, -1
  %20 = and i8 %18, %19
  store volatile i8 %20, ptr %17, align 1, !tbaa !2
  br label %21

21:                                               ; preds = %9, %13, %1
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_IRQ_GetFlag(i16 noundef %0) local_unnamed_addr #1 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %20, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !5
  %7 = load i8, ptr %4, align 1, !tbaa !9
  %8 = icmp eq i8 %6, 0
  br i1 %8, label %9, label %14

9:                                                ; preds = %3
  %10 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %11 = load i8, ptr %10, align 1, !tbaa !8
  %12 = icmp eq i8 %11, 0
  %13 = select i1 %12, ptr inttoptr (i16 12 to ptr), ptr inttoptr (i16 13 to ptr)
  br label %14

14:                                               ; preds = %3, %9
  %15 = phi ptr [ %13, %9 ], [ inttoptr (i16 11 to ptr), %3 ]
  %16 = load volatile i8, ptr %15, align 1, !tbaa !2
  %17 = and i8 %16, %7
  %18 = icmp ne i8 %17, 0
  %19 = zext i1 %18 to i8
  br label %20

20:                                               ; preds = %1, %14
  %21 = phi i8 [ %19, %14 ], [ 0, %1 ]
  ret i8 %21
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @EPIC_IRQ_SetPriority(i16 noundef %0, i16 noundef %1) local_unnamed_addr #2 {
  ret void
}

attributes #0 = { nofree norecurse nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { mustprogress nofree norecurse nounwind willreturn "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{!"clang version 20.1.8"}
!2 = !{!3, !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C/C++ TBAA"}
!5 = !{!6, !3, i64 2}
!6 = !{!"", !3, i64 0, !3, i64 1, !3, i64 2, !3, i64 3}
!7 = !{!6, !3, i64 1}
!8 = !{!6, !3, i64 3}
!9 = !{!6, !3, i64 0}
