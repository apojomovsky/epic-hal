; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

%struct.irq_desc_t = type { i8, i8, i8, i8 }
%struct.USART_HandleTypeDef = type { i16, i16, i16, i16, i8, ptr, ptr }

@g_usart = internal unnamed_addr global ptr null, align 2
@irq_table = internal unnamed_addr constant [15 x %struct.irq_desc_t] [%struct.irq_desc_t { i8 1, i8 8, i8 1, i8 0 }, %struct.irq_desc_t { i8 2, i8 16, i8 1, i8 0 }, %struct.irq_desc_t { i8 4, i8 32, i8 1, i8 0 }, %struct.irq_desc_t { i8 1, i8 1, i8 0, i8 0 }, %struct.irq_desc_t { i8 2, i8 2, i8 0, i8 0 }, %struct.irq_desc_t { i8 4, i8 4, i8 0, i8 0 }, %struct.irq_desc_t { i8 1, i8 1, i8 0, i8 1 }, %struct.irq_desc_t { i8 8, i8 8, i8 0, i8 0 }, %struct.irq_desc_t { i8 8, i8 8, i8 0, i8 1 }, %struct.irq_desc_t { i8 16, i8 16, i8 0, i8 0 }, %struct.irq_desc_t { i8 32, i8 32, i8 0, i8 0 }, %struct.irq_desc_t { i8 64, i8 64, i8 0, i8 0 }, %struct.irq_desc_t { i8 16, i8 16, i8 0, i8 1 }, %struct.irq_desc_t { i8 64, i8 64, i8 0, i8 1 }, %struct.irq_desc_t { i8 -128, i8 -128, i8 0, i8 0 }], align 1
@llvm.compiler.used = appending global [1 x ptr] [ptr @PIC16_IRQ_Handler], section "llvm.metadata"
@epic_serial_init.s_usart = internal global %struct.USART_HandleTypeDef zeroinitializer, align 2
@g_tx_count = internal global i8 0, align 1
@g_tx_tail = internal global i8 0, align 1
@g_tx_head = internal global i8 0, align 1
@g_rx_count = internal global i8 0, align 1
@g_rx_tail = internal global i8 0, align 1
@g_rx_head = internal global i8 0, align 1
@g_tx_buf = internal global [32 x i8] zeroinitializer, align 1
@g_rx_buf = internal global [32 x i8] zeroinitializer, align 1
@s_fmt_buf = internal unnamed_addr global [12 x i8] zeroinitializer, align 1
@.str = private unnamed_addr constant [34 x i8] c"epic-serial ready at 115200 8N1\0D\0A\00", align 1
@main.buf = internal global [8 x i8] zeroinitializer, align 1

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

18:                                               ; preds = %6, %4
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
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 10) #9
  %40 = load ptr, ptr %27, align 2, !tbaa !14
  %41 = icmp eq ptr %40, null
  br i1 %41, label %43, label %42

42:                                               ; preds = %15
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  br label %44

43:                                               ; preds = %15
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #9
  br label %44

44:                                               ; preds = %43, %42
  %45 = load ptr, ptr %35, align 2, !tbaa !15
  %46 = icmp eq ptr %45, null
  br i1 %46, label %48, label %47

47:                                               ; preds = %44
  tail call void @EPIC_IRQ_Enable(i16 noundef 10) #9
  br label %49

48:                                               ; preds = %44
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 10) #9
  br label %49

49:                                               ; preds = %48, %47, %1
  %50 = phi i16 [ 4, %1 ], [ 0, %48 ], [ 0, %47 ]
  ret i16 %50
}

; Function Attrs: nounwind
define dso_local noundef i16 @EPIC_USART_DeInit() local_unnamed_addr #1 {
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #9
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 10) #9
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 9) #9
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 10) #9
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
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 9) #9
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_GetTX9D() local_unnamed_addr #2 {
  %1 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  %2 = and i8 %1, 1
  ret i8 %2
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_USART_SetTX9D(i8 noundef zeroext %0) local_unnamed_addr #3 {
  %2 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  %3 = icmp ne i8 %0, 0
  %4 = and i8 %2, -2
  %5 = zext i1 %3 to i8
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_IsTxShiftRegisterEmpty() local_unnamed_addr #2 {
  %1 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !9
  %2 = lshr i8 %1, 1
  %3 = and i8 %2, 1
  ret i8 %3
}

; Function Attrs: nounwind
define dso_local zeroext i8 @EPIC_USART_Receive() local_unnamed_addr #1 {
  %1 = load volatile i8, ptr inttoptr (i16 26 to ptr), align 2, !tbaa !9
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 10) #9
  ret i8 %1
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_GetRX9D() local_unnamed_addr #2 {
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
  tail call void %9() #9
  br label %12

12:                                               ; preds = %11, %7, %0
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
  tail call void %12(i8 noundef zeroext %5) #9
  br label %15

15:                                               ; preds = %14, %10, %4, %0
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local zeroext range(i8 0, 2) i8 @EPIC_IRQ_Disable() local_unnamed_addr #3 {
  %1 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  %2 = lshr i8 %1, 7
  %3 = and i8 %1, 127
  store volatile i8 %3, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  ret i8 %2
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_Restore(i8 noundef zeroext %0) local_unnamed_addr #3 {
  %2 = icmp eq i8 %0, 0
  %3 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  %4 = and i8 %3, 127
  %5 = select i1 %2, i8 0, i8 -128
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_Enable(i16 noundef %0) local_unnamed_addr #3 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %24, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !16
  %7 = getelementptr inbounds nuw i8, ptr %4, i16 1
  %8 = load i8, ptr %7, align 1, !tbaa !18
  %9 = icmp eq i8 %6, 0
  br i1 %9, label %10, label %20

