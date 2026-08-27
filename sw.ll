; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

%struct.irq_desc_t = type { i8, i8, i8, i8 }
%struct.TIMER1_HandleTypeDef = type { i16, i16, i16, i16, i16, ptr }
%struct.CCP_HandleTypeDef = type { i16, i16, i16, %struct.CCP_PWMConfigTypeDef, ptr }
%struct.CCP_PWMConfigTypeDef = type { i16, i16 }

@g_ccp_callbacks = internal unnamed_addr global [3 x ptr] zeroinitializer, align 2
@addrs = internal unnamed_addr constant [2 x { i8, i8, i8, i8, i16 }] [{ i8, i8, i8, i8, i16 } { i8 21, i8 22, i8 23, i8 0, i16 5 }, { i8, i8, i8, i8, i16 } { i8 27, i8 28, i8 29, i8 0, i16 6 }], align 2
@ps_ratio = internal unnamed_addr constant [4 x i16] [i16 1, i16 2, i16 4, i16 8], align 2
@g_t1_handle = internal unnamed_addr global ptr null, align 2
@s_rb_change_callback = internal unnamed_addr global ptr null, align 2
@irq_table = internal unnamed_addr constant [15 x %struct.irq_desc_t] [%struct.irq_desc_t { i8 1, i8 8, i8 1, i8 0 }, %struct.irq_desc_t { i8 2, i8 16, i8 1, i8 0 }, %struct.irq_desc_t { i8 4, i8 32, i8 1, i8 0 }, %struct.irq_desc_t { i8 1, i8 1, i8 0, i8 0 }, %struct.irq_desc_t { i8 2, i8 2, i8 0, i8 0 }, %struct.irq_desc_t { i8 4, i8 4, i8 0, i8 0 }, %struct.irq_desc_t { i8 1, i8 1, i8 0, i8 1 }, %struct.irq_desc_t { i8 8, i8 8, i8 0, i8 0 }, %struct.irq_desc_t { i8 8, i8 8, i8 0, i8 1 }, %struct.irq_desc_t { i8 16, i8 16, i8 0, i8 0 }, %struct.irq_desc_t { i8 32, i8 32, i8 0, i8 0 }, %struct.irq_desc_t { i8 64, i8 64, i8 0, i8 0 }, %struct.irq_desc_t { i8 16, i8 16, i8 0, i8 1 }, %struct.irq_desc_t { i8 64, i8 64, i8 0, i8 1 }, %struct.irq_desc_t { i8 -128, i8 -128, i8 0, i8 0 }], align 1
@llvm.compiler.used = appending global [1 x ptr] [ptr @PIC16_IRQ_Handler], section "llvm.metadata"
@g_chan_a = internal unnamed_addr global ptr null, align 2
@g_cycles_per_bit = internal unnamed_addr global i16 0, align 2
@s_timer1 = internal global %struct.TIMER1_HandleTypeDef zeroinitializer, align 2

; Function Attrs: nounwind
define dso_local range(i16 0, 5) i16 @EPIC_CCP_Init(ptr noundef readonly %0) local_unnamed_addr #0 {
  %2 = icmp eq ptr %0, null
  br i1 %2, label %57, label %3

3:                                                ; preds = %1
  %4 = load i16, ptr %0, align 2, !tbaa !2
  %5 = add i16 %4, -1
  %6 = icmp ult i16 %5, 2
  br i1 %6, label %7, label %57

7:                                                ; preds = %3
  %8 = icmp eq i16 %4, 2
  %9 = select i1 %8, ptr getelementptr inbounds nuw (i8, ptr @addrs, i16 6), ptr @addrs
  %10 = getelementptr inbounds nuw i8, ptr %0, i16 10
  %11 = load ptr, ptr %10, align 2, !tbaa !10
  %12 = getelementptr inbounds nuw [3 x ptr], ptr @g_ccp_callbacks, i16 0, i16 %4
  store ptr %11, ptr %12, align 2, !tbaa !11
  %13 = select i1 %8, i16 6, i16 5
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef %13) #9
  %14 = load i16, ptr %0, align 2, !tbaa !2
  %15 = getelementptr inbounds nuw [3 x ptr], ptr @g_ccp_callbacks, i16 0, i16 %14
  %16 = load ptr, ptr %15, align 2, !tbaa !11
  %17 = icmp eq ptr %16, null
  br i1 %17, label %19, label %18

18:                                               ; preds = %7
  tail call void @EPIC_IRQ_Enable(i16 noundef %13) #9
  br label %20

19:                                               ; preds = %7
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef %13) #9
  br label %20

20:                                               ; preds = %19, %18
  %21 = getelementptr inbounds nuw i8, ptr %0, i16 2
  %22 = load i16, ptr %21, align 2, !tbaa !12
  %23 = icmp eq i16 %22, 12
  br i1 %23, label %24, label %38

24:                                               ; preds = %20
  %25 = getelementptr inbounds nuw i8, ptr %0, i16 8
  %26 = load i16, ptr %25, align 2, !tbaa !13
  %27 = trunc i16 %26 to i8
  %28 = shl i8 %27, 4
  %29 = and i8 %28, 48
  %30 = or disjoint i8 %29, 12
  %31 = lshr i16 %26, 2
  %32 = trunc i16 %31 to i8
  %33 = load i8, ptr %9, align 2, !tbaa !14
  %34 = zext i8 %33 to i16
  %35 = inttoptr i16 %34 to ptr
  store volatile i8 %32, ptr %35, align 1, !tbaa !16
  %36 = select i1 %8, i16 28, i16 22
  %37 = inttoptr i16 %36 to ptr
  store volatile i8 0, ptr %37, align 2, !tbaa !16
  br label %53

38:                                               ; preds = %20
  %39 = getelementptr inbounds nuw i8, ptr %0, i16 4
  %40 = load i16, ptr %39, align 2, !tbaa !17
  %41 = trunc i16 %40 to i8
  %42 = load i8, ptr %9, align 2, !tbaa !14
  %43 = zext i8 %42 to i16
  %44 = inttoptr i16 %43 to ptr
  store volatile i8 %41, ptr %44, align 1, !tbaa !16
  %45 = load i16, ptr %39, align 2, !tbaa !17
  %46 = lshr i16 %45, 8
  %47 = trunc nuw i16 %46 to i8
  %48 = select i1 %8, i16 28, i16 22
  %49 = inttoptr i16 %48 to ptr
  store volatile i8 %47, ptr %49, align 2, !tbaa !16
  %50 = load i16, ptr %21, align 2, !tbaa !12
  %51 = trunc i16 %50 to i8
  %52 = and i8 %51, 15
  br label %53

53:                                               ; preds = %38, %24
  %54 = phi i8 [ %30, %24 ], [ %52, %38 ]
  %55 = select i1 %8, i16 29, i16 23
  %56 = inttoptr i16 %55 to ptr
  store volatile i8 %54, ptr %56, align 1, !tbaa !16
  br label %57

57:                                               ; preds = %53, %3, %1
  %58 = phi i16 [ 4, %1 ], [ 4, %3 ], [ 0, %53 ]
  ret i16 %58
}

; Function Attrs: nounwind
define dso_local range(i16 0, 5) i16 @EPIC_CCP_DeInit(i16 noundef %0) local_unnamed_addr #0 {
  %2 = add i16 %0, -3
  %3 = icmp ult i16 %2, -2
  br i1 %3, label %10, label %4

4:                                                ; preds = %1
  %5 = icmp eq i16 %0, 2
  %6 = select i1 %5, i16 6, i16 5
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef %6) #9
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef %6) #9
  %7 = select i1 %5, i16 29, i16 23
  %8 = inttoptr i16 %7 to ptr
  store volatile i8 0, ptr %8, align 1, !tbaa !16
  %9 = getelementptr inbounds nuw [3 x ptr], ptr @g_ccp_callbacks, i16 0, i16 %0
  store ptr null, ptr %9, align 2, !tbaa !11
  br label %10

10:                                               ; preds = %4, %1
  %11 = phi i16 [ 0, %4 ], [ 4, %1 ]
  ret i16 %11
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_CCP_SetCompare(i16 noundef %0, i16 noundef zeroext %1) local_unnamed_addr #1 {
  %3 = add i16 %0, -3
  %4 = icmp ult i16 %3, -2
  br i1 %4, label %14, label %5

5:                                                ; preds = %2
  %6 = icmp eq i16 %0, 2
  %7 = lshr i16 %1, 8
  %8 = trunc nuw i16 %7 to i8
  %9 = select i1 %6, i16 28, i16 22
  %10 = inttoptr i16 %9 to ptr
  store volatile i8 %8, ptr %10, align 2, !tbaa !16
  %11 = trunc i16 %1 to i8
  %12 = select i1 %6, i16 27, i16 21
  %13 = inttoptr i16 %12 to ptr
  store volatile i8 %11, ptr %13, align 1, !tbaa !16
  br label %14

14:                                               ; preds = %5, %2
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_CCP_SetMode(i16 noundef %0, i16 noundef %1) local_unnamed_addr #1 {
  %3 = add i16 %0, -3
  %4 = icmp ult i16 %3, -2
  br i1 %4, label %11, label %5

5:                                                ; preds = %2
  %6 = icmp eq i16 %0, 2
  %7 = trunc i16 %1 to i8
  %8 = and i8 %7, 15
  %9 = select i1 %6, i16 29, i16 23
  %10 = inttoptr i16 %9 to ptr
  store volatile i8 %8, ptr %10, align 1, !tbaa !16
  br label %11

11:                                               ; preds = %5, %2
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local zeroext i16 @EPIC_CCP_GetCapture(i16 noundef %0) local_unnamed_addr #1 {
  %2 = add i16 %0, -3
  %3 = icmp ult i16 %2, -2
  br i1 %3, label %20, label %4

4:                                                ; preds = %1
  %5 = icmp eq i16 %0, 2
  %6 = select i1 %5, i16 28, i16 22
  %7 = inttoptr i16 %6 to ptr
  %8 = select i1 %5, i16 27, i16 21
  %9 = inttoptr i16 %8 to ptr
  br label %10

10:                                               ; preds = %10, %4
  %11 = load volatile i8, ptr %7, align 2, !tbaa !16
  %12 = load volatile i8, ptr %9, align 1, !tbaa !16
  %13 = load volatile i8, ptr %7, align 2, !tbaa !16
  %14 = icmp eq i8 %11, %13
  br i1 %14, label %15, label %10, !llvm.loop !18

15:                                               ; preds = %10
  %16 = zext i8 %13 to i16
  %17 = shl nuw i16 %16, 8
  %18 = zext i8 %12 to i16
  %19 = or disjoint i16 %17, %18
  br label %20

20:                                               ; preds = %15, %1
  %21 = phi i16 [ %19, %15 ], [ 0, %1 ]
  ret i16 %21
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_CCP_SetPWMDuty(i16 noundef %0, i16 noundef zeroext %1) local_unnamed_addr #1 {
  %3 = add i16 %0, -3
  %4 = icmp ult i16 %3, -2
  br i1 %4, label %19, label %5

5:                                                ; preds = %2
  %6 = icmp eq i16 %0, 2
  %7 = select i1 %6, i16 29, i16 23
  %8 = inttoptr i16 %7 to ptr
  %9 = load volatile i8, ptr %8, align 1, !tbaa !16
  %10 = and i8 %9, -49
  %11 = trunc i16 %1 to i8
  %12 = shl i8 %11, 4
  %13 = and i8 %12, 48
  %14 = or disjoint i8 %10, %13
  store volatile i8 %14, ptr %8, align 1, !tbaa !16
  %15 = lshr i16 %1, 2
  %16 = trunc i16 %15 to i8
  %17 = select i1 %6, i16 27, i16 21
  %18 = inttoptr i16 %17 to ptr
  store volatile i8 %16, ptr %18, align 1, !tbaa !16
  br label %19

19:                                               ; preds = %5, %2
  ret void
}

; Function Attrs: nounwind
define weak dso_local void @CCP1_IRQHandler() local_unnamed_addr #0 {
  %1 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %2 = and i8 %1, -5
  store volatile i8 %2, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %3 = load ptr, ptr getelementptr inbounds nuw (i8, ptr @g_ccp_callbacks, i16 2), align 2, !tbaa !11
  %4 = icmp eq ptr %3, null
  br i1 %4, label %6, label %5

5:                                                ; preds = %0
  tail call void %3() #9
  br label %6

6:                                                ; preds = %5, %0
  ret void
}

; Function Attrs: nounwind
define weak dso_local void @CCP2_IRQHandler() local_unnamed_addr #0 {
  %1 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  %2 = and i8 %1, -2
  store volatile i8 %2, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  %3 = load ptr, ptr getelementptr inbounds nuw (i8, ptr @g_ccp_callbacks, i16 4), align 2, !tbaa !11
  %4 = icmp eq ptr %3, null
  br i1 %4, label %6, label %5

5:                                                ; preds = %0
  tail call void %3() #9
  br label %6

6:                                                ; preds = %5, %0
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local zeroext i16 @EPIC_TIMER1_ReadCounter() local_unnamed_addr #1 {
  br label %1

1:                                                ; preds = %1, %0
  %2 = load volatile i8, ptr inttoptr (i16 15 to ptr), align 1, !tbaa !16
  %3 = load volatile i8, ptr inttoptr (i16 14 to ptr), align 2, !tbaa !16
  %4 = load volatile i8, ptr inttoptr (i16 15 to ptr), align 1, !tbaa !16
  %5 = icmp eq i8 %2, %4
  br i1 %5, label %6, label %1, !llvm.loop !21

6:                                                ; preds = %1
  %7 = zext i8 %4 to i16
  %8 = shl nuw i16 %7, 8
  %9 = zext i8 %3 to i16
  %10 = or disjoint i16 %8, %9
  ret i16 %10
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_TIMER1_WriteCounter(i16 noundef zeroext %0) local_unnamed_addr #1 {
  %2 = lshr i16 %0, 8
  %3 = trunc nuw i16 %2 to i8
  store volatile i8 %3, ptr inttoptr (i16 15 to ptr), align 1, !tbaa !16
  %4 = trunc i16 %0 to i8
  store volatile i8 %4, ptr inttoptr (i16 14 to ptr), align 2, !tbaa !16
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local zeroext i16 @EPIC_TIMER1_PrescalerToRatio(i16 noundef %0) local_unnamed_addr #2 {
  %2 = icmp ugt i16 %0, 3
  br i1 %2, label %6, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [4 x i16], ptr @ps_ratio, i16 0, i16 %0
  %5 = load i16, ptr %4, align 2, !tbaa !22
  br label %6

6:                                                ; preds = %3, %1
  %7 = phi i16 [ %5, %3 ], [ 1, %1 ]
  ret i16 %7
}

; Function Attrs: nounwind
define dso_local range(i16 0, 5) i16 @EPIC_TIMER1_Init(ptr noundef %0) local_unnamed_addr #0 {
  %2 = icmp eq ptr %0, null
  br i1 %2, label %12, label %3

3:                                                ; preds = %1
  %4 = load volatile i8, ptr inttoptr (i16 16 to ptr), align 16, !tbaa !16
  %5 = and i8 %4, -2
  store volatile i8 %5, ptr inttoptr (i16 16 to ptr), align 16, !tbaa !16
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 3) #9
  %6 = getelementptr inbounds nuw i8, ptr %0, i16 10
  %7 = load ptr, ptr %6, align 2, !tbaa !23
  %8 = icmp eq ptr %7, null
  br i1 %8, label %10, label %9

9:                                                ; preds = %3
  tail call void @EPIC_IRQ_Enable(i16 noundef 3) #9
  br label %11

10:                                               ; preds = %3
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 3) #9
  br label %11

11:                                               ; preds = %10, %9
  store ptr %0, ptr @g_t1_handle, align 2, !tbaa !11
  br label %12

12:                                               ; preds = %11, %1
  %13 = phi i16 [ 0, %11 ], [ 4, %1 ]
  ret i16 %13
}

; Function Attrs: nounwind
define dso_local noundef i16 @EPIC_TIMER1_DeInit() local_unnamed_addr #0 {
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 3) #9
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 3) #9
  store volatile i8 0, ptr inttoptr (i16 16 to ptr), align 16, !tbaa !16
  store volatile i8 0, ptr inttoptr (i16 15 to ptr), align 1, !tbaa !16
  store volatile i8 0, ptr inttoptr (i16 14 to ptr), align 2, !tbaa !16
  store ptr null, ptr @g_t1_handle, align 2, !tbaa !11
  ret i16 0
}