10:                                               ; preds = %3
  %11 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %12 = load i8, ptr %11, align 1, !tbaa !19
  %13 = icmp eq i8 %12, 0
  br i1 %13, label %17, label %14

14:                                               ; preds = %10
  %15 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !9
  %16 = or i8 %15, %8
  store volatile i8 %16, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !9
  br label %20

17:                                               ; preds = %10
  %18 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !9
  %19 = or i8 %18, %8
  store volatile i8 %19, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !9
  br label %20

20:                                               ; preds = %17, %14, %3
  %21 = phi i8 [ %8, %3 ], [ 64, %17 ], [ 64, %14 ]
  %22 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  %23 = or i8 %22, %21
  store volatile i8 %23, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  br label %24

24:                                               ; preds = %20, %1
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_DisableSrc(i16 noundef %0) local_unnamed_addr #3 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %25, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !16
  %7 = getelementptr inbounds nuw i8, ptr %4, i16 1
  %8 = load i8, ptr %7, align 1, !tbaa !18
  %9 = icmp eq i8 %6, 0
  br i1 %9, label %14, label %10

10:                                               ; preds = %3
  %11 = xor i8 %8, -1
  %12 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  %13 = and i8 %12, %11
  store volatile i8 %13, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  br label %25

14:                                               ; preds = %3
  %15 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %16 = load i8, ptr %15, align 1, !tbaa !19
  %17 = icmp eq i8 %16, 0
  %18 = xor i8 %8, -1
  br i1 %17, label %22, label %19

19:                                               ; preds = %14
  %20 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !9
  %21 = and i8 %20, %18
  store volatile i8 %21, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !9
  br label %25

22:                                               ; preds = %14
  %23 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !9
  %24 = and i8 %23, %18
  store volatile i8 %24, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !9
  br label %25

25:                                               ; preds = %22, %19, %10, %1
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_ClearFlag(i16 noundef %0) local_unnamed_addr #3 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %21, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !16
  %7 = load i8, ptr %4, align 1, !tbaa !20
  %8 = icmp eq i8 %6, 0
  br i1 %8, label %13, label %9

9:                                                ; preds = %3
  %10 = xor i8 %7, -1
  %11 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  %12 = and i8 %11, %10
  store volatile i8 %12, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  br label %21

13:                                               ; preds = %3
  %14 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %15 = load i8, ptr %14, align 1, !tbaa !19
  %16 = icmp eq i8 %15, 0
  %17 = select i1 %16, ptr inttoptr (i16 12 to ptr), ptr inttoptr (i16 13 to ptr)
  %18 = load volatile i8, ptr %17, align 1, !tbaa !9
  %19 = xor i8 %7, -1
  %20 = and i8 %18, %19
  store volatile i8 %20, ptr %17, align 1, !tbaa !9
  br label %21

21:                                               ; preds = %13, %9, %1
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_IRQ_GetFlag(i16 noundef %0) local_unnamed_addr #2 {
  %2 = icmp ugt i16 %0, 14
  br i1 %2, label %20, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [15 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !16
  %7 = load i8, ptr %4, align 1, !tbaa !20
  %8 = icmp eq i8 %6, 0
  br i1 %8, label %9, label %14

9:                                                ; preds = %3
  %10 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %11 = load i8, ptr %10, align 1, !tbaa !19
  %12 = icmp eq i8 %11, 0
  %13 = select i1 %12, ptr inttoptr (i16 12 to ptr), ptr inttoptr (i16 13 to ptr)
  br label %14

14:                                               ; preds = %9, %3
  %15 = phi ptr [ %13, %9 ], [ inttoptr (i16 11 to ptr), %3 ]
  %16 = load volatile i8, ptr %15, align 1, !tbaa !9
  %17 = and i8 %16, %7
  %18 = icmp ne i8 %17, 0
  %19 = zext i1 %18 to i8
  br label %20

20:                                               ; preds = %14, %1
  %21 = phi i8 [ %19, %14 ], [ 0, %1 ]
  ret i8 %21
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @EPIC_IRQ_SetPriority(i16 noundef %0, i16 noundef %1) local_unnamed_addr #0 {
  ret void
}

; Function Attrs: noinline nounwind
define dso_local msp430_intrcc void @PIC16_IRQ_Handler() #4 {
  tail call void @epic_dispatch_all_irqs() #9
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_dispatch_all_irqs() local_unnamed_addr #1 {
  %1 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  %2 = zext i8 %1 to i16
  %3 = and i16 %2, 4
  %4 = icmp eq i16 %3, 0
  br i1 %4, label %8, label %5

5:                                                ; preds = %0
  %6 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  %7 = and i8 %6, -5
  store volatile i8 %7, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  br label %8

8:                                                ; preds = %5, %0
  %9 = and i16 %2, 1
  %10 = icmp eq i16 %9, 0
  br i1 %10, label %14, label %11

11:                                               ; preds = %8
  %12 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  %13 = and i8 %12, -2
  store volatile i8 %13, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !9
  br label %14

14:                                               ; preds = %11, %8
  %15 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %16 = zext i8 %15 to i16
  %17 = and i16 %16, 1
  %18 = icmp eq i16 %17, 0
  br i1 %18, label %22, label %19

19:                                               ; preds = %14
  %20 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %21 = and i8 %20, -2
  store volatile i8 %21, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  br label %22