; Function Attrs: nofree norecurse nounwind
define dso_local range(i16 0, 5) i16 @EPIC_TIMER1_Start(ptr noundef readonly %0) local_unnamed_addr #1 {
  %2 = icmp eq ptr %0, null
  br i1 %2, label %29, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw i8, ptr %0, i16 8
  %5 = load i16, ptr %4, align 2, !tbaa !25
  %6 = lshr i16 %5, 8
  %7 = trunc nuw i16 %6 to i8
  store volatile i8 %7, ptr inttoptr (i16 15 to ptr), align 1, !tbaa !16
  %8 = trunc i16 %5 to i8
  store volatile i8 %8, ptr inttoptr (i16 14 to ptr), align 2, !tbaa !16
  %9 = getelementptr inbounds nuw i8, ptr %0, i16 6
  %10 = load i16, ptr %9, align 2, !tbaa !26
  %11 = trunc i16 %10 to i8
  %12 = shl i8 %11, 4
  %13 = and i8 %12, 48
  %14 = getelementptr inbounds nuw i8, ptr %0, i16 4
  %15 = load i16, ptr %14, align 2, !tbaa !27
  %16 = icmp eq i16 %15, 1
  %17 = or disjoint i8 %13, 8
  %18 = select i1 %16, i8 %17, i8 %13
  %19 = getelementptr inbounds nuw i8, ptr %0, i16 2
  %20 = load i16, ptr %19, align 2, !tbaa !28
  %21 = icmp eq i16 %20, 1
  %22 = or disjoint i8 %18, 4
  %23 = select i1 %21, i8 %22, i8 %18
  %24 = load i16, ptr %0, align 2, !tbaa !29
  %25 = icmp eq i16 %24, 1
  %26 = or disjoint i8 %23, 2
  %27 = select i1 %25, i8 %26, i8 %23
  %28 = or i8 %27, 1
  store volatile i8 %28, ptr inttoptr (i16 16 to ptr), align 16, !tbaa !16
  br label %29

29:                                               ; preds = %3, %1
  %30 = phi i16 [ 0, %3 ], [ 4, %1 ]
  ret i16 %30
}

; Function Attrs: nofree norecurse nounwind
define dso_local noundef i16 @EPIC_TIMER1_Stop() local_unnamed_addr #1 {
  %1 = load volatile i8, ptr inttoptr (i16 16 to ptr), align 16, !tbaa !16
  %2 = and i8 %1, -2
  store volatile i8 %2, ptr inttoptr (i16 16 to ptr), align 16, !tbaa !16
  ret i16 0
}

; Function Attrs: nounwind
define weak dso_local void @TIMER1_IRQHandler() local_unnamed_addr #0 {
  %1 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %2 = and i8 %1, 1
  %3 = icmp eq i8 %2, 0
  br i1 %3, label %14, label %4

4:                                                ; preds = %0
  %5 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %6 = and i8 %5, -2
  store volatile i8 %6, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %7 = load ptr, ptr @g_t1_handle, align 2, !tbaa !11
  %8 = icmp eq ptr %7, null
  br i1 %8, label %14, label %9

9:                                                ; preds = %4
  %10 = getelementptr inbounds nuw i8, ptr %7, i16 10
  %11 = load ptr, ptr %10, align 2, !tbaa !23
  %12 = icmp eq ptr %11, null
  br i1 %12, label %14, label %13

13:                                               ; preds = %9
  tail call void %11() #9
  br label %14

14:                                               ; preds = %13, %9, %4, %0
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_GPIO_Init(i16 noundef %0, i16 noundef zeroext %1, i16 noundef %2) local_unnamed_addr #1 {
  %4 = add i16 %0, -1
  %5 = icmp ult i16 %4, 4
  %6 = add i16 %0, 133
  %7 = select i1 %5, i16 %6, i16 133
  %8 = icmp eq i16 %0, 0
  %9 = icmp eq i16 %0, 4
  %10 = select i1 %8, i16 -193, i16 -1
  %11 = select i1 %9, i16 -249, i16 %10
  %12 = and i16 %11, %1
  %13 = inttoptr i16 %7 to ptr
  %14 = load volatile i8, ptr %13, align 1, !tbaa !16
  switch i16 %2, label %24 [
    i16 1, label %15
    i16 3, label %15
    i16 2, label %18
  ]

15:                                               ; preds = %3, %3
  %16 = trunc i16 %12 to i8
  %17 = or i8 %14, %16
  br label %22

18:                                               ; preds = %3
  %19 = trunc i16 %12 to i8
  %20 = xor i8 %19, -1
  %21 = and i8 %14, %20
  br label %22

22:                                               ; preds = %18, %15
  %23 = phi i8 [ %21, %18 ], [ %17, %15 ]
  store volatile i8 %23, ptr %13, align 1, !tbaa !16
  br label %24

24:                                               ; preds = %22, %3
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_GPIO_DeInit(i16 noundef %0) local_unnamed_addr #1 {
  %2 = add i16 %0, -1
  %3 = icmp ult i16 %2, 4
  %4 = add i16 %0, 133
  %5 = select i1 %3, i16 %4, i16 133
  %6 = icmp eq i16 %0, 0
  %7 = icmp eq i16 %0, 4
  %8 = select i1 %6, i8 63, i8 -1
  %9 = select i1 %7, i8 7, i8 %8
  %10 = inttoptr i16 %5 to ptr
  store volatile i8 %9, ptr %10, align 1, !tbaa !16
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_GPIO_WritePin(i16 noundef %0, i16 noundef zeroext %1, i16 noundef %2) local_unnamed_addr #1 {
  %4 = add i16 %0, -1
  %5 = icmp ult i16 %4, 4
  %6 = add i16 %0, 5
  %7 = select i1 %5, i16 %6, i16 5
  %8 = icmp eq i16 %0, 4
  %9 = icmp eq i16 %0, 0
  %10 = select i1 %9, i16 63, i16 255
  %11 = select i1 %8, i16 7, i16 %10
  %12 = and i16 %11, %1
  %13 = inttoptr i16 %7 to ptr
  %14 = load volatile i8, ptr %13, align 1, !tbaa !16
  %15 = icmp eq i16 %2, 1
  %16 = trunc nuw i16 %12 to i8
  %17 = or i8 %14, %16
  %18 = xor i8 %16, -1
  %19 = and i8 %14, %18
  %20 = select i1 %15, i8 %17, i8 %19
  store volatile i8 %20, ptr %13, align 1, !tbaa !16
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_GPIO_TogglePin(i16 noundef %0, i16 noundef zeroext %1) local_unnamed_addr #1 {
  %3 = add i16 %0, -1
  %4 = icmp ult i16 %3, 4
  %5 = add i16 %0, 5
  %6 = select i1 %4, i16 %5, i16 5
  %7 = icmp eq i16 %0, 4
  %8 = icmp eq i16 %0, 0
  %9 = select i1 %8, i16 63, i16 255
  %10 = select i1 %7, i16 7, i16 %9
  %11 = and i16 %10, %1
  %12 = inttoptr i16 %6 to ptr
  %13 = load volatile i8, ptr %12, align 1, !tbaa !16
  %14 = trunc nuw i16 %11 to i8
  %15 = xor i8 %13, %14
  store volatile i8 %15, ptr %12, align 1, !tbaa !16
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local range(i16 0, 2) i16 @EPIC_GPIO_ReadPin(i16 noundef %0, i16 noundef zeroext %1) local_unnamed_addr #3 {
  %3 = add i16 %0, -1
  %4 = icmp ult i16 %3, 4
  %5 = add i16 %0, 5
  %6 = select i1 %4, i16 %5, i16 5
  %7 = icmp eq i16 %0, 4
  %8 = icmp eq i16 %0, 0
  %9 = select i1 %8, i16 63, i16 255
  %10 = select i1 %7, i16 7, i16 %9
  %11 = and i16 %10, %1
  %12 = inttoptr i16 %6 to ptr
  %13 = load volatile i8, ptr %12, align 1, !tbaa !16
  %14 = zext i8 %13 to i16
  %15 = and i16 %11, %14
  %16 = icmp ne i16 %15, 0
  %17 = zext i1 %16 to i16
  ret i16 %17
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_GPIO_WritePort(i16 noundef %0, i8 noundef zeroext %1) local_unnamed_addr #1 {
  %3 = add i16 %0, -1
  %4 = icmp ult i16 %3, 4
  %5 = add i16 %0, 5
  %6 = select i1 %4, i16 %5, i16 5
  %7 = icmp eq i16 %0, 4
  %8 = icmp eq i16 %0, 0
  %9 = select i1 %8, i8 63, i8 -1
  %10 = select i1 %7, i8 7, i8 %9
  %11 = and i8 %10, %1
  %12 = inttoptr i16 %6 to ptr
  store volatile i8 %11, ptr %12, align 1, !tbaa !16
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext i8 @EPIC_GPIO_ReadPort(i16 noundef %0) local_unnamed_addr #3 {
  %2 = add i16 %0, -1
  %3 = icmp ult i16 %2, 4
  %4 = add i16 %0, 5
  %5 = select i1 %3, i16 %4, i16 5
  %6 = inttoptr i16 %5 to ptr
  %7 = load volatile i8, ptr %6, align 1, !tbaa !16
  ret i8 %7
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_GPIO_SetPullups(i16 noundef %0) local_unnamed_addr #1 {
  %2 = load volatile i8, ptr inttoptr (i16 129 to ptr), align 1, !tbaa !16
  %3 = icmp eq i16 %0, 1
  %4 = and i8 %2, 127
  %5 = select i1 %3, i8 0, i8 -128
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 129 to ptr), align 1, !tbaa !16
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(write, argmem: none, inaccessiblemem: none)
define dso_local void @EPIC_GPIO_RegisterChangeCallback(ptr noundef %0) local_unnamed_addr #4 {
  store ptr %0, ptr @s_rb_change_callback, align 2, !tbaa !11
  ret void
}

; Function Attrs: nounwind
define weak dso_local void @RB_IRQHandler() local_unnamed_addr #0 {
  %1 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %2 = and i8 %1, 1
  %3 = icmp eq i8 %2, 0
  br i1 %3, label %11, label %4

4:                                                ; preds = %0
  %5 = load volatile i8, ptr inttoptr (i16 6 to ptr), align 2, !tbaa !16
  %6 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %7 = and i8 %6, -2
  store volatile i8 %7, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %8 = load ptr, ptr @s_rb_change_callback, align 2, !tbaa !11
  %9 = icmp eq ptr %8, null
  br i1 %9, label %11, label %10

10:                                               ; preds = %4
  tail call void %8(i8 noundef zeroext %5) #9
  br label %11

11:                                               ; preds = %10, %4, %0
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local zeroext range(i8 0, 2) i8 @EPIC_IRQ_Disable() local_unnamed_addr #1 {
  %1 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %2 = lshr i8 %1, 7
  %3 = and i8 %1, 127
  store volatile i8 %3, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  ret i8 %2
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_Restore(i8 noundef zeroext %0) local_unnamed_addr #1 {
  %2 = icmp eq i8 %0, 0
  %3 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %4 = and i8 %3, 127
  %5 = select i1 %2, i8 0, i8 -128
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_Enable(i16 noundef %0) local_unnamed_addr #1 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %24, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !30
  %7 = getelementptr inbounds nuw i8, ptr %4, i16 1
  %8 = load i8, ptr %7, align 1, !tbaa !32
  %9 = icmp eq i8 %6, 0
  br i1 %9, label %10, label %20

10:                                               ; preds = %3
  %11 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %12 = load i8, ptr %11, align 1, !tbaa !33
  %13 = icmp eq i8 %12, 0
  br i1 %13, label %17, label %14

14:                                               ; preds = %10
  %15 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !16
  %16 = or i8 %15, %8
  store volatile i8 %16, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !16
  br label %20

17:                                               ; preds = %10
  %18 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !16
  %19 = or i8 %18, %8
  store volatile i8 %19, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !16
  br label %20

20:                                               ; preds = %17, %14, %3
  %21 = phi i8 [ %8, %3 ], [ 64, %17 ], [ 64, %14 ]
  %22 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %23 = or i8 %22, %21
  store volatile i8 %23, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  br label %24

24:                                               ; preds = %20, %1
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_DisableSrc(i16 noundef %0) local_unnamed_addr #1 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %25, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !30
  %7 = getelementptr inbounds nuw i8, ptr %4, i16 1
  %8 = load i8, ptr %7, align 1, !tbaa !32
  %9 = icmp eq i8 %6, 0
  br i1 %9, label %14, label %10

10:                                               ; preds = %3
  %11 = xor i8 %8, -1
  %12 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %13 = and i8 %12, %11
  store volatile i8 %13, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  br label %25

14:                                               ; preds = %3
  %15 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %16 = load i8, ptr %15, align 1, !tbaa !33
  %17 = icmp eq i8 %16, 0
  %18 = xor i8 %8, -1
  br i1 %17, label %22, label %19

19:                                               ; preds = %14
  %20 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !16
  %21 = and i8 %20, %18
  store volatile i8 %21, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !16
  br label %25

22:                                               ; preds = %14
  %23 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !16
  %24 = and i8 %23, %18
  store volatile i8 %24, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !16
  br label %25

25:                                               ; preds = %22, %19, %10, %1
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_ClearFlag(i16 noundef %0) local_unnamed_addr #1 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %21, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !30
  %7 = load i8, ptr %4, align 1, !tbaa !34
  %8 = icmp eq i8 %6, 0
  br i1 %8, label %13, label %9

9:                                                ; preds = %3
  %10 = xor i8 %7, -1
  %11 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %12 = and i8 %11, %10
  store volatile i8 %12, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  br label %21

13:                                               ; preds = %3
  %14 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %15 = load i8, ptr %14, align 1, !tbaa !33
  %16 = icmp eq i8 %15, 0
  %17 = select i1 %16, ptr inttoptr (i16 12 to ptr), ptr inttoptr (i16 13 to ptr)
  %18 = load volatile i8, ptr %17, align 1, !tbaa !16
  %19 = xor i8 %7, -1
  %20 = and i8 %18, %19
  store volatile i8 %20, ptr %17, align 1, !tbaa !16
  br label %21

21:                                               ; preds = %13, %9, %1
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_IRQ_GetFlag(i16 noundef %0) local_unnamed_addr #3 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %20, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !30
  %7 = load i8, ptr %4, align 1, !tbaa !34
  %8 = icmp eq i8 %6, 0
  br i1 %8, label %9, label %14

9:                                                ; preds = %3
  %10 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %11 = load i8, ptr %10, align 1, !tbaa !33
  %12 = icmp eq i8 %11, 0
  %13 = select i1 %12, ptr inttoptr (i16 12 to ptr), ptr inttoptr (i16 13 to ptr)
  br label %14

14:                                               ; preds = %9, %3
  %15 = phi ptr [ %13, %9 ], [ inttoptr (i16 11 to ptr), %3 ]
  %16 = load volatile i8, ptr %15, align 1, !tbaa !16
  %17 = and i8 %16, %7
  %18 = icmp ne i8 %17, 0
  %19 = zext i1 %18 to i8
  br label %20

20:                                               ; preds = %14, %1
  %21 = phi i8 [ %19, %14 ], [ 0, %1 ]
  ret i8 %21
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @EPIC_IRQ_SetPriority(i16 noundef %0, i16 noundef %1) local_unnamed_addr #2 {
  ret void
}

; Function Attrs: noinline nounwind
define dso_local msp430_intrcc void @PIC16_IRQ_Handler() #5 {
  tail call void @epic_dispatch_all_irqs() #9
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_dispatch_all_irqs() local_unnamed_addr #0 {
  %1 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %2 = and i8 %1, 1
  %3 = icmp eq i8 %2, 0
  br i1 %3, label %12, label %4

4:                                                ; preds = %0
  %5 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !16
  %6 = and i8 %5, 1
  %7 = icmp eq i8 %6, 0
  br i1 %7, label %9, label %8

8:                                                ; preds = %4
  tail call void @TIMER1_IRQHandler() #9
  br label %12

9:                                                ; preds = %4
  %10 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %11 = and i8 %10, -2
  store volatile i8 %11, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  br label %12

12:                                               ; preds = %9, %8, %0
  %13 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %14 = and i8 %13, 2
  %15 = icmp eq i8 %14, 0
  br i1 %15, label %19, label %16

16:                                               ; preds = %12
  %17 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %18 = and i8 %17, -3
  store volatile i8 %18, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  br label %19

19:                                               ; preds = %16, %12
  %20 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %21 = and i8 %20, 4
  %22 = icmp eq i8 %21, 0
  br i1 %22, label %24, label %23

23:                                               ; preds = %19
  tail call void @CCP1_IRQHandler() #9
  br label %24

24:                                               ; preds = %23, %19
  %25 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %26 = and i8 %25, 8
  %27 = icmp eq i8 %26, 0
  br i1 %27, label %31, label %28

28:                                               ; preds = %24
  %29 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %30 = and i8 %29, -9
  store volatile i8 %30, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  br label %31

31:                                               ; preds = %28, %24
  %32 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %33 = and i8 %32, 32
  %34 = icmp eq i8 %33, 0
  br i1 %34, label %38, label %35

35:                                               ; preds = %31
  %36 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %37 = and i8 %36, -33
  store volatile i8 %37, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  br label %38

38:                                               ; preds = %35, %31
  %39 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %40 = and i8 %39, 16
  %41 = icmp eq i8 %40, 0
  br i1 %41, label %49, label %42

42:                                               ; preds = %38
  %43 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !16
  %44 = and i8 %43, 16
  %45 = icmp eq i8 %44, 0
  br i1 %45, label %46, label %49

46:                                               ; preds = %42
  %47 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %48 = and i8 %47, -17
  store volatile i8 %48, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  br label %49