22:                                               ; preds = %19, %14
  %23 = and i16 %16, 2
  %24 = icmp eq i16 %23, 0
  br i1 %24, label %28, label %25

25:                                               ; preds = %22
  %26 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %27 = and i8 %26, -3
  store volatile i8 %27, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  br label %28

28:                                               ; preds = %25, %22
  %29 = and i16 %16, 4
  %30 = icmp eq i16 %29, 0
  br i1 %30, label %34, label %31

31:                                               ; preds = %28
  %32 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %33 = and i8 %32, -5
  store volatile i8 %33, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  br label %34

34:                                               ; preds = %31, %28
  %35 = and i16 %16, 8
  %36 = icmp eq i16 %35, 0
  br i1 %36, label %40, label %37

37:                                               ; preds = %34
  %38 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %39 = and i8 %38, -9
  store volatile i8 %39, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  br label %40

40:                                               ; preds = %37, %34
  %41 = and i16 %16, 32
  %42 = icmp eq i16 %41, 0
  br i1 %42, label %44, label %43

43:                                               ; preds = %40
  tail call void @USART_RX_IRQHandler() #9
  br label %44

44:                                               ; preds = %43, %40
  %45 = and i16 %16, 16
  %46 = icmp eq i16 %45, 0
  br i1 %46, label %55, label %47

47:                                               ; preds = %44
  %48 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !9
  %49 = and i8 %48, 16
  %50 = icmp eq i8 %49, 0
  br i1 %50, label %52, label %51

51:                                               ; preds = %47
  tail call void @USART_TX_IRQHandler() #9
  br label %55

52:                                               ; preds = %47
  %53 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %54 = and i8 %53, -17
  store volatile i8 %54, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  br label %55

55:                                               ; preds = %52, %51, %44
  %56 = and i16 %16, 64
  %57 = icmp eq i16 %56, 0
  br i1 %57, label %61, label %58

58:                                               ; preds = %55
  %59 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %60 = and i8 %59, -65
  store volatile i8 %60, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  br label %61

61:                                               ; preds = %58, %55
  %62 = icmp sgt i8 %15, -1
  br i1 %62, label %66, label %63

63:                                               ; preds = %61
  %64 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  %65 = and i8 %64, 127
  store volatile i8 %65, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !9
  br label %66

66:                                               ; preds = %63, %61
  %67 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !9
  %68 = zext i8 %67 to i16
  %69 = and i16 %68, 1
  %70 = icmp eq i16 %69, 0
  br i1 %70, label %74, label %71

71:                                               ; preds = %66
  %72 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !9
  %73 = and i8 %72, -2
  store volatile i8 %73, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !9
  br label %74

74:                                               ; preds = %71, %66
  %75 = and i16 %68, 8
  %76 = icmp eq i16 %75, 0
  br i1 %76, label %80, label %77

77:                                               ; preds = %74
  %78 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !9
  %79 = and i8 %78, -9
  store volatile i8 %79, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !9
  br label %80

80:                                               ; preds = %77, %74
  %81 = and i16 %68, 16
  %82 = icmp eq i16 %81, 0
  br i1 %82, label %90, label %83

83:                                               ; preds = %80
  %84 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !9
  %85 = and i8 %84, 16
  %86 = icmp eq i8 %85, 0
  br i1 %86, label %87, label %90

87:                                               ; preds = %83
  %88 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !9
  %89 = and i8 %88, -17
  store volatile i8 %89, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !9
  br label %90

90:                                               ; preds = %87, %83, %80
  %91 = and i16 %68, 64
  %92 = icmp eq i16 %91, 0
  br i1 %92, label %96, label %93

93:                                               ; preds = %90
  %94 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !9
  %95 = and i8 %94, -65
  store volatile i8 %95, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !9
  br label %96

96:                                               ; preds = %93, %90
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @epic_harness_init(i32 noundef %0) local_unnamed_addr #0 {
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @epic_harness_tick() local_unnamed_addr #0 {
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i16 @epic_harness_running(i32 noundef %0) local_unnamed_addr #0 {
  ret i16 1
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @epic_harness_log(ptr nocapture noundef readnone %0, ...) local_unnamed_addr #0 {
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_init(i32 noundef %0, i32 noundef %1) local_unnamed_addr #1 {
  %3 = tail call zeroext i16 @USART_ComputeSPBRG(i32 noundef %0, i32 noundef %1, i16 noundef 0, i16 noundef 1) #9
  %4 = trunc i16 %3 to i8
  store i16 0, ptr @epic_serial_init.s_usart, align 2, !tbaa !21
  store i16 1, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 2), align 2, !tbaa !21
  store i16 1, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 4), align 2, !tbaa !21
  store i16 0, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 6), align 2, !tbaa !21
  store i8 %4, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 8), align 2, !tbaa !9
  store i8 0, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 9), align 1
  store ptr @epic_serial_on_tx, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 10), align 2, !tbaa !2
  store ptr @epic_serial_on_rx, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 12), align 2, !tbaa !2
  %5 = tail call i16 @EPIC_USART_Init(ptr noundef nonnull @epic_serial_init.s_usart) #9
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #9
  store volatile i8 0, ptr @g_tx_count, align 1, !tbaa !9
  store volatile i8 0, ptr @g_tx_tail, align 1, !tbaa !9
  store volatile i8 0, ptr @g_tx_head, align 1, !tbaa !9
  store volatile i8 0, ptr @g_rx_count, align 1, !tbaa !9
  store volatile i8 0, ptr @g_rx_tail, align 1, !tbaa !9
  store volatile i8 0, ptr @g_rx_head, align 1, !tbaa !9
  ret void
}

; Function Attrs: nounwind
define internal void @epic_serial_on_tx() #1 {
  %1 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %2 = icmp eq i8 %1, 0
  br i1 %2, label %13, label %3

3:                                                ; preds = %0
  %4 = load volatile i8, ptr @g_tx_tail, align 1, !tbaa !9
  %5 = zext i8 %4 to i16
  %6 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %5
  %7 = load volatile i8, ptr %6, align 1, !tbaa !9
  %8 = load volatile i8, ptr @g_tx_tail, align 1, !tbaa !9
  %9 = add i8 %8, 1
  %10 = and i8 %9, 31
  store volatile i8 %10, ptr @g_tx_tail, align 1, !tbaa !9
  %11 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %12 = add i8 %11, -1
  store volatile i8 %12, ptr @g_tx_count, align 1, !tbaa !9
  store volatile i8 %7, ptr inttoptr (i16 25 to ptr), align 1, !tbaa !9
  br label %14

13:                                               ; preds = %0
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #9
  br label %14

14:                                               ; preds = %13, %3
  ret void
}

; Function Attrs: nofree norecurse nounwind memory(readwrite, argmem: none)
define internal void @epic_serial_on_rx(i8 noundef zeroext %0) #5 {
  %2 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !9
  %3 = icmp ult i8 %2, 32
  br i1 %3, label %4, label %13

4:                                                ; preds = %1
  %5 = load volatile i8, ptr @g_rx_head, align 1, !tbaa !9
  %6 = zext i8 %5 to i16
  %7 = getelementptr inbounds nuw [32 x i8], ptr @g_rx_buf, i16 0, i16 %6
  store volatile i8 %0, ptr %7, align 1, !tbaa !9
  %8 = load volatile i8, ptr @g_rx_head, align 1, !tbaa !9
  %9 = add i8 %8, 1
  %10 = and i8 %9, 31
  store volatile i8 %10, ptr @g_rx_head, align 1, !tbaa !9
  %11 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !9
  %12 = add i8 %11, 1
  store volatile i8 %12, ptr @g_rx_count, align 1, !tbaa !9
  br label %13

13:                                               ; preds = %4, %1
  ret void
}

; Function Attrs: nounwind
define dso_local noundef i16 @epic_serial_write(ptr nocapture noundef readonly %0, i16 noundef returned %1) local_unnamed_addr #1 {
  %3 = icmp sgt i16 %1, 0
  br i1 %3, label %4, label %8

4:                                                ; preds = %12, %2
  %5 = phi i16 [ %23, %12 ], [ 0, %2 ]
  %6 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %7 = icmp ugt i8 %6, 31
  br i1 %7, label %9, label %12

8:                                                ; preds = %12, %2
  ret i16 %1

9:                                                ; preds = %9, %4
  tail call void @epic_dispatch_all_irqs() #9
  %10 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %11 = icmp ugt i8 %10, 31
  br i1 %11, label %9, label %12, !llvm.loop !22

12:                                               ; preds = %9, %4
  %13 = getelementptr inbounds nuw i8, ptr %0, i16 %5
  %14 = load i8, ptr %13, align 1, !tbaa !9
  %15 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %16 = zext i8 %15 to i16
  %17 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %16
  store volatile i8 %14, ptr %17, align 1, !tbaa !9
  %18 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %19 = add i8 %18, 1
  %20 = and i8 %19, 31
  store volatile i8 %20, ptr @g_tx_head, align 1, !tbaa !9
  %21 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %22 = add i8 %21, 1
  store volatile i8 %22, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  %23 = add nuw nsw i16 %5, 1
  %24 = icmp eq i16 %23, %1
  br i1 %24, label %8, label %4, !llvm.loop !25
}

; Function Attrs: nofree norecurse nounwind memory(readwrite, argmem: write)
define dso_local i16 @epic_serial_read(ptr nocapture noundef writeonly %0, i16 noundef %1) local_unnamed_addr #6 {
  %3 = icmp sgt i16 %1, 0
  br i1 %3, label %4, label %21

4:                                                ; preds = %8, %2
  %5 = phi i16 [ %13, %8 ], [ 0, %2 ]
  %6 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !9
  %7 = icmp eq i8 %6, 0
  br i1 %7, label %21, label %8

8:                                                ; preds = %4
  %9 = load volatile i8, ptr @g_rx_tail, align 1, !tbaa !9
  %10 = zext i8 %9 to i16
  %11 = getelementptr inbounds nuw [32 x i8], ptr @g_rx_buf, i16 0, i16 %10
  %12 = load volatile i8, ptr %11, align 1, !tbaa !9
  %13 = add nuw nsw i16 %5, 1
  %14 = getelementptr inbounds nuw i8, ptr %0, i16 %5
  store i8 %12, ptr %14, align 1, !tbaa !9
  %15 = load volatile i8, ptr @g_rx_tail, align 1, !tbaa !9
  %16 = add i8 %15, 1
  %17 = and i8 %16, 31
  store volatile i8 %17, ptr @g_rx_tail, align 1, !tbaa !9
  %18 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !9
  %19 = add i8 %18, -1
  store volatile i8 %19, ptr @g_rx_count, align 1, !tbaa !9
  %20 = icmp eq i16 %13, %1
  br i1 %20, label %21, label %4, !llvm.loop !26