49:                                               ; preds = %46, %42, %38
  %50 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %51 = and i8 %50, 64
  %52 = icmp eq i8 %51, 0
  br i1 %52, label %56, label %53

53:                                               ; preds = %49
  %54 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %55 = and i8 %54, -65
  store volatile i8 %55, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  br label %56

56:                                               ; preds = %53, %49
  %57 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %58 = icmp sgt i8 %57, -1
  br i1 %58, label %62, label %59

59:                                               ; preds = %56
  %60 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  %61 = and i8 %60, 127
  store volatile i8 %61, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !16
  br label %62

62:                                               ; preds = %59, %56
  %63 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  %64 = and i8 %63, 1
  %65 = icmp eq i8 %64, 0
  br i1 %65, label %67, label %66

66:                                               ; preds = %62
  tail call void @CCP2_IRQHandler() #9
  br label %67

67:                                               ; preds = %66, %62
  %68 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  %69 = and i8 %68, 8
  %70 = icmp eq i8 %69, 0
  br i1 %70, label %74, label %71

71:                                               ; preds = %67
  %72 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  %73 = and i8 %72, -9
  store volatile i8 %73, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  br label %74

74:                                               ; preds = %71, %67
  %75 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  %76 = and i8 %75, 16
  %77 = icmp eq i8 %76, 0
  br i1 %77, label %85, label %78

78:                                               ; preds = %74
  %79 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !16
  %80 = and i8 %79, 16
  %81 = icmp eq i8 %80, 0
  br i1 %81, label %82, label %85

82:                                               ; preds = %78
  %83 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  %84 = and i8 %83, -17
  store volatile i8 %84, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  br label %85

85:                                               ; preds = %82, %78, %74
  %86 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  %87 = and i8 %86, 64
  %88 = icmp eq i8 %87, 0
  br i1 %88, label %92, label %89

89:                                               ; preds = %85
  %90 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  %91 = and i8 %90, -65
  store volatile i8 %91, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !16
  br label %92

92:                                               ; preds = %89, %85
  %93 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %94 = and i8 %93, 4
  %95 = icmp eq i8 %94, 0
  br i1 %95, label %99, label %96

96:                                               ; preds = %92
  %97 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %98 = and i8 %97, -5
  store volatile i8 %98, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  br label %99

99:                                               ; preds = %96, %92
  %100 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %101 = and i8 %100, 1
  %102 = icmp eq i8 %101, 0
  br i1 %102, label %106, label %103

103:                                              ; preds = %99
  %104 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  %105 = and i8 %104, -2
  store volatile i8 %105, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !16
  br label %106

106:                                              ; preds = %103, %99
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @epic_harness_init(i32 noundef %0) local_unnamed_addr #2 {
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @epic_harness_tick() local_unnamed_addr #2 {
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i16 @epic_harness_running(i32 noundef %0) local_unnamed_addr #2 {
  ret i16 1
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @epic_harness_log(ptr nocapture noundef readnone %0, ...) local_unnamed_addr #2 {
  ret void
}

; Function Attrs: nounwind
define dso_local range(i16 0, 5) i16 @EPIC_SWUART_Init(ptr noundef %0, i16 noundef %1, i16 noundef zeroext %2, i16 noundef %3, i16 noundef zeroext %4, i32 noundef %5, i32 noundef %6) local_unnamed_addr #0 {
  %8 = alloca %struct.CCP_HandleTypeDef, align 2
  %9 = alloca %struct.CCP_HandleTypeDef, align 2
  %10 = icmp eq ptr %0, null
  br i1 %10, label %56, label %11

11:                                               ; preds = %7
  %12 = icmp eq i16 %1, 2
  %13 = icmp eq i16 %2, 2
  %14 = and i1 %12, %13
  %15 = icmp eq i16 %3, 2
  %16 = and i1 %14, %15
  %17 = icmp eq i16 %4, 4
  %18 = and i1 %16, %17
  %19 = load ptr, ptr @g_chan_a, align 2
  %20 = icmp eq ptr %19, null
  %21 = select i1 %18, i1 %20, i1 false
  br i1 %21, label %22, label %56

22:                                               ; preds = %11
  store i16 %1, ptr %0, align 2, !tbaa !35
  %23 = getelementptr inbounds nuw i8, ptr %0, i16 2
  store i16 %2, ptr %23, align 2, !tbaa !37
  %24 = getelementptr inbounds nuw i8, ptr %0, i16 4
  store i16 %3, ptr %24, align 2, !tbaa !38
  %25 = getelementptr inbounds nuw i8, ptr %0, i16 6
  store i16 %4, ptr %25, align 2, !tbaa !39
  %26 = getelementptr inbounds nuw i8, ptr %0, i16 8
  store volatile i8 0, ptr %26, align 2, !tbaa !40
  %27 = getelementptr inbounds nuw i8, ptr %0, i16 12
  store volatile i16 0, ptr %27, align 2, !tbaa !41
  %28 = getelementptr inbounds nuw i8, ptr %0, i16 24
  store volatile i8 0, ptr %28, align 2, !tbaa !42
  %29 = getelementptr inbounds nuw i8, ptr %0, i16 23
  store volatile i8 0, ptr %29, align 1, !tbaa !43
  %30 = getelementptr inbounds nuw i8, ptr %0, i16 22
  store volatile i8 0, ptr %30, align 2, !tbaa !44
  %31 = getelementptr inbounds nuw i8, ptr %0, i16 25
  store volatile i8 0, ptr %31, align 1, !tbaa !45
  %32 = getelementptr inbounds nuw i8, ptr %0, i16 28
  store volatile i16 0, ptr %32, align 2, !tbaa !46
  %33 = getelementptr inbounds nuw i8, ptr %0, i16 40
  store volatile i8 0, ptr %33, align 2, !tbaa !47
  %34 = getelementptr inbounds nuw i8, ptr %0, i16 39
  store volatile i8 0, ptr %34, align 1, !tbaa !48
  %35 = getelementptr inbounds nuw i8, ptr %0, i16 38
  store volatile i8 0, ptr %35, align 2, !tbaa !49
  %36 = getelementptr inbounds nuw i8, ptr %0, i16 42
  store volatile i16 0, ptr %36, align 2, !tbaa !50
  %37 = lshr i32 %5, 2
  %38 = lshr i32 %6, 1
  %39 = add nuw i32 %38, %37
  %40 = udiv i32 %39, %6
  %41 = tail call i32 @llvm.umin.i32(i32 %40, i32 65535)
  %42 = tail call i32 @llvm.umax.i32(i32 %41, i32 1)
  %43 = trunc nuw i32 %42 to i16
  store i16 %43, ptr @g_cycles_per_bit, align 2, !tbaa !22
  tail call void @EPIC_GPIO_Init(i16 noundef %1, i16 noundef zeroext %2, i16 noundef 2) #9
  tail call void @EPIC_GPIO_WritePin(i16 noundef %1, i16 noundef zeroext %2, i16 noundef 1) #9
  tail call void @EPIC_GPIO_Init(i16 noundef %3, i16 noundef zeroext %4, i16 noundef 1) #9
  store i16 0, ptr @s_timer1, align 2, !tbaa !51
  store i16 0, ptr getelementptr inbounds nuw (i8, ptr @s_timer1, i16 2), align 2, !tbaa !51
  store i16 0, ptr getelementptr inbounds nuw (i8, ptr @s_timer1, i16 4), align 2, !tbaa !51
  store i16 0, ptr getelementptr inbounds nuw (i8, ptr @s_timer1, i16 6), align 2, !tbaa !51
  store i16 0, ptr getelementptr inbounds nuw (i8, ptr @s_timer1, i16 8), align 2, !tbaa !22
  store ptr null, ptr getelementptr inbounds nuw (i8, ptr @s_timer1, i16 10), align 2, !tbaa !11
  %44 = tail call i16 @EPIC_TIMER1_Init(ptr noundef nonnull @s_timer1) #9
  %45 = tail call i16 @EPIC_TIMER1_Start(ptr noundef nonnull @s_timer1) #9
  call void @llvm.lifetime.start.p0(i64 12, ptr nonnull %8) #10
  store i16 1, ptr %8, align 2, !tbaa !2
  %46 = getelementptr inbounds nuw i8, ptr %8, i16 2
  store i16 4, ptr %46, align 2, !tbaa !12
  %47 = getelementptr inbounds nuw i8, ptr %8, i16 4
  store i16 0, ptr %47, align 2, !tbaa !17
  %48 = getelementptr inbounds nuw i8, ptr %8, i16 6
  store i32 0, ptr %48, align 2
  %49 = getelementptr inbounds nuw i8, ptr %8, i16 10
  store ptr @on_rx_event_a, ptr %49, align 2, !tbaa !10
  %50 = call i16 @EPIC_CCP_Init(ptr noundef nonnull %8) #9
  call void @llvm.lifetime.start.p0(i64 12, ptr nonnull %9) #10
  store i16 2, ptr %9, align 2, !tbaa !2
  %51 = getelementptr inbounds nuw i8, ptr %9, i16 2
  store i16 0, ptr %51, align 2, !tbaa !12
  %52 = getelementptr inbounds nuw i8, ptr %9, i16 4
  store i16 0, ptr %52, align 2, !tbaa !17
  %53 = getelementptr inbounds nuw i8, ptr %9, i16 6
  store i32 0, ptr %53, align 2
  %54 = getelementptr inbounds nuw i8, ptr %9, i16 10
  store ptr @on_tx_event_a, ptr %54, align 2, !tbaa !10
  %55 = call i16 @EPIC_CCP_Init(ptr noundef nonnull %9) #9
  call void @EPIC_IRQ_Restore(i8 noundef zeroext 1) #9
  store ptr %0, ptr @g_chan_a, align 2, !tbaa !11
  call void @llvm.lifetime.end.p0(i64 12, ptr nonnull %9) #10
  call void @llvm.lifetime.end.p0(i64 12, ptr nonnull %8) #10
  br label %56