21:                                               ; preds = %8, %4, %2
  %22 = phi i16 [ 0, %2 ], [ %5, %4 ], [ %1, %8 ]
  ret i16 %22
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn memory(readwrite, argmem: none)
define dso_local range(i16 0, 256) i16 @epic_serial_available() local_unnamed_addr #7 {
  %1 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !9
  %2 = zext i8 %1 to i16
  ret i16 %2
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn memory(readwrite, argmem: none)
define dso_local range(i16 0, 256) i16 @epic_serial_tx_pending() local_unnamed_addr #7 {
  %1 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %2 = zext i8 %1 to i16
  ret i16 %2
}

; Function Attrs: nounwind
define dso_local void @epic_serial_flush() local_unnamed_addr #1 {
  %1 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %2 = icmp eq i8 %1, 0
  br i1 %2, label %3, label %6

3:                                                ; preds = %6, %0
  %4 = tail call zeroext i8 @EPIC_USART_IsTxShiftRegisterEmpty() #9
  %5 = icmp eq i8 %4, 0
  br i1 %5, label %9, label %12

6:                                                ; preds = %6, %0
  tail call void @epic_dispatch_all_irqs() #9
  %7 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %8 = icmp eq i8 %7, 0
  br i1 %8, label %3, label %6, !llvm.loop !27

9:                                                ; preds = %9, %3
  tail call void @epic_dispatch_all_irqs() #9
  %10 = tail call zeroext i8 @EPIC_USART_IsTxShiftRegisterEmpty() #9
  %11 = icmp eq i8 %10, 0
  br i1 %11, label %9, label %12, !llvm.loop !28

12:                                               ; preds = %9, %3
  ret void
}

; Function Attrs: nounwind
define dso_local void @putch(i8 noundef zeroext %0) local_unnamed_addr #1 {
  %2 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %3 = icmp ugt i8 %2, 31
  br i1 %3, label %4, label %7

4:                                                ; preds = %4, %1
  tail call void @epic_dispatch_all_irqs() #9
  %5 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %6 = icmp ugt i8 %5, 31
  br i1 %6, label %4, label %7, !llvm.loop !22

7:                                                ; preds = %4, %1
  %8 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %9 = zext i8 %8 to i16
  %10 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %9
  store volatile i8 %0, ptr %10, align 1, !tbaa !9
  %11 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %12 = add i8 %11, 1
  %13 = and i8 %12, 31
  store volatile i8 %13, ptr @g_tx_head, align 1, !tbaa !9
  %14 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %15 = add i8 %14, 1
  store volatile i8 %15, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_char(i8 noundef zeroext %0) local_unnamed_addr #1 {
  %2 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %3 = icmp ugt i8 %2, 31
  br i1 %3, label %4, label %7

4:                                                ; preds = %4, %1
  tail call void @epic_dispatch_all_irqs() #9
  %5 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %6 = icmp ugt i8 %5, 31
  br i1 %6, label %4, label %7, !llvm.loop !22

7:                                                ; preds = %4, %1
  %8 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %9 = zext i8 %8 to i16
  %10 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %9
  store volatile i8 %0, ptr %10, align 1, !tbaa !9
  %11 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %12 = add i8 %11, 1
  %13 = and i8 %12, 31
  store volatile i8 %13, ptr @g_tx_head, align 1, !tbaa !9
  %14 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %15 = add i8 %14, 1
  store volatile i8 %15, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_str(ptr nocapture noundef readonly %0) local_unnamed_addr #1 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi i16 [ 0, %1 ], [ %7, %2 ]
  %4 = getelementptr inbounds nuw i8, ptr %0, i16 %3
  %5 = load i8, ptr %4, align 1, !tbaa !9
  %6 = icmp eq i8 %5, 0
  %7 = add nuw nsw i16 %3, 1
  br i1 %6, label %8, label %2, !llvm.loop !29

8:                                                ; preds = %2
  %9 = icmp eq i16 %3, 0
  br i1 %9, label %30, label %10

10:                                               ; preds = %17, %8
  %11 = phi i16 [ %28, %17 ], [ 0, %8 ]
  %12 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %13 = icmp ugt i8 %12, 31
  br i1 %13, label %14, label %17

14:                                               ; preds = %14, %10
  tail call void @epic_dispatch_all_irqs() #9
  %15 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %16 = icmp ugt i8 %15, 31
  br i1 %16, label %14, label %17, !llvm.loop !22

17:                                               ; preds = %14, %10
  %18 = getelementptr inbounds nuw i8, ptr %0, i16 %11
  %19 = load i8, ptr %18, align 1, !tbaa !9
  %20 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %21 = zext i8 %20 to i16
  %22 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %21
  store volatile i8 %19, ptr %22, align 1, !tbaa !9
  %23 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %24 = add i8 %23, 1
  %25 = and i8 %24, 31
  store volatile i8 %25, ptr @g_tx_head, align 1, !tbaa !9
  %26 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %27 = add i8 %26, 1
  store volatile i8 %27, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  %28 = add nuw nsw i16 %11, 1
  %29 = icmp eq i16 %28, %3
  br i1 %29, label %30, label %10, !llvm.loop !25

30:                                               ; preds = %17, %8
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_u16(i16 noundef zeroext %0) local_unnamed_addr #1 {
  %2 = zext i16 %0 to i32
  br label %3

3:                                                ; preds = %3, %1
  %4 = phi i8 [ %17, %3 ], [ 1, %1 ]
  %5 = phi i32 [ %8, %3 ], [ %2, %1 ]
  %6 = phi i8 [ %15, %3 ], [ 0, %1 ]
  %7 = freeze i32 %5
  %8 = udiv i32 %7, 10
  %9 = mul i32 %8, 10
  %10 = sub i32 %7, %9
  %11 = trunc nuw nsw i32 %10 to i8
  %12 = or disjoint i8 %11, 48
  %13 = zext i8 %6 to i16
  %14 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %13
  store i8 %12, ptr %14, align 1, !tbaa !9
  %15 = add i8 %6, 1
  %16 = icmp samesign ult i32 %5, 10
  %17 = add i8 %4, 1
  br i1 %16, label %18, label %3, !llvm.loop !30

18:                                               ; preds = %3
  %19 = icmp eq i8 %15, 0
  br i1 %19, label %42, label %20

20:                                               ; preds = %18
  %21 = zext i8 %4 to i16
  br label %22

22:                                               ; preds = %32, %20
  %23 = phi i16 [ %21, %20 ], [ %24, %32 ]
  %24 = add nsw i16 %23, -1
  %25 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %24
  %26 = load i8, ptr %25, align 1, !tbaa !9
  %27 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %28 = icmp ugt i8 %27, 31
  br i1 %28, label %29, label %32

29:                                               ; preds = %29, %22
  tail call void @epic_dispatch_all_irqs() #9
  %30 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %31 = icmp ugt i8 %30, 31
  br i1 %31, label %29, label %32, !llvm.loop !22

32:                                               ; preds = %29, %22
  %33 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %34 = zext i8 %33 to i16
  %35 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %34
  store volatile i8 %26, ptr %35, align 1, !tbaa !9
  %36 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %37 = add i8 %36, 1
  %38 = and i8 %37, 31
  store volatile i8 %38, ptr @g_tx_head, align 1, !tbaa !9
  %39 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %40 = add i8 %39, 1
  store volatile i8 %40, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  %41 = icmp eq i16 %24, 0
  br i1 %41, label %42, label %22, !llvm.loop !31

42:                                               ; preds = %32, %18
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_u32(i32 noundef %0) local_unnamed_addr #1 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi i8 [ %16, %2 ], [ 1, %1 ]
  %4 = phi i32 [ %7, %2 ], [ %0, %1 ]
  %5 = phi i8 [ %14, %2 ], [ 0, %1 ]
  %6 = freeze i32 %4
  %7 = udiv i32 %6, 10
  %8 = mul i32 %7, 10
  %9 = sub i32 %6, %8
  %10 = trunc nuw nsw i32 %9 to i8
  %11 = or disjoint i8 %10, 48
  %12 = zext i8 %5 to i16
  %13 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %12
  store i8 %11, ptr %13, align 1, !tbaa !9
  %14 = add i8 %5, 1
  %15 = icmp ult i32 %4, 10
  %16 = add i8 %3, 1
  br i1 %15, label %17, label %2, !llvm.loop !30

17:                                               ; preds = %2
  %18 = icmp eq i8 %14, 0
  br i1 %18, label %41, label %19

19:                                               ; preds = %17
  %20 = zext i8 %3 to i16
  br label %21

21:                                               ; preds = %31, %19
  %22 = phi i16 [ %20, %19 ], [ %23, %31 ]
  %23 = add nsw i16 %22, -1
  %24 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %23
  %25 = load i8, ptr %24, align 1, !tbaa !9
  %26 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %27 = icmp ugt i8 %26, 31
  br i1 %27, label %28, label %31

28:                                               ; preds = %28, %21
  tail call void @epic_dispatch_all_irqs() #9
  %29 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %30 = icmp ugt i8 %29, 31
  br i1 %30, label %28, label %31, !llvm.loop !22

31:                                               ; preds = %28, %21
  %32 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %33 = zext i8 %32 to i16
  %34 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %33
  store volatile i8 %25, ptr %34, align 1, !tbaa !9
  %35 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %36 = add i8 %35, 1
  %37 = and i8 %36, 31
  store volatile i8 %37, ptr @g_tx_head, align 1, !tbaa !9
  %38 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %39 = add i8 %38, 1
  store volatile i8 %39, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  %40 = icmp eq i16 %23, 0
  br i1 %40, label %41, label %21, !llvm.loop !31

41:                                               ; preds = %31, %17
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_i16(i16 noundef signext %0) local_unnamed_addr #1 {
  %2 = sext i16 %0 to i32
  tail call fastcc void @epic_serial_put_idec(i32 noundef %2) #10
  ret void
}

; Function Attrs: nounwind
define internal fastcc void @epic_serial_put_idec(i32 noundef %0) unnamed_addr #1 {
  %2 = icmp slt i32 %0, 0
  br i1 %2, label %3, label %58

3:                                                ; preds = %1
  %4 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %5 = icmp ugt i8 %4, 31
  br i1 %5, label %6, label %9

6:                                                ; preds = %6, %3
  tail call void @epic_dispatch_all_irqs() #9
  %7 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %8 = icmp ugt i8 %7, 31
  br i1 %8, label %6, label %9, !llvm.loop !22

9:                                                ; preds = %6, %3
  %10 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %11 = zext i8 %10 to i16
  %12 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %11
  store volatile i8 45, ptr %12, align 1, !tbaa !9
  %13 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %14 = add i8 %13, 1
  %15 = and i8 %14, 31
  store volatile i8 %15, ptr @g_tx_head, align 1, !tbaa !9
  %16 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %17 = add i8 %16, 1
  store volatile i8 %17, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  %18 = sub i32 0, %0
  br label %19