56:                                               ; preds = %22, %11, %7
  %57 = phi i16 [ 4, %7 ], [ 0, %22 ], [ 4, %11 ]
  ret i16 %57
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.umin.i32(i32, i32) #6

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.umax.i32(i32, i32) #6

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #7

; Function Attrs: nounwind
define internal void @on_rx_event_a() #0 {
  %1 = load ptr, ptr @g_chan_a, align 2, !tbaa !11
  %2 = getelementptr inbounds nuw i8, ptr %1, i16 25
  %3 = load volatile i8, ptr %2, align 1, !tbaa !45
  %4 = icmp eq i8 %3, 0
  %5 = getelementptr inbounds nuw i8, ptr %1, i16 4
  %6 = load i16, ptr %5, align 2, !tbaa !38
  %7 = getelementptr inbounds nuw i8, ptr %1, i16 6
  %8 = load i16, ptr %7, align 2, !tbaa !39
  %9 = tail call i16 @EPIC_GPIO_ReadPin(i16 noundef %6, i16 noundef zeroext %8) #9
  br i1 %4, label %66, label %10

10:                                               ; preds = %0
  %11 = icmp eq i16 %9, 1
  %12 = load volatile i8, ptr %2, align 1, !tbaa !45
  %13 = icmp eq i8 %12, 10
  br i1 %13, label %14, label %41

14:                                               ; preds = %10
  br i1 %11, label %15, label %36

15:                                               ; preds = %14
  %16 = getelementptr inbounds nuw i8, ptr %1, i16 26
  %17 = load volatile i8, ptr %16, align 2, !tbaa !52
  %18 = getelementptr inbounds nuw i8, ptr %1, i16 40
  %19 = load volatile i8, ptr %18, align 2, !tbaa !47
  %20 = icmp ugt i8 %19, 7
  br i1 %20, label %21, label %25

21:                                               ; preds = %15
  %22 = getelementptr inbounds nuw i8, ptr %1, i16 42
  %23 = load volatile i16, ptr %22, align 2, !tbaa !50
  %24 = add i16 %23, 1
  store volatile i16 %24, ptr %22, align 2, !tbaa !50
  br label %40

25:                                               ; preds = %15
  %26 = getelementptr inbounds nuw i8, ptr %1, i16 30
  %27 = getelementptr inbounds nuw i8, ptr %1, i16 38
  %28 = load volatile i8, ptr %27, align 2, !tbaa !49
  %29 = zext i8 %28 to i16
  %30 = getelementptr inbounds nuw [8 x i8], ptr %26, i16 0, i16 %29
  store i8 %17, ptr %30, align 1, !tbaa !16
  %31 = load volatile i8, ptr %27, align 2, !tbaa !49
  %32 = add i8 %31, 1
  %33 = and i8 %32, 7
  store volatile i8 %33, ptr %27, align 2, !tbaa !49
  %34 = load volatile i8, ptr %18, align 2, !tbaa !47
  %35 = add i8 %34, 1
  store volatile i8 %35, ptr %18, align 2, !tbaa !47
  br label %40

36:                                               ; preds = %14
  %37 = getelementptr inbounds nuw i8, ptr %1, i16 42
  %38 = load volatile i16, ptr %37, align 2, !tbaa !50
  %39 = add i16 %38, 1
  store volatile i16 %39, ptr %37, align 2, !tbaa !50
  br label %40

40:                                               ; preds = %36, %25, %21
  store volatile i8 0, ptr %2, align 1, !tbaa !45
  store volatile i8 4, ptr inttoptr (i16 23 to ptr), align 1, !tbaa !16
  br label %83

41:                                               ; preds = %10
  %42 = getelementptr inbounds nuw i8, ptr %1, i16 26
  %43 = load volatile i8, ptr %42, align 2, !tbaa !52
  %44 = lshr i8 %43, 1
  %45 = select i1 %11, i8 -128, i8 0
  %46 = or disjoint i8 %44, %45
  store volatile i8 %46, ptr %42, align 2, !tbaa !52
  %47 = getelementptr inbounds nuw i8, ptr %1, i16 27
  %48 = load volatile i8, ptr %47, align 1, !tbaa !53
  %49 = add i8 %48, 1
  store volatile i8 %49, ptr %47, align 1, !tbaa !53
  %50 = load volatile i8, ptr %47, align 1, !tbaa !53
  %51 = icmp ult i8 %50, 8
  br i1 %51, label %52, label %55

52:                                               ; preds = %41
  %53 = load volatile i8, ptr %47, align 1, !tbaa !53
  %54 = add i8 %53, 2
  br label %55

55:                                               ; preds = %52, %41
  %56 = phi i8 [ %54, %52 ], [ 10, %41 ]
  store volatile i8 %56, ptr %2, align 1, !tbaa !45
  %57 = getelementptr inbounds nuw i8, ptr %1, i16 28
  %58 = load volatile i16, ptr %57, align 2, !tbaa !46
  %59 = load i16, ptr @g_cycles_per_bit, align 2, !tbaa !22
  %60 = add i16 %59, %58
  store volatile i16 %60, ptr %57, align 2, !tbaa !46
  %61 = load volatile i16, ptr %57, align 2, !tbaa !46
  %62 = trunc i16 %61 to i8
  store volatile i8 %62, ptr inttoptr (i16 21 to ptr), align 1, !tbaa !16
  %63 = load volatile i16, ptr %57, align 2, !tbaa !46
  %64 = lshr i16 %63, 8
  %65 = trunc nuw i16 %64 to i8
  store volatile i8 %65, ptr inttoptr (i16 22 to ptr), align 2, !tbaa !16
  br label %83

66:                                               ; preds = %0
  %67 = icmp eq i16 %9, 0
  br i1 %67, label %68, label %83

68:                                               ; preds = %66
  %69 = getelementptr inbounds nuw i8, ptr %1, i16 26
  store volatile i8 0, ptr %69, align 2, !tbaa !52
  %70 = getelementptr inbounds nuw i8, ptr %1, i16 27
  store volatile i8 0, ptr %70, align 1, !tbaa !53
  store volatile i8 2, ptr %2, align 1, !tbaa !45
  %71 = tail call zeroext i16 @EPIC_TIMER1_ReadCounter() #9
  %72 = load i16, ptr @g_cycles_per_bit, align 2, !tbaa !22
  %73 = lshr i16 %72, 1
  %74 = add i16 %71, -325
  %75 = add i16 %74, %72
  %76 = add i16 %75, %73
  %77 = getelementptr inbounds nuw i8, ptr %1, i16 28
  store volatile i16 %76, ptr %77, align 2, !tbaa !46
  %78 = load volatile i16, ptr %77, align 2, !tbaa !46
  %79 = trunc i16 %78 to i8
  store volatile i8 %79, ptr inttoptr (i16 21 to ptr), align 1, !tbaa !16
  %80 = load volatile i16, ptr %77, align 2, !tbaa !46
  %81 = lshr i16 %80, 8
  %82 = trunc nuw i16 %81 to i8
  store volatile i8 %82, ptr inttoptr (i16 22 to ptr), align 2, !tbaa !16
  store volatile i8 10, ptr inttoptr (i16 23 to ptr), align 1, !tbaa !16
  br label %83

83:                                               ; preds = %68, %66, %55, %40
  ret void
}