19:                                               ; preds = %19, %9
  %20 = phi i8 [ %33, %19 ], [ 1, %9 ]
  %21 = phi i32 [ %24, %19 ], [ %18, %9 ]
  %22 = phi i8 [ %31, %19 ], [ 0, %9 ]
  %23 = freeze i32 %21
  %24 = udiv i32 %23, 10
  %25 = mul i32 %24, 10
  %26 = sub i32 %23, %25
  %27 = trunc nuw nsw i32 %26 to i8
  %28 = or disjoint i8 %27, 48
  %29 = zext i8 %22 to i16
  %30 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %29
  store i8 %28, ptr %30, align 1, !tbaa !9
  %31 = add i8 %22, 1
  %32 = icmp ult i32 %21, 10
  %33 = add i8 %20, 1
  br i1 %32, label %34, label %19, !llvm.loop !30

34:                                               ; preds = %19
  %35 = icmp eq i8 %31, 0
  br i1 %35, label %97, label %36

36:                                               ; preds = %34
  %37 = zext i8 %20 to i16
  br label %38

38:                                               ; preds = %48, %36
  %39 = phi i16 [ %37, %36 ], [ %40, %48 ]
  %40 = add nsw i16 %39, -1
  %41 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %40
  %42 = load i8, ptr %41, align 1, !tbaa !9
  %43 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %44 = icmp ugt i8 %43, 31
  br i1 %44, label %45, label %48

45:                                               ; preds = %45, %38
  tail call void @epic_dispatch_all_irqs() #9
  %46 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %47 = icmp ugt i8 %46, 31
  br i1 %47, label %45, label %48, !llvm.loop !22

48:                                               ; preds = %45, %38
  %49 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %50 = zext i8 %49 to i16
  %51 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %50
  store volatile i8 %42, ptr %51, align 1, !tbaa !9
  %52 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %53 = add i8 %52, 1
  %54 = and i8 %53, 31
  store volatile i8 %54, ptr @g_tx_head, align 1, !tbaa !9
  %55 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %56 = add i8 %55, 1
  store volatile i8 %56, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  %57 = icmp eq i16 %40, 0
  br i1 %57, label %97, label %38, !llvm.loop !31

58:                                               ; preds = %58, %1
  %59 = phi i8 [ %72, %58 ], [ 1, %1 ]
  %60 = phi i32 [ %63, %58 ], [ %0, %1 ]
  %61 = phi i8 [ %70, %58 ], [ 0, %1 ]
  %62 = freeze i32 %60
  %63 = udiv i32 %62, 10
  %64 = mul i32 %63, 10
  %65 = sub i32 %62, %64
  %66 = trunc nuw nsw i32 %65 to i8
  %67 = or disjoint i8 %66, 48
  %68 = zext i8 %61 to i16
  %69 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %68
  store i8 %67, ptr %69, align 1, !tbaa !9
  %70 = add i8 %61, 1
  %71 = icmp samesign ult i32 %60, 10
  %72 = add i8 %59, 1
  br i1 %71, label %73, label %58, !llvm.loop !30

73:                                               ; preds = %58
  %74 = icmp eq i8 %70, 0
  br i1 %74, label %97, label %75

75:                                               ; preds = %73
  %76 = zext i8 %59 to i16
  br label %77

77:                                               ; preds = %87, %75
  %78 = phi i16 [ %76, %75 ], [ %79, %87 ]
  %79 = add nsw i16 %78, -1
  %80 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %79
  %81 = load i8, ptr %80, align 1, !tbaa !9
  %82 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %83 = icmp ugt i8 %82, 31
  br i1 %83, label %84, label %87

84:                                               ; preds = %84, %77
  tail call void @epic_dispatch_all_irqs() #9
  %85 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %86 = icmp ugt i8 %85, 31
  br i1 %86, label %84, label %87, !llvm.loop !22

87:                                               ; preds = %84, %77
  %88 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %89 = zext i8 %88 to i16
  %90 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %89
  store volatile i8 %81, ptr %90, align 1, !tbaa !9
  %91 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %92 = add i8 %91, 1
  %93 = and i8 %92, 31
  store volatile i8 %93, ptr @g_tx_head, align 1, !tbaa !9
  %94 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %95 = add i8 %94, 1
  store volatile i8 %95, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  %96 = icmp eq i16 %79, 0
  br i1 %96, label %97, label %77, !llvm.loop !31

97:                                               ; preds = %87, %73, %48, %34
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_i32(i32 noundef %0) local_unnamed_addr #1 {
  tail call fastcc void @epic_serial_put_idec(i32 noundef %0) #10
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_hex8(i8 noundef zeroext %0) local_unnamed_addr #1 {
  %2 = zext i8 %0 to i32
  br label %3

3:                                                ; preds = %21, %1
  %4 = phi i16 [ 2, %1 ], [ %5, %21 ]
  %5 = add nsw i16 %4, -1
  %6 = shl nuw nsw i16 %5, 2
  %7 = zext nneg i16 %6 to i32
  %8 = lshr i32 %2, %7
  %9 = trunc nuw nsw i32 %8 to i16
  %10 = and i16 %9, 15
  %11 = icmp samesign ult i16 %10, 10
  %12 = or disjoint i16 %10, 48
  %13 = add nuw nsw i16 %10, 55
  %14 = select i1 %11, i16 %12, i16 %13
  %15 = trunc nuw nsw i16 %14 to i8
  %16 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %17 = icmp ugt i8 %16, 31
  br i1 %17, label %18, label %21

18:                                               ; preds = %18, %3
  tail call void @epic_dispatch_all_irqs() #9
  %19 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %20 = icmp ugt i8 %19, 31
  br i1 %20, label %18, label %21, !llvm.loop !22

21:                                               ; preds = %18, %3
  %22 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %23 = zext i8 %22 to i16
  %24 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %23
  store volatile i8 %15, ptr %24, align 1, !tbaa !9
  %25 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %26 = add i8 %25, 1
  %27 = and i8 %26, 31
  store volatile i8 %27, ptr @g_tx_head, align 1, !tbaa !9
  %28 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %29 = add i8 %28, 1
  store volatile i8 %29, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  %30 = icmp ugt i16 %4, 1
  br i1 %30, label %3, label %31, !llvm.loop !32

31:                                               ; preds = %21
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_hex16(i16 noundef zeroext %0) local_unnamed_addr #1 {
  %2 = zext i16 %0 to i32
  br label %3

3:                                                ; preds = %21, %1
  %4 = phi i16 [ 4, %1 ], [ %5, %21 ]
  %5 = add nsw i16 %4, -1
  %6 = shl nuw nsw i16 %5, 2
  %7 = zext nneg i16 %6 to i32
  %8 = lshr i32 %2, %7
  %9 = trunc nuw i32 %8 to i16
  %10 = and i16 %9, 15
  %11 = icmp samesign ult i16 %10, 10
  %12 = or disjoint i16 %10, 48
  %13 = add nuw nsw i16 %10, 55
  %14 = select i1 %11, i16 %12, i16 %13
  %15 = trunc nuw nsw i16 %14 to i8
  %16 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %17 = icmp ugt i8 %16, 31
  br i1 %17, label %18, label %21

18:                                               ; preds = %18, %3
  tail call void @epic_dispatch_all_irqs() #9
  %19 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %20 = icmp ugt i8 %19, 31
  br i1 %20, label %18, label %21, !llvm.loop !22

21:                                               ; preds = %18, %3
  %22 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %23 = zext i8 %22 to i16
  %24 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %23
  store volatile i8 %15, ptr %24, align 1, !tbaa !9
  %25 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !9
  %26 = add i8 %25, 1
  %27 = and i8 %26, 31
  store volatile i8 %27, ptr @g_tx_head, align 1, !tbaa !9
  %28 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !9
  %29 = add i8 %28, 1
  store volatile i8 %29, ptr @g_tx_count, align 1, !tbaa !9
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #9
  %30 = icmp ugt i16 %4, 1
  br i1 %30, label %3, label %31, !llvm.loop !32

31:                                               ; preds = %21
  ret void
}

; Function Attrs: noreturn nounwind
define dso_local noundef i16 @main() local_unnamed_addr #8 {
  tail call void @epic_serial_init(i32 noundef 20000000, i32 noundef 115200) #9
  tail call void @EPIC_IRQ_Restore(i8 noundef zeroext 1) #9
  tail call void @epic_serial_put_str(ptr noundef nonnull @.str) #9
  tail call void @epic_serial_flush() #9
  br label %1

1:                                                ; preds = %7, %0
  %2 = tail call i16 @epic_serial_available() #9
  %3 = icmp sgt i16 %2, 0
  br i1 %3, label %4, label %7

4:                                                ; preds = %1
  %5 = tail call i16 @epic_serial_read(ptr noundef nonnull @main.buf, i16 noundef 8) #9
  %6 = icmp sgt i16 %5, 0
  br i1 %6, label %8, label %7

7:                                                ; preds = %8, %4, %1
  br label %1, !llvm.loop !33

8:                                                ; preds = %4
  %9 = tail call i16 @epic_serial_write(ptr noundef nonnull @main.buf, i16 noundef %5) #9
  br label %7
}

attributes #0 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { mustprogress nofree norecurse nounwind willreturn "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { nofree norecurse nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #4 = { noinline nounwind "interrupt"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #5 = { nofree norecurse nounwind memory(readwrite, argmem: none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #6 = { nofree norecurse nounwind memory(readwrite, argmem: write) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #7 = { mustprogress nofree norecurse nounwind willreturn memory(readwrite, argmem: none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #8 = { noreturn nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #9 = { nobuiltin nounwind "no-builtins" }
attributes #10 = { nobuiltin "no-builtins" }

!llvm.ident = !{!0, !0, !0, !0, !0, !0, !0}
!llvm.module.flags = !{!1}

!0 = !{!"clang version 20.1.8"}
!1 = !{i32 1, !"wchar_size", i32 2}
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
!16 = !{!17, !4, i64 2}
!17 = !{!"", !4, i64 0, !4, i64 1, !4, i64 2, !4, i64 3}
!18 = !{!17, !4, i64 1}
!19 = !{!17, !4, i64 3}
!20 = !{!17, !4, i64 0}
!21 = !{!8, !8, i64 0}
!22 = distinct !{!22, !23, !24}
!23 = !{!"llvm.loop.mustprogress"}
!24 = !{!"llvm.loop.unroll.disable"}
!25 = distinct !{!25, !23, !24}
!26 = distinct !{!26, !23, !24}
!27 = distinct !{!27, !23, !24}
!28 = distinct !{!28, !23, !24}
!29 = distinct !{!29, !23, !24}
!30 = distinct !{!30, !23, !24}
!31 = distinct !{!31, !23, !24}
!32 = distinct !{!32, !23, !24}
!33 = distinct !{!33, !24}