; Function Attrs: nounwind
define internal void @on_tx_event_a() #0 {
  %1 = load ptr, ptr @g_chan_a, align 2, !tbaa !11
  %2 = getelementptr inbounds nuw i8, ptr %1, i16 8
  %3 = load volatile i8, ptr %2, align 2, !tbaa !40
  switch i8 %3, label %35 [
    i8 0, label %4
    i8 1, label %22
  ]

4:                                                ; preds = %0
  %5 = getelementptr inbounds nuw i8, ptr %1, i16 24
  %6 = load volatile i8, ptr %5, align 2, !tbaa !42
  %7 = icmp eq i8 %6, 0
  br i1 %7, label %45, label %8

8:                                                ; preds = %4
  %9 = getelementptr inbounds nuw i8, ptr %1, i16 14
  %10 = getelementptr inbounds nuw i8, ptr %1, i16 23
  %11 = load volatile i8, ptr %10, align 1, !tbaa !43
  %12 = zext i8 %11 to i16
  %13 = getelementptr inbounds nuw [8 x i8], ptr %9, i16 0, i16 %12
  %14 = load i8, ptr %13, align 1, !tbaa !16
  %15 = getelementptr inbounds nuw i8, ptr %1, i16 9
  store volatile i8 %14, ptr %15, align 1, !tbaa !54
  %16 = load volatile i8, ptr %10, align 1, !tbaa !43
  %17 = add i8 %16, 1
  %18 = and i8 %17, 7
  store volatile i8 %18, ptr %10, align 1, !tbaa !43
  %19 = load volatile i8, ptr %5, align 2, !tbaa !42
  %20 = add i8 %19, -1
  store volatile i8 %20, ptr %5, align 2, !tbaa !42
  %21 = getelementptr inbounds nuw i8, ptr %1, i16 10
  store volatile i8 0, ptr %21, align 2, !tbaa !55
  br label %35

22:                                               ; preds = %0
  %23 = getelementptr inbounds nuw i8, ptr %1, i16 9
  %24 = load volatile i8, ptr %23, align 1, !tbaa !54
  %25 = and i8 %24, 1
  %26 = icmp eq i8 %25, 0
  %27 = select i1 %26, i16 9, i16 8
  %28 = load volatile i8, ptr %23, align 1, !tbaa !54
  %29 = lshr i8 %28, 1
  store volatile i8 %29, ptr %23, align 1, !tbaa !54
  %30 = getelementptr inbounds nuw i8, ptr %1, i16 10
  %31 = load volatile i8, ptr %30, align 2, !tbaa !55
  %32 = add i8 %31, 1
  store volatile i8 %32, ptr %30, align 2, !tbaa !55
  %33 = load volatile i8, ptr %30, align 2, !tbaa !55
  %34 = icmp ugt i8 %33, 7
  br i1 %34, label %35, label %38

35:                                               ; preds = %22, %8, %0
  %36 = phi i8 [ 1, %8 ], [ 2, %22 ], [ 0, %0 ]
  %37 = phi i16 [ 9, %8 ], [ %27, %22 ], [ 8, %0 ]
  store volatile i8 %36, ptr %2, align 2, !tbaa !40
  br label %38

38:                                               ; preds = %35, %22
  %39 = phi i16 [ %27, %22 ], [ %37, %35 ]
  %40 = getelementptr inbounds nuw i8, ptr %1, i16 12
  %41 = load volatile i16, ptr %40, align 2, !tbaa !41
  %42 = load i16, ptr @g_cycles_per_bit, align 2, !tbaa !22
  %43 = add i16 %42, %41
  store volatile i16 %43, ptr %40, align 2, !tbaa !41
  %44 = load volatile i16, ptr %40, align 2, !tbaa !41
  tail call void @EPIC_CCP_SetCompare(i16 noundef 2, i16 noundef zeroext %44) #9
  br label %45

45:                                               ; preds = %38, %4
  %46 = phi i16 [ %39, %38 ], [ 0, %4 ]
  tail call void @EPIC_CCP_SetMode(i16 noundef 2, i16 noundef %46) #9
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #7

; Function Attrs: nounwind
define dso_local range(i16 0, 5) i16 @EPIC_SWUART_DeInit(ptr noundef readonly %0) local_unnamed_addr #0 {
  %2 = icmp ne ptr %0, null
  %3 = load ptr, ptr @g_chan_a, align 2
  %4 = icmp eq ptr %3, %0
  %5 = select i1 %2, i1 %4, i1 false
  br i1 %5, label %6, label %16

6:                                                ; preds = %1
  %7 = tail call i16 @EPIC_CCP_DeInit(i16 noundef 1) #9
  %8 = tail call i16 @EPIC_CCP_DeInit(i16 noundef 2) #9
  store ptr null, ptr @g_chan_a, align 2, !tbaa !11
  %9 = load i16, ptr %0, align 2, !tbaa !35
  %10 = getelementptr inbounds nuw i8, ptr %0, i16 2
  %11 = load i16, ptr %10, align 2, !tbaa !37
  tail call void @EPIC_GPIO_WritePin(i16 noundef %9, i16 noundef zeroext %11, i16 noundef 1) #9
  %12 = load ptr, ptr @g_chan_a, align 2, !tbaa !11
  %13 = icmp eq ptr %12, null
  br i1 %13, label %14, label %16

14:                                               ; preds = %6
  %15 = tail call i16 @EPIC_TIMER1_DeInit() #9
  br label %16

16:                                               ; preds = %14, %6, %1
  %17 = phi i16 [ 4, %1 ], [ 0, %14 ], [ 0, %6 ]
  ret i16 %17
}

; Function Attrs: nounwind
define dso_local i16 @EPIC_SWUART_Write(ptr noundef %0, ptr nocapture noundef readonly %1, i16 noundef %2) local_unnamed_addr #0 {
  %4 = icmp eq ptr %0, null
  br i1 %4, label %54, label %5

5:                                                ; preds = %3
  %6 = getelementptr inbounds nuw i8, ptr %0, i16 24
  %7 = icmp eq i16 %2, 0
  br i1 %7, label %28, label %8

8:                                                ; preds = %5
  %9 = getelementptr inbounds nuw i8, ptr %0, i16 14
  %10 = getelementptr inbounds nuw i8, ptr %0, i16 22
  br label %11

11:                                               ; preds = %15, %8
  %12 = phi i16 [ 0, %8 ], [ %26, %15 ]
  %13 = load volatile i8, ptr %6, align 2, !tbaa !42
  %14 = icmp ult i8 %13, 8
  br i1 %14, label %15, label %28

15:                                               ; preds = %11
  %16 = getelementptr inbounds nuw i8, ptr %1, i16 %12
  %17 = load i8, ptr %16, align 1, !tbaa !16
  %18 = load volatile i8, ptr %10, align 2, !tbaa !44
  %19 = zext i8 %18 to i16
  %20 = getelementptr inbounds nuw [8 x i8], ptr %9, i16 0, i16 %19
  store i8 %17, ptr %20, align 1, !tbaa !16
  %21 = load volatile i8, ptr %10, align 2, !tbaa !44
  %22 = add i8 %21, 1
  %23 = and i8 %22, 7
  store volatile i8 %23, ptr %10, align 2, !tbaa !44
  %24 = load volatile i8, ptr %6, align 2, !tbaa !42
  %25 = add i8 %24, 1
  store volatile i8 %25, ptr %6, align 2, !tbaa !42
  %26 = add nuw i16 %12, 1
  %27 = icmp eq i16 %26, %2
  br i1 %27, label %28, label %11, !llvm.loop !56

28:                                               ; preds = %15, %11, %5
  %29 = phi i16 [ 0, %5 ], [ %12, %11 ], [ %2, %15 ]
  %30 = icmp eq i16 %29, 0
  br i1 %30, label %54, label %31

31:                                               ; preds = %28
  %32 = getelementptr inbounds nuw i8, ptr %0, i16 8
  %33 = load volatile i8, ptr %32, align 2, !tbaa !40
  %34 = icmp eq i8 %33, 0
  br i1 %34, label %35, label %54

35:                                               ; preds = %31
  %36 = getelementptr inbounds nuw i8, ptr %0, i16 14
  %37 = getelementptr inbounds nuw i8, ptr %0, i16 23
  %38 = load volatile i8, ptr %37, align 1, !tbaa !43
  %39 = zext i8 %38 to i16
  %40 = getelementptr inbounds nuw [8 x i8], ptr %36, i16 0, i16 %39
  %41 = load i8, ptr %40, align 1, !tbaa !16
  %42 = getelementptr inbounds nuw i8, ptr %0, i16 9
  store volatile i8 %41, ptr %42, align 1, !tbaa !54
  %43 = load volatile i8, ptr %37, align 1, !tbaa !43
  %44 = add i8 %43, 1
  %45 = and i8 %44, 7
  store volatile i8 %45, ptr %37, align 1, !tbaa !43
  %46 = getelementptr inbounds nuw i8, ptr %0, i16 24
  %47 = load volatile i8, ptr %46, align 2, !tbaa !42
  %48 = add i8 %47, -1
  store volatile i8 %48, ptr %46, align 2, !tbaa !42
  %49 = getelementptr inbounds nuw i8, ptr %0, i16 10
  store volatile i8 0, ptr %49, align 2, !tbaa !55
  store volatile i8 1, ptr %32, align 2, !tbaa !40
  %50 = tail call zeroext i16 @EPIC_TIMER1_ReadCounter() #9
  %51 = add i16 %50, 120
  %52 = getelementptr inbounds nuw i8, ptr %0, i16 12
  store volatile i16 %51, ptr %52, align 2, !tbaa !41
  %53 = load volatile i16, ptr %52, align 2, !tbaa !41
  tail call void @EPIC_CCP_SetCompare(i16 noundef 2, i16 noundef zeroext %53) #9
  tail call void @EPIC_CCP_SetMode(i16 noundef 2, i16 noundef 9) #9
  br label %54

54:                                               ; preds = %35, %31, %28, %3
  %55 = phi i16 [ 0, %3 ], [ %29, %35 ], [ %29, %31 ], [ %29, %28 ]
  ret i16 %55
}

; Function Attrs: nofree norecurse nounwind memory(argmem: readwrite, inaccessiblemem: readwrite)
define dso_local i16 @EPIC_SWUART_Read(ptr noundef %0, ptr nocapture noundef writeonly %1, i16 noundef %2) local_unnamed_addr #8 {
  %4 = icmp eq ptr %0, null
  br i1 %4, label %28, label %5

5:                                                ; preds = %3
  %6 = getelementptr inbounds nuw i8, ptr %0, i16 40
  %7 = icmp eq i16 %2, 0
  br i1 %7, label %28, label %8

8:                                                ; preds = %5
  %9 = getelementptr inbounds nuw i8, ptr %0, i16 30
  %10 = getelementptr inbounds nuw i8, ptr %0, i16 39
  br label %11

11:                                               ; preds = %15, %8
  %12 = phi i16 [ 0, %8 ], [ %26, %15 ]
  %13 = load volatile i8, ptr %6, align 2, !tbaa !47
  %14 = icmp eq i8 %13, 0
  br i1 %14, label %28, label %15

15:                                               ; preds = %11
  %16 = load volatile i8, ptr %10, align 1, !tbaa !48
  %17 = zext i8 %16 to i16
  %18 = getelementptr inbounds nuw [8 x i8], ptr %9, i16 0, i16 %17
  %19 = load i8, ptr %18, align 1, !tbaa !16
  %20 = getelementptr inbounds nuw i8, ptr %1, i16 %12
  store i8 %19, ptr %20, align 1, !tbaa !16
  %21 = load volatile i8, ptr %10, align 1, !tbaa !48
  %22 = add i8 %21, 1
  %23 = and i8 %22, 7
  store volatile i8 %23, ptr %10, align 1, !tbaa !48
  %24 = load volatile i8, ptr %6, align 2, !tbaa !47
  %25 = add i8 %24, -1
  store volatile i8 %25, ptr %6, align 2, !tbaa !47
  %26 = add nuw i16 %12, 1
  %27 = icmp eq i16 %26, %2
  br i1 %27, label %28, label %11, !llvm.loop !57

28:                                               ; preds = %15, %11, %5, %3
  %29 = phi i16 [ 0, %3 ], [ 0, %5 ], [ %12, %11 ], [ %2, %15 ]
  ret i16 %29
}

; Function Attrs: nofree norecurse nounwind memory(argmem: readwrite, inaccessiblemem: readwrite)
define dso_local zeroext i16 @EPIC_SWUART_GetErrorCount(ptr noundef %0) local_unnamed_addr #8 {
  %2 = icmp eq ptr %0, null
  br i1 %2, label %9, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw i8, ptr %0, i16 42
  br label %5

5:                                                ; preds = %5, %3
  %6 = load volatile i16, ptr %4, align 2, !tbaa !50
  %7 = load volatile i16, ptr %4, align 2, !tbaa !50
  %8 = icmp eq i16 %6, %7
  br i1 %8, label %9, label %5, !llvm.loop !58

9:                                                ; preds = %5, %1
  %10 = phi i16 [ 0, %1 ], [ %6, %5 ]
  ret i16 %10
}

attributes #0 = { nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { nofree norecurse nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { mustprogress nofree norecurse nounwind willreturn "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #4 = { mustprogress nofree norecurse nosync nounwind willreturn memory(write, argmem: none, inaccessiblemem: none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #5 = { noinline nounwind "interrupt"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #6 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #7 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #8 = { nofree norecurse nounwind memory(argmem: readwrite, inaccessiblemem: readwrite) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #9 = { nobuiltin nounwind "no-builtins" }
attributes #10 = { nounwind }

!llvm.ident = !{!0, !0, !0, !0, !0, !0, !0, !0}
!llvm.module.flags = !{!1}

!0 = !{!"clang version 20.1.8"}
!1 = !{i32 1, !"wchar_size", i32 2}
!2 = !{!3, !4, i64 0}
!3 = !{!"", !4, i64 0, !4, i64 2, !7, i64 4, !8, i64 6, !9, i64 10}
!4 = !{!"int", !5, i64 0}
!5 = !{!"omnipotent char", !6, i64 0}
!6 = !{!"Simple C/C++ TBAA"}
!7 = !{!"short", !5, i64 0}
!8 = !{!"", !7, i64 0, !7, i64 2}
!9 = !{!"any pointer", !5, i64 0}
!10 = !{!3, !9, i64 10}
!11 = !{!9, !9, i64 0}
!12 = !{!3, !4, i64 2}
!13 = !{!3, !7, i64 8}
!14 = !{!15, !5, i64 0}
!15 = !{!"", !5, i64 0, !5, i64 1, !5, i64 2, !4, i64 4}
!16 = !{!5, !5, i64 0}
!17 = !{!3, !7, i64 4}
!18 = distinct !{!18, !19, !20}
!19 = !{!"llvm.loop.mustprogress"}
!20 = !{!"llvm.loop.unroll.disable"}
!21 = distinct !{!21, !19, !20}
!22 = !{!7, !7, i64 0}
!23 = !{!24, !9, i64 10}
!24 = !{!"", !4, i64 0, !4, i64 2, !4, i64 4, !4, i64 6, !7, i64 8, !9, i64 10}
!25 = !{!24, !7, i64 8}
!26 = !{!24, !4, i64 6}
!27 = !{!24, !4, i64 4}
!28 = !{!24, !4, i64 2}
!29 = !{!24, !4, i64 0}
!30 = !{!31, !5, i64 2}
!31 = !{!"", !5, i64 0, !5, i64 1, !5, i64 2, !5, i64 3}
!32 = !{!31, !5, i64 1}
!33 = !{!31, !5, i64 3}
!34 = !{!31, !5, i64 0}
!35 = !{!36, !4, i64 0}
!36 = !{!"", !4, i64 0, !7, i64 2, !4, i64 4, !7, i64 6, !5, i64 8, !5, i64 9, !5, i64 10, !7, i64 12, !5, i64 14, !5, i64 22, !5, i64 23, !5, i64 24, !5, i64 25, !5, i64 26, !5, i64 27, !7, i64 28, !5, i64 30, !5, i64 38, !5, i64 39, !5, i64 40, !7, i64 42}
!37 = !{!36, !7, i64 2}
!38 = !{!36, !4, i64 4}
!39 = !{!36, !7, i64 6}
!40 = !{!36, !5, i64 8}
!41 = !{!36, !7, i64 12}
!42 = !{!36, !5, i64 24}
!43 = !{!36, !5, i64 23}
!44 = !{!36, !5, i64 22}
!45 = !{!36, !5, i64 25}
!46 = !{!36, !7, i64 28}
!47 = !{!36, !5, i64 40}
!48 = !{!36, !5, i64 39}
!49 = !{!36, !5, i64 38}
!50 = !{!36, !7, i64 42}
!51 = !{!4, !4, i64 0}
!52 = !{!36, !5, i64 26}
!53 = !{!36, !5, i64 27}
!54 = !{!36, !5, i64 9}
!55 = !{!36, !5, i64 10}
!56 = distinct !{!56, !19, !20}
!57 = distinct !{!57, !19, !20}
!58 = distinct !{!58, !19, !20}
