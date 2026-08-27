; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

%struct.irq_desc_t = type { i8, i8, i8, i8 }
%struct.USART_HandleTypeDef = type { i16, i16, i16, i16, i16, i8, ptr, ptr }

@irq_table = internal unnamed_addr constant [17 x %struct.irq_desc_t] [%struct.irq_desc_t { i8 1, i8 8, i8 1, i8 0 }, %struct.irq_desc_t { i8 2, i8 16, i8 1, i8 0 }, %struct.irq_desc_t { i8 4, i8 32, i8 1, i8 0 }, %struct.irq_desc_t { i8 1, i8 1, i8 0, i8 0 }, %struct.irq_desc_t { i8 2, i8 2, i8 0, i8 0 }, %struct.irq_desc_t { i8 4, i8 4, i8 0, i8 0 }, %struct.irq_desc_t { i8 1, i8 1, i8 0, i8 1 }, %struct.irq_desc_t { i8 8, i8 8, i8 0, i8 0 }, %struct.irq_desc_t { i8 8, i8 8, i8 0, i8 1 }, %struct.irq_desc_t { i8 16, i8 16, i8 0, i8 0 }, %struct.irq_desc_t { i8 32, i8 32, i8 0, i8 0 }, %struct.irq_desc_t { i8 64, i8 64, i8 0, i8 0 }, %struct.irq_desc_t { i8 16, i8 16, i8 0, i8 1 }, %struct.irq_desc_t { i8 32, i8 32, i8 0, i8 1 }, %struct.irq_desc_t { i8 64, i8 64, i8 0, i8 1 }, %struct.irq_desc_t { i8 4, i8 4, i8 0, i8 1 }, %struct.irq_desc_t { i8 -128, i8 -128, i8 0, i8 1 }], align 1
@llvm.compiler.used = appending global [1 x ptr] [ptr @PIC16_IRQ_Handler], section "llvm.metadata"
@epic_serial_init.s_usart = internal unnamed_addr global %struct.USART_HandleTypeDef zeroinitializer, align 2
@g_tx_count = internal global i8 0, align 1
@g_tx_tail = internal global i8 0, align 1
@g_tx_head = internal global i8 0, align 1
@g_rx_count = internal global i8 0, align 1
@g_rx_tail = internal global i8 0, align 1
@g_rx_head = internal global i8 0, align 1
@g_tx_buf = internal global [32 x i8] zeroinitializer, align 1
@g_rx_buf = internal global [32 x i8] zeroinitializer, align 1
@g_usart_tx_cb = dso_local local_unnamed_addr global ptr null, align 2
@g_usart_rx_cb = dso_local local_unnamed_addr global ptr null, align 2
@s_fmt_buf = internal unnamed_addr global [12 x i8] zeroinitializer, align 1
@main.banner = internal unnamed_addr constant [34 x i8] c"epic-serial ready at 115200 8N1\0D\0A\00", align 1
@main.buf = internal global [8 x i8] zeroinitializer, align 1

; Function Attrs: nofree norecurse nounwind memory(inaccessiblemem: readwrite)
define dso_local zeroext i16 @USART_ComputeSPBRG(i32 noundef %0, i32 noundef %1, i16 noundef %2, i16 noundef %3) local_unnamed_addr #0 {
  %5 = alloca i32, align 2
  %6 = icmp eq i16 %2, 1
  %7 = icmp eq i16 %3, 1
  %8 = select i1 %7, i32 4, i32 6
  %9 = select i1 %6, i32 2, i32 %8
  %10 = shl i32 %1, %9
  %11 = icmp ugt i32 %10, %0
  br i1 %11, label %15, label %12

12:                                               ; preds = %4
  %13 = udiv i32 %0, %10
  %14 = add i32 %13, -1
  br label %15

15:                                               ; preds = %12, %4
  %16 = phi i32 [ %14, %12 ], [ 0, %4 ]
  call void @llvm.lifetime.start.p0(i64 4, ptr nonnull %5)
  store volatile i32 %16, ptr %5, align 2, !tbaa !2
  %17 = load volatile i32, ptr %5, align 2, !tbaa !2
  %18 = icmp ugt i32 %17, 255
  br i1 %18, label %22, label %19

19:                                               ; preds = %15
  %20 = load volatile i32, ptr %5, align 2, !tbaa !2
  %21 = trunc i32 %20 to i16
  br label %22

22:                                               ; preds = %19, %15
  %23 = phi i16 [ %21, %19 ], [ -1, %15 ]
  call void @llvm.lifetime.end.p0(i64 4, ptr nonnull %5)
  ret i16 %23
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #1

; Function Attrs: nounwind
define dso_local noundef i16 @EPIC_USART_DeInit() local_unnamed_addr #2 {
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #11
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 10) #11
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 9) #11
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 10) #11
  store volatile i8 0, ptr inttoptr (i16 24 to ptr), align 8, !tbaa !6
  store volatile i8 2, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !6
  store volatile i8 0, ptr inttoptr (i16 391 to ptr), align 1, !tbaa !6
  %1 = load volatile i8, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !6
  %2 = load volatile i8, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !6
  %3 = and i8 %2, -97
  %4 = or disjoint i8 %3, 32
  store volatile i8 %4, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !6
  store volatile i8 0, ptr inttoptr (i16 154 to ptr), align 2, !tbaa !6
  store volatile i8 0, ptr inttoptr (i16 153 to ptr), align 1, !tbaa !6
  %5 = load volatile i8, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !6
  %6 = and i8 %5, -97
  %7 = and i8 %1, 96
  %8 = or disjoint i8 %6, %7
  store volatile i8 %8, ptr inttoptr (i16 3 to ptr), align 1, !tbaa !6
  store ptr null, ptr @g_usart_tx_cb, align 2, !tbaa !7
  store ptr null, ptr @g_usart_rx_cb, align 2, !tbaa !7
  ret i16 0
}

; Function Attrs: nounwind
define dso_local void @EPIC_USART_Transmit(i8 noundef zeroext %0) local_unnamed_addr #2 {
  store volatile i8 %0, ptr inttoptr (i16 25 to ptr), align 1, !tbaa !6
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 9) #11
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_GetTX9D() local_unnamed_addr #3 {
  %1 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !6
  %2 = and i8 %1, 1
  ret i8 %2
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_USART_SetTX9D(i8 noundef zeroext %0) local_unnamed_addr #4 {
  %2 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !6
  %3 = icmp ne i8 %0, 0
  %4 = and i8 %2, -2
  %5 = zext i1 %3 to i8
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !6
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_IsTxShiftRegisterEmpty() local_unnamed_addr #3 {
  %1 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !6
  %2 = lshr i8 %1, 1
  %3 = and i8 %2, 1
  ret i8 %3
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_USART_SendBreak() local_unnamed_addr #4 {
  %1 = load volatile i8, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !6
  %2 = or i8 %1, 8
  store volatile i8 %2, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !6
  ret void
}

; Function Attrs: nounwind
define dso_local zeroext i8 @EPIC_USART_Receive() local_unnamed_addr #2 {
  %1 = load volatile i8, ptr inttoptr (i16 26 to ptr), align 2, !tbaa !6
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 10) #11
  ret i8 %1
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_GetRX9D() local_unnamed_addr #3 {
  %1 = load volatile i8, ptr inttoptr (i16 24 to ptr), align 8, !tbaa !6
  %2 = and i8 %1, 1
  ret i8 %2
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_USART_SetAutoBaud(i8 noundef zeroext %0) local_unnamed_addr #4 {
  %2 = load volatile i8, ptr inttoptr (i16 391 to ptr), align 1, !tbaa !6
  %3 = icmp ne i8 %0, 0
  %4 = and i8 %2, -2
  %5 = zext i1 %3 to i8
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 391 to ptr), align 1, !tbaa !6
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_USART_GetAutoBaudOverflow() local_unnamed_addr #3 {
  %1 = load volatile i8, ptr inttoptr (i16 391 to ptr), align 1, !tbaa !6
  %2 = lshr i8 %1, 7
  ret i8 %2
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_USART_SetWakeUp(i8 noundef zeroext %0) local_unnamed_addr #4 {
  %2 = load volatile i8, ptr inttoptr (i16 391 to ptr), align 1, !tbaa !6
  %3 = icmp eq i8 %0, 0
  %4 = and i8 %2, -3
  %5 = select i1 %3, i8 0, i8 2
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 391 to ptr), align 1, !tbaa !6
  ret void
}

; Function Attrs: nounwind
define dso_local void @USART_TX_IRQHandler() local_unnamed_addr #2 {
  %1 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %2 = and i8 %1, 16
  %3 = icmp ne i8 %2, 0
  %4 = load ptr, ptr @g_usart_tx_cb, align 2
  %5 = icmp ne ptr %4, null
  %6 = select i1 %3, i1 %5, i1 false
  br i1 %6, label %7, label %8

7:                                                ; preds = %0
  tail call void %4() #11
  br label %8

8:                                                ; preds = %7, %0
  ret void
}

; Function Attrs: nounwind
define dso_local void @USART_RX_IRQHandler() local_unnamed_addr #2 {
  %1 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %2 = and i8 %1, 32
  %3 = icmp eq i8 %2, 0
  br i1 %3, label %11, label %4

4:                                                ; preds = %0
  %5 = load volatile i8, ptr inttoptr (i16 26 to ptr), align 2, !tbaa !6
  %6 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %7 = and i8 %6, -33
  store volatile i8 %7, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %8 = load ptr, ptr @g_usart_rx_cb, align 2, !tbaa !7
  %9 = icmp eq ptr %8, null
  br i1 %9, label %11, label %10

10:                                               ; preds = %4
  tail call void %8(i8 noundef zeroext %5) #11
  br label %11

11:                                               ; preds = %10, %4, %0
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local zeroext range(i8 0, 2) i8 @EPIC_IRQ_Disable() local_unnamed_addr #4 {
  %1 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  %2 = lshr i8 %1, 7
  %3 = and i8 %1, 127
  store volatile i8 %3, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  ret i8 %2
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_Restore(i8 noundef zeroext %0) local_unnamed_addr #4 {
  %2 = icmp eq i8 %0, 0
  %3 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  %4 = and i8 %3, 127
  %5 = select i1 %2, i8 0, i8 -128
  %6 = or disjoint i8 %4, %5
  store volatile i8 %6, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_Enable(i16 noundef %0) local_unnamed_addr #4 {
  %2 = icmp ugt i16 %0, 16
  br i1 %2, label %24, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [17 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !9
  %7 = getelementptr inbounds nuw i8, ptr %4, i16 1
  %8 = load i8, ptr %7, align 1, !tbaa !11
  %9 = icmp eq i8 %6, 0
  br i1 %9, label %10, label %20

10:                                               ; preds = %3
  %11 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %12 = load i8, ptr %11, align 1, !tbaa !12
  %13 = icmp eq i8 %12, 0
  br i1 %13, label %17, label %14

14:                                               ; preds = %10
  %15 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !6
  %16 = or i8 %15, %8
  store volatile i8 %16, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !6
  br label %20

17:                                               ; preds = %10
  %18 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !6
  %19 = or i8 %18, %8
  store volatile i8 %19, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !6
  br label %20

20:                                               ; preds = %17, %14, %3
  %21 = phi i8 [ %8, %3 ], [ 64, %17 ], [ 64, %14 ]
  %22 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  %23 = or i8 %22, %21
  store volatile i8 %23, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  br label %24

24:                                               ; preds = %20, %1
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_DisableSrc(i16 noundef %0) local_unnamed_addr #4 {
  %2 = icmp ugt i16 %0, 16
  br i1 %2, label %25, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [17 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !9
  %7 = getelementptr inbounds nuw i8, ptr %4, i16 1
  %8 = load i8, ptr %7, align 1, !tbaa !11
  %9 = icmp eq i8 %6, 0
  br i1 %9, label %14, label %10

10:                                               ; preds = %3
  %11 = xor i8 %8, -1
  %12 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  %13 = and i8 %12, %11
  store volatile i8 %13, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  br label %25

14:                                               ; preds = %3
  %15 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %16 = load i8, ptr %15, align 1, !tbaa !12
  %17 = icmp eq i8 %16, 0
  %18 = xor i8 %8, -1
  br i1 %17, label %22, label %19

19:                                               ; preds = %14
  %20 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !6
  %21 = and i8 %20, %18
  store volatile i8 %21, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !6
  br label %25

22:                                               ; preds = %14
  %23 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !6
  %24 = and i8 %23, %18
  store volatile i8 %24, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !6
  br label %25

25:                                               ; preds = %22, %19, %10, %1
  ret void
}

; Function Attrs: nofree norecurse nounwind
define dso_local void @EPIC_IRQ_ClearFlag(i16 noundef %0) local_unnamed_addr #4 {
  %2 = icmp ugt i16 %0, 16
  br i1 %2, label %21, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [17 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !9
  %7 = load i8, ptr %4, align 1, !tbaa !13
  %8 = icmp eq i8 %6, 0
  br i1 %8, label %13, label %9

9:                                                ; preds = %3
  %10 = xor i8 %7, -1
  %11 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  %12 = and i8 %11, %10
  store volatile i8 %12, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  br label %21

13:                                               ; preds = %3
  %14 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %15 = load i8, ptr %14, align 1, !tbaa !12
  %16 = icmp eq i8 %15, 0
  %17 = select i1 %16, ptr inttoptr (i16 12 to ptr), ptr inttoptr (i16 13 to ptr)
  %18 = load volatile i8, ptr %17, align 1, !tbaa !6
  %19 = xor i8 %7, -1
  %20 = and i8 %18, %19
  store volatile i8 %20, ptr %17, align 1, !tbaa !6
  br label %21

21:                                               ; preds = %13, %9, %1
  ret void
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn
define dso_local zeroext range(i8 0, 2) i8 @EPIC_IRQ_GetFlag(i16 noundef %0) local_unnamed_addr #3 {
  %2 = icmp ugt i16 %0, 16
  br i1 %2, label %20, label %3

3:                                                ; preds = %1
  %4 = getelementptr inbounds nuw [17 x %struct.irq_desc_t], ptr @irq_table, i16 0, i16 %0
  %5 = getelementptr inbounds nuw i8, ptr %4, i16 2
  %6 = load i8, ptr %5, align 1, !tbaa !9
  %7 = load i8, ptr %4, align 1, !tbaa !13
  %8 = icmp eq i8 %6, 0
  br i1 %8, label %9, label %14

9:                                                ; preds = %3
  %10 = getelementptr inbounds nuw i8, ptr %4, i16 3
  %11 = load i8, ptr %10, align 1, !tbaa !12
  %12 = icmp eq i8 %11, 0
  %13 = select i1 %12, ptr inttoptr (i16 12 to ptr), ptr inttoptr (i16 13 to ptr)
  br label %14

14:                                               ; preds = %9, %3
  %15 = phi ptr [ %13, %9 ], [ inttoptr (i16 11 to ptr), %3 ]
  %16 = load volatile i8, ptr %15, align 1, !tbaa !6
  %17 = and i8 %16, %7
  %18 = icmp ne i8 %17, 0
  %19 = zext i1 %18 to i8
  br label %20

20:                                               ; preds = %14, %1
  %21 = phi i8 [ %19, %14 ], [ 0, %1 ]
  ret i8 %21
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @EPIC_IRQ_SetPriority(i16 noundef %0, i16 noundef %1) local_unnamed_addr #5 {
  ret void
}

; Function Attrs: noinline nounwind
define dso_local msp430_intrcc void @PIC16_IRQ_Handler() #6 {
  tail call void @epic_dispatch_all_irqs() #11
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_dispatch_all_irqs() local_unnamed_addr #2 {
  %1 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %2 = and i8 %1, 1
  %3 = icmp eq i8 %2, 0
  br i1 %3, label %7, label %4

4:                                                ; preds = %0
  %5 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %6 = and i8 %5, -2
  store volatile i8 %6, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  br label %7

7:                                                ; preds = %4, %0
  %8 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %9 = and i8 %8, 2
  %10 = icmp eq i8 %9, 0
  br i1 %10, label %14, label %11

11:                                               ; preds = %7
  %12 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %13 = and i8 %12, -3
  store volatile i8 %13, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  br label %14

14:                                               ; preds = %11, %7
  %15 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %16 = and i8 %15, 4
  %17 = icmp eq i8 %16, 0
  br i1 %17, label %21, label %18

18:                                               ; preds = %14
  %19 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %20 = and i8 %19, -5
  store volatile i8 %20, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  br label %21

21:                                               ; preds = %18, %14
  %22 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %23 = and i8 %22, 8
  %24 = icmp eq i8 %23, 0
  br i1 %24, label %28, label %25

25:                                               ; preds = %21
  %26 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %27 = and i8 %26, -9
  store volatile i8 %27, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  br label %28

28:                                               ; preds = %25, %21
  %29 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %30 = and i8 %29, 32
  %31 = icmp eq i8 %30, 0
  br i1 %31, label %33, label %32

32:                                               ; preds = %28
  tail call void @USART_RX_IRQHandler() #11
  br label %33

33:                                               ; preds = %32, %28
  %34 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %35 = and i8 %34, 16
  %36 = icmp eq i8 %35, 0
  br i1 %36, label %45, label %37

37:                                               ; preds = %33
  %38 = load volatile i8, ptr inttoptr (i16 140 to ptr), align 4, !tbaa !6
  %39 = and i8 %38, 16
  %40 = icmp eq i8 %39, 0
  br i1 %40, label %42, label %41

41:                                               ; preds = %37
  tail call void @USART_TX_IRQHandler() #11
  br label %45

42:                                               ; preds = %37
  %43 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %44 = and i8 %43, -17
  store volatile i8 %44, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  br label %45

45:                                               ; preds = %42, %41, %33
  %46 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %47 = and i8 %46, 64
  %48 = icmp eq i8 %47, 0
  br i1 %48, label %52, label %49

49:                                               ; preds = %45
  %50 = load volatile i8, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  %51 = and i8 %50, -65
  store volatile i8 %51, ptr inttoptr (i16 12 to ptr), align 4, !tbaa !6
  br label %52

52:                                               ; preds = %49, %45
  %53 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %54 = and i8 %53, 1
  %55 = icmp eq i8 %54, 0
  br i1 %55, label %59, label %56

56:                                               ; preds = %52
  %57 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %58 = and i8 %57, -2
  store volatile i8 %58, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  br label %59

59:                                               ; preds = %56, %52
  %60 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %61 = and i8 %60, 32
  %62 = icmp eq i8 %61, 0
  br i1 %62, label %66, label %63

63:                                               ; preds = %59
  %64 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %65 = and i8 %64, -33
  store volatile i8 %65, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  br label %66

66:                                               ; preds = %63, %59
  %67 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %68 = and i8 %67, 64
  %69 = icmp eq i8 %68, 0
  br i1 %69, label %73, label %70

70:                                               ; preds = %66
  %71 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %72 = and i8 %71, -65
  store volatile i8 %72, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  br label %73

73:                                               ; preds = %70, %66
  %74 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %75 = and i8 %74, 4
  %76 = icmp eq i8 %75, 0
  br i1 %76, label %80, label %77

77:                                               ; preds = %73
  %78 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %79 = and i8 %78, -5
  store volatile i8 %79, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  br label %80

80:                                               ; preds = %77, %73
  %81 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %82 = and i8 %81, 16
  %83 = icmp eq i8 %82, 0
  br i1 %83, label %91, label %84

84:                                               ; preds = %80
  %85 = load volatile i8, ptr inttoptr (i16 141 to ptr), align 1, !tbaa !6
  %86 = and i8 %85, 16
  %87 = icmp eq i8 %86, 0
  br i1 %87, label %88, label %91

88:                                               ; preds = %84
  %89 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %90 = and i8 %89, -17
  store volatile i8 %90, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  br label %91

91:                                               ; preds = %88, %84, %80
  %92 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %93 = icmp sgt i8 %92, -1
  br i1 %93, label %97, label %94

94:                                               ; preds = %91
  %95 = load volatile i8, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  %96 = and i8 %95, 127
  store volatile i8 %96, ptr inttoptr (i16 13 to ptr), align 1, !tbaa !6
  br label %97

97:                                               ; preds = %94, %91
  %98 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  %99 = and i8 %98, 4
  %100 = icmp eq i8 %99, 0
  br i1 %100, label %104, label %101

101:                                              ; preds = %97
  %102 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  %103 = and i8 %102, -5
  store volatile i8 %103, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  br label %104

104:                                              ; preds = %101, %97
  %105 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  %106 = and i8 %105, 1
  %107 = icmp eq i8 %106, 0
  br i1 %107, label %111, label %108

108:                                              ; preds = %104
  %109 = load volatile i8, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  %110 = and i8 %109, -2
  store volatile i8 %110, ptr inttoptr (i16 11 to ptr), align 1, !tbaa !6
  br label %111

111:                                              ; preds = %108, %104
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @epic_harness_init(i32 noundef %0) local_unnamed_addr #5 {
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @epic_harness_tick() local_unnamed_addr #5 {
  ret void
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local noundef i16 @epic_harness_running(i32 noundef %0) local_unnamed_addr #5 {
  ret i16 1
}

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none)
define dso_local void @epic_harness_log(ptr nocapture noundef readnone %0, ...) local_unnamed_addr #5 {
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_init(i32 noundef %0, i32 noundef %1) local_unnamed_addr #2 {
  %3 = tail call zeroext i16 @USART_ComputeSPBRG(i32 noundef %0, i32 noundef %1, i16 noundef 0, i16 noundef 1) #11
  %4 = and i16 %3, 255
  store i16 0, ptr @epic_serial_init.s_usart, align 2, !tbaa !14
  store i16 1, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 2), align 2, !tbaa !14
  store i16 1, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 4), align 2, !tbaa !14
  store i16 0, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 6), align 2, !tbaa !14
  store i16 %4, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 8), align 2, !tbaa !16
  store i16 0, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 10), align 2
  store ptr @epic_serial_on_tx, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 12), align 2, !tbaa !7
  store ptr @epic_serial_on_rx, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 14), align 2, !tbaa !7
  store ptr @epic_serial_on_tx, ptr @g_usart_tx_cb, align 2, !tbaa !7
  store ptr @epic_serial_on_rx, ptr @g_usart_rx_cb, align 2, !tbaa !7
  store volatile i8 0, ptr inttoptr (i16 154 to ptr), align 2, !tbaa !6
  %5 = load i16, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 8), align 2, !tbaa !18
  %6 = trunc i16 %5 to i8
  store volatile i8 %6, ptr inttoptr (i16 153 to ptr), align 1, !tbaa !6
  %7 = load i16, ptr @epic_serial_init.s_usart, align 2, !tbaa !20
  %8 = icmp eq i16 %7, 1
  %9 = load i16, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 2), align 2
  %10 = icmp eq i16 %9, 1
  %11 = select i1 %10, i8 -110, i8 18
  %12 = select i1 %8, i8 %11, i8 2
  %13 = load i16, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 4), align 2, !tbaa !21
  %14 = icmp eq i16 %13, 1
  %15 = or disjoint i8 %12, 4
  %16 = select i1 %14, i8 %15, i8 %12
  %17 = load i16, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 6), align 2, !tbaa !22
  %18 = icmp eq i16 %17, 1
  %19 = or disjoint i8 %16, 64
  %20 = select i1 %18, i8 %19, i8 %16
  %21 = load ptr, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 12), align 2, !tbaa !23
  %22 = icmp eq ptr %21, null
  %23 = or i8 %20, 32
  %24 = select i1 %22, i8 %20, i8 %23
  store volatile i8 %24, ptr inttoptr (i16 152 to ptr), align 8, !tbaa !6
  %25 = load i16, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 6), align 2, !tbaa !22
  %26 = icmp eq i16 %25, 1
  %27 = select i1 %26, i8 -64, i8 -128
  %28 = load ptr, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 14), align 2, !tbaa !24
  %29 = icmp eq ptr %28, null
  %30 = or disjoint i8 %27, 16
  %31 = select i1 %29, i8 %27, i8 %30
  store volatile i8 %31, ptr inttoptr (i16 24 to ptr), align 8, !tbaa !6
  %32 = load i8, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 10), align 2, !tbaa !25
  %33 = icmp eq i8 %32, 0
  %34 = select i1 %33, i8 0, i8 8
  store volatile i8 %34, ptr inttoptr (i16 391 to ptr), align 1, !tbaa !6
  tail call void @EPIC_IRQ_ClearFlag(i16 noundef 10) #11
  %35 = load ptr, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 12), align 2, !tbaa !23
  %36 = icmp eq ptr %35, null
  br i1 %36, label %38, label %37

37:                                               ; preds = %2
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  br label %39

38:                                               ; preds = %2
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #11
  br label %39

39:                                               ; preds = %38, %37
  %40 = load ptr, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 14), align 2, !tbaa !24
  %41 = icmp eq ptr %40, null
  br i1 %41, label %43, label %42

42:                                               ; preds = %39
  tail call void @EPIC_IRQ_Enable(i16 noundef 10) #11
  br label %44

43:                                               ; preds = %39
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 10) #11
  br label %44

44:                                               ; preds = %43, %42
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #11
  store volatile i8 0, ptr @g_tx_count, align 1, !tbaa !6
  store volatile i8 0, ptr @g_tx_tail, align 1, !tbaa !6
  store volatile i8 0, ptr @g_tx_head, align 1, !tbaa !6
  store volatile i8 0, ptr @g_rx_count, align 1, !tbaa !6
  store volatile i8 0, ptr @g_rx_tail, align 1, !tbaa !6
  store volatile i8 0, ptr @g_rx_head, align 1, !tbaa !6
  ret void
}

; Function Attrs: nounwind
define internal void @epic_serial_on_tx() #2 {
  %1 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %2 = icmp eq i8 %1, 0
  br i1 %2, label %13, label %3

3:                                                ; preds = %0
  %4 = load volatile i8, ptr @g_tx_tail, align 1, !tbaa !6
  %5 = zext i8 %4 to i16
  %6 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %5
  %7 = load volatile i8, ptr %6, align 1, !tbaa !6
  %8 = load volatile i8, ptr @g_tx_tail, align 1, !tbaa !6
  %9 = add i8 %8, 1
  %10 = and i8 %9, 31
  store volatile i8 %10, ptr @g_tx_tail, align 1, !tbaa !6
  %11 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %12 = add i8 %11, -1
  store volatile i8 %12, ptr @g_tx_count, align 1, !tbaa !6
  store volatile i8 %7, ptr inttoptr (i16 25 to ptr), align 1, !tbaa !6
  br label %14

13:                                               ; preds = %0
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #11
  br label %14

14:                                               ; preds = %13, %3
  ret void
}

; Function Attrs: nofree norecurse nounwind memory(readwrite, argmem: none)
define internal void @epic_serial_on_rx(i8 noundef zeroext %0) #7 {
  %2 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !6
  %3 = icmp ult i8 %2, 32
  br i1 %3, label %4, label %13

4:                                                ; preds = %1
  %5 = load volatile i8, ptr @g_rx_head, align 1, !tbaa !6
  %6 = zext i8 %5 to i16
  %7 = getelementptr inbounds nuw [32 x i8], ptr @g_rx_buf, i16 0, i16 %6
  store volatile i8 %0, ptr %7, align 1, !tbaa !6
  %8 = load volatile i8, ptr @g_rx_head, align 1, !tbaa !6
  %9 = add i8 %8, 1
  %10 = and i8 %9, 31
  store volatile i8 %10, ptr @g_rx_head, align 1, !tbaa !6
  %11 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !6
  %12 = add i8 %11, 1
  store volatile i8 %12, ptr @g_rx_count, align 1, !tbaa !6
  br label %13

13:                                               ; preds = %4, %1
  ret void
}

; Function Attrs: nounwind
define dso_local noundef i16 @epic_serial_write(ptr nocapture noundef readonly %0, i16 noundef returned %1) local_unnamed_addr #2 {
  %3 = icmp sgt i16 %1, 0
  br i1 %3, label %4, label %6

4:                                                ; preds = %10, %2
  %5 = phi i16 [ %21, %10 ], [ 0, %2 ]
  br label %7

6:                                                ; preds = %10, %2
  ret i16 %1

7:                                                ; preds = %7, %4
  %8 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %9 = icmp ugt i8 %8, 31
  br i1 %9, label %7, label %10, !llvm.loop !26

10:                                               ; preds = %7
  %11 = getelementptr inbounds nuw i8, ptr %0, i16 %5
  %12 = load i8, ptr %11, align 1, !tbaa !6
  %13 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %14 = zext i8 %13 to i16
  %15 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %14
  store volatile i8 %12, ptr %15, align 1, !tbaa !6
  %16 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %17 = add i8 %16, 1
  %18 = and i8 %17, 31
  store volatile i8 %18, ptr @g_tx_head, align 1, !tbaa !6
  %19 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %20 = add i8 %19, 1
  store volatile i8 %20, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  %21 = add nuw nsw i16 %5, 1
  %22 = icmp eq i16 %21, %1
  br i1 %22, label %6, label %4, !llvm.loop !29
}

; Function Attrs: nofree norecurse nounwind memory(readwrite, argmem: write)
define dso_local i16 @epic_serial_read(ptr nocapture noundef writeonly %0, i16 noundef %1) local_unnamed_addr #8 {
  %3 = icmp sgt i16 %1, 0
  br i1 %3, label %4, label %21

4:                                                ; preds = %8, %2
  %5 = phi i16 [ %13, %8 ], [ 0, %2 ]
  %6 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !6
  %7 = icmp eq i8 %6, 0
  br i1 %7, label %21, label %8

8:                                                ; preds = %4
  %9 = load volatile i8, ptr @g_rx_tail, align 1, !tbaa !6
  %10 = zext i8 %9 to i16
  %11 = getelementptr inbounds nuw [32 x i8], ptr @g_rx_buf, i16 0, i16 %10
  %12 = load volatile i8, ptr %11, align 1, !tbaa !6
  %13 = add nuw nsw i16 %5, 1
  %14 = getelementptr inbounds nuw i8, ptr %0, i16 %5
  store i8 %12, ptr %14, align 1, !tbaa !6
  %15 = load volatile i8, ptr @g_rx_tail, align 1, !tbaa !6
  %16 = add i8 %15, 1
  %17 = and i8 %16, 31
  store volatile i8 %17, ptr @g_rx_tail, align 1, !tbaa !6
  %18 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !6
  %19 = add i8 %18, -1
  store volatile i8 %19, ptr @g_rx_count, align 1, !tbaa !6
  %20 = icmp eq i16 %13, %1
  br i1 %20, label %21, label %4, !llvm.loop !30

21:                                               ; preds = %8, %4, %2
  %22 = phi i16 [ 0, %2 ], [ %5, %4 ], [ %1, %8 ]
  ret i16 %22
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn memory(readwrite, argmem: none)
define dso_local range(i16 0, 256) i16 @epic_serial_available() local_unnamed_addr #9 {
  %1 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !6
  %2 = zext i8 %1 to i16
  ret i16 %2
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn memory(readwrite, argmem: none)
define dso_local range(i16 0, 256) i16 @epic_serial_tx_pending() local_unnamed_addr #9 {
  %1 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %2 = zext i8 %1 to i16
  ret i16 %2
}

; Function Attrs: nounwind
define dso_local void @epic_serial_flush() local_unnamed_addr #2 {
  br label %1

1:                                                ; preds = %1, %0
  %2 = tail call zeroext i8 @EPIC_USART_IsTxShiftRegisterEmpty() #11
  %3 = icmp eq i8 %2, 0
  br i1 %3, label %1, label %4, !llvm.loop !31

4:                                                ; preds = %1
  ret void
}

; Function Attrs: nounwind
define dso_local void @putch(i8 noundef zeroext %0) local_unnamed_addr #2 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %4 = icmp ugt i8 %3, 31
  br i1 %4, label %2, label %5, !llvm.loop !26

5:                                                ; preds = %2
  %6 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %7 = zext i8 %6 to i16
  %8 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %7
  store volatile i8 %0, ptr %8, align 1, !tbaa !6
  %9 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %10 = add i8 %9, 1
  %11 = and i8 %10, 31
  store volatile i8 %11, ptr @g_tx_head, align 1, !tbaa !6
  %12 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %13 = add i8 %12, 1
  store volatile i8 %13, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_char(i8 noundef zeroext %0) local_unnamed_addr #2 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %4 = icmp ugt i8 %3, 31
  br i1 %4, label %2, label %5, !llvm.loop !26

5:                                                ; preds = %2
  %6 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %7 = zext i8 %6 to i16
  %8 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %7
  store volatile i8 %0, ptr %8, align 1, !tbaa !6
  %9 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %10 = add i8 %9, 1
  %11 = and i8 %10, 31
  store volatile i8 %11, ptr @g_tx_head, align 1, !tbaa !6
  %12 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %13 = add i8 %12, 1
  store volatile i8 %13, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_str(ptr nocapture noundef readonly %0) local_unnamed_addr #2 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi i16 [ 0, %1 ], [ %7, %2 ]
  %4 = getelementptr inbounds nuw i8, ptr %0, i16 %3
  %5 = load i8, ptr %4, align 1, !tbaa !6
  %6 = icmp eq i8 %5, 0
  %7 = add nuw nsw i16 %3, 1
  br i1 %6, label %8, label %2, !llvm.loop !32

8:                                                ; preds = %2
  %9 = icmp eq i16 %3, 0
  br i1 %9, label %28, label %10

10:                                               ; preds = %15, %8
  %11 = phi i16 [ %26, %15 ], [ 0, %8 ]
  br label %12

12:                                               ; preds = %12, %10
  %13 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %14 = icmp ugt i8 %13, 31
  br i1 %14, label %12, label %15, !llvm.loop !26

15:                                               ; preds = %12
  %16 = getelementptr inbounds nuw i8, ptr %0, i16 %11
  %17 = load i8, ptr %16, align 1, !tbaa !6
  %18 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %19 = zext i8 %18 to i16
  %20 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %19
  store volatile i8 %17, ptr %20, align 1, !tbaa !6
  %21 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %22 = add i8 %21, 1
  %23 = and i8 %22, 31
  store volatile i8 %23, ptr @g_tx_head, align 1, !tbaa !6
  %24 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %25 = add i8 %24, 1
  store volatile i8 %25, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  %26 = add nuw nsw i16 %11, 1
  %27 = icmp eq i16 %26, %3
  br i1 %27, label %28, label %10, !llvm.loop !29

28:                                               ; preds = %15, %8
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_u16(i16 noundef zeroext %0) local_unnamed_addr #2 {
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
  store i8 %12, ptr %14, align 1, !tbaa !6
  %15 = add i8 %6, 1
  %16 = icmp samesign ult i32 %5, 10
  %17 = add i8 %4, 1
  br i1 %16, label %18, label %3, !llvm.loop !33

18:                                               ; preds = %3
  %19 = icmp eq i8 %15, 0
  br i1 %19, label %40, label %20

20:                                               ; preds = %18
  %21 = zext i8 %4 to i16
  br label %22

22:                                               ; preds = %30, %20
  %23 = phi i16 [ %21, %20 ], [ %24, %30 ]
  %24 = add nsw i16 %23, -1
  %25 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %24
  %26 = load i8, ptr %25, align 1, !tbaa !6
  br label %27

27:                                               ; preds = %27, %22
  %28 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %29 = icmp ugt i8 %28, 31
  br i1 %29, label %27, label %30, !llvm.loop !26

30:                                               ; preds = %27
  %31 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %32 = zext i8 %31 to i16
  %33 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %32
  store volatile i8 %26, ptr %33, align 1, !tbaa !6
  %34 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %35 = add i8 %34, 1
  %36 = and i8 %35, 31
  store volatile i8 %36, ptr @g_tx_head, align 1, !tbaa !6
  %37 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %38 = add i8 %37, 1
  store volatile i8 %38, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  %39 = icmp eq i16 %24, 0
  br i1 %39, label %40, label %22, !llvm.loop !34

40:                                               ; preds = %30, %18
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_u32(i32 noundef %0) local_unnamed_addr #2 {
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
  store i8 %11, ptr %13, align 1, !tbaa !6
  %14 = add i8 %5, 1
  %15 = icmp ult i32 %4, 10
  %16 = add i8 %3, 1
  br i1 %15, label %17, label %2, !llvm.loop !33

17:                                               ; preds = %2
  %18 = icmp eq i8 %14, 0
  br i1 %18, label %39, label %19

19:                                               ; preds = %17
  %20 = zext i8 %3 to i16
  br label %21

21:                                               ; preds = %29, %19
  %22 = phi i16 [ %20, %19 ], [ %23, %29 ]
  %23 = add nsw i16 %22, -1
  %24 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %23
  %25 = load i8, ptr %24, align 1, !tbaa !6
  br label %26

26:                                               ; preds = %26, %21
  %27 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %28 = icmp ugt i8 %27, 31
  br i1 %28, label %26, label %29, !llvm.loop !26

29:                                               ; preds = %26
  %30 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %31 = zext i8 %30 to i16
  %32 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %31
  store volatile i8 %25, ptr %32, align 1, !tbaa !6
  %33 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %34 = add i8 %33, 1
  %35 = and i8 %34, 31
  store volatile i8 %35, ptr @g_tx_head, align 1, !tbaa !6
  %36 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %37 = add i8 %36, 1
  store volatile i8 %37, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  %38 = icmp eq i16 %23, 0
  br i1 %38, label %39, label %21, !llvm.loop !34

39:                                               ; preds = %29, %17
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_i16(i16 noundef signext %0) local_unnamed_addr #2 {
  %2 = sext i16 %0 to i32
  tail call fastcc void @epic_serial_put_idec(i32 noundef %2) #12
  ret void
}

; Function Attrs: nounwind
define internal fastcc void @epic_serial_put_idec(i32 noundef %0) unnamed_addr #2 {
  %2 = icmp slt i32 %0, 0
  br i1 %2, label %3, label %53

3:                                                ; preds = %3, %1
  %4 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %5 = icmp ugt i8 %4, 31
  br i1 %5, label %3, label %6, !llvm.loop !26

6:                                                ; preds = %3
  %7 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %8 = zext i8 %7 to i16
  %9 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %8
  store volatile i8 45, ptr %9, align 1, !tbaa !6
  %10 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %11 = add i8 %10, 1
  %12 = and i8 %11, 31
  store volatile i8 %12, ptr @g_tx_head, align 1, !tbaa !6
  %13 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %14 = add i8 %13, 1
  store volatile i8 %14, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  %15 = sub i32 0, %0
  br label %16

16:                                               ; preds = %16, %6
  %17 = phi i8 [ %30, %16 ], [ 1, %6 ]
  %18 = phi i32 [ %21, %16 ], [ %15, %6 ]
  %19 = phi i8 [ %28, %16 ], [ 0, %6 ]
  %20 = freeze i32 %18
  %21 = udiv i32 %20, 10
  %22 = mul i32 %21, 10
  %23 = sub i32 %20, %22
  %24 = trunc nuw nsw i32 %23 to i8
  %25 = or disjoint i8 %24, 48
  %26 = zext i8 %19 to i16
  %27 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %26
  store i8 %25, ptr %27, align 1, !tbaa !6
  %28 = add i8 %19, 1
  %29 = icmp ult i32 %18, 10
  %30 = add i8 %17, 1
  br i1 %29, label %31, label %16, !llvm.loop !33

31:                                               ; preds = %16
  %32 = icmp eq i8 %28, 0
  br i1 %32, label %90, label %33

33:                                               ; preds = %31
  %34 = zext i8 %17 to i16
  br label %35

35:                                               ; preds = %43, %33
  %36 = phi i16 [ %34, %33 ], [ %37, %43 ]
  %37 = add nsw i16 %36, -1
  %38 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %37
  %39 = load i8, ptr %38, align 1, !tbaa !6
  br label %40

40:                                               ; preds = %40, %35
  %41 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %42 = icmp ugt i8 %41, 31
  br i1 %42, label %40, label %43, !llvm.loop !26

43:                                               ; preds = %40
  %44 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %45 = zext i8 %44 to i16
  %46 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %45
  store volatile i8 %39, ptr %46, align 1, !tbaa !6
  %47 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %48 = add i8 %47, 1
  %49 = and i8 %48, 31
  store volatile i8 %49, ptr @g_tx_head, align 1, !tbaa !6
  %50 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %51 = add i8 %50, 1
  store volatile i8 %51, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  %52 = icmp eq i16 %37, 0
  br i1 %52, label %90, label %35, !llvm.loop !34

53:                                               ; preds = %53, %1
  %54 = phi i8 [ %67, %53 ], [ 1, %1 ]
  %55 = phi i32 [ %58, %53 ], [ %0, %1 ]
  %56 = phi i8 [ %65, %53 ], [ 0, %1 ]
  %57 = freeze i32 %55
  %58 = udiv i32 %57, 10
  %59 = mul i32 %58, 10
  %60 = sub i32 %57, %59
  %61 = trunc nuw nsw i32 %60 to i8
  %62 = or disjoint i8 %61, 48
  %63 = zext i8 %56 to i16
  %64 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %63
  store i8 %62, ptr %64, align 1, !tbaa !6
  %65 = add i8 %56, 1
  %66 = icmp samesign ult i32 %55, 10
  %67 = add i8 %54, 1
  br i1 %66, label %68, label %53, !llvm.loop !33

68:                                               ; preds = %53
  %69 = icmp eq i8 %65, 0
  br i1 %69, label %90, label %70

70:                                               ; preds = %68
  %71 = zext i8 %54 to i16
  br label %72

72:                                               ; preds = %80, %70
  %73 = phi i16 [ %71, %70 ], [ %74, %80 ]
  %74 = add nsw i16 %73, -1
  %75 = getelementptr inbounds nuw [12 x i8], ptr @s_fmt_buf, i16 0, i16 %74
  %76 = load i8, ptr %75, align 1, !tbaa !6
  br label %77

77:                                               ; preds = %77, %72
  %78 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %79 = icmp ugt i8 %78, 31
  br i1 %79, label %77, label %80, !llvm.loop !26

80:                                               ; preds = %77
  %81 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %82 = zext i8 %81 to i16
  %83 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %82
  store volatile i8 %76, ptr %83, align 1, !tbaa !6
  %84 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %85 = add i8 %84, 1
  %86 = and i8 %85, 31
  store volatile i8 %86, ptr @g_tx_head, align 1, !tbaa !6
  %87 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %88 = add i8 %87, 1
  store volatile i8 %88, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  %89 = icmp eq i16 %74, 0
  br i1 %89, label %90, label %72, !llvm.loop !34

90:                                               ; preds = %80, %68, %43, %31
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_i32(i32 noundef %0) local_unnamed_addr #2 {
  tail call fastcc void @epic_serial_put_idec(i32 noundef %0) #12
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_hex8(i8 noundef zeroext %0) local_unnamed_addr #2 {
  %2 = zext i8 %0 to i32
  br label %3

3:                                                ; preds = %16, %1
  %4 = phi i16 [ 2, %1 ], [ %5, %16 ]
  %5 = add nsw i16 %4, -1
  %6 = shl nuw nsw i16 %5, 2
  %7 = zext nneg i16 %6 to i32
  %8 = lshr i32 %2, %7
  %9 = trunc nuw nsw i32 %8 to i16
  %10 = and i16 %9, 15
  %11 = icmp samesign ult i16 %10, 10
  %12 = add nuw nsw i16 %10, 55
  br label %13

13:                                               ; preds = %13, %3
  %14 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %15 = icmp ugt i8 %14, 31
  br i1 %15, label %13, label %16, !llvm.loop !26

16:                                               ; preds = %13
  %17 = or disjoint i16 %10, 48
  %18 = select i1 %11, i16 %17, i16 %12
  %19 = trunc nuw nsw i16 %18 to i8
  %20 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %21 = zext i8 %20 to i16
  %22 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %21
  store volatile i8 %19, ptr %22, align 1, !tbaa !6
  %23 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %24 = add i8 %23, 1
  %25 = and i8 %24, 31
  store volatile i8 %25, ptr @g_tx_head, align 1, !tbaa !6
  %26 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %27 = add i8 %26, 1
  store volatile i8 %27, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  %28 = icmp ugt i16 %4, 1
  br i1 %28, label %3, label %29, !llvm.loop !35

29:                                               ; preds = %16
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_hex16(i16 noundef zeroext %0) local_unnamed_addr #2 {
  %2 = zext i16 %0 to i32
  br label %3

3:                                                ; preds = %16, %1
  %4 = phi i16 [ 4, %1 ], [ %5, %16 ]
  %5 = add nsw i16 %4, -1
  %6 = shl nuw nsw i16 %5, 2
  %7 = zext nneg i16 %6 to i32
  %8 = lshr i32 %2, %7
  %9 = trunc nuw i32 %8 to i16
  %10 = and i16 %9, 15
  %11 = icmp samesign ult i16 %10, 10
  %12 = add nuw nsw i16 %10, 55
  br label %13

13:                                               ; preds = %13, %3
  %14 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %15 = icmp ugt i8 %14, 31
  br i1 %15, label %13, label %16, !llvm.loop !26

16:                                               ; preds = %13
  %17 = or disjoint i16 %10, 48
  %18 = select i1 %11, i16 %17, i16 %12
  %19 = trunc nuw nsw i16 %18 to i8
  %20 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %21 = zext i8 %20 to i16
  %22 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %21
  store volatile i8 %19, ptr %22, align 1, !tbaa !6
  %23 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %24 = add i8 %23, 1
  %25 = and i8 %24, 31
  store volatile i8 %25, ptr @g_tx_head, align 1, !tbaa !6
  %26 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %27 = add i8 %26, 1
  store volatile i8 %27, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #11
  %28 = icmp ugt i16 %4, 1
  br i1 %28, label %3, label %29, !llvm.loop !35

29:                                               ; preds = %16
  ret void
}

; Function Attrs: noreturn nounwind
define dso_local noundef i16 @main() local_unnamed_addr #10 {
  tail call void @epic_serial_init(i32 noundef 20000000, i32 noundef 115200) #11
  tail call void @EPIC_IRQ_Restore(i8 noundef zeroext 1) #11
  br label %1

1:                                                ; preds = %1, %0
  %2 = phi i16 [ 0, %0 ], [ %5, %1 ]
  %3 = getelementptr inbounds nuw [34 x i8], ptr @main.banner, i16 0, i16 %2
  %4 = load i8, ptr %3, align 1, !tbaa !6
  tail call void @epic_serial_put_char(i8 noundef zeroext %4) #11
  %5 = add nuw nsw i16 %2, 1
  %6 = icmp eq i16 %5, 33
  br i1 %6, label %7, label %1, !llvm.loop !36

7:                                                ; preds = %13, %1
  %8 = tail call i16 @epic_serial_available() #11
  %9 = icmp sgt i16 %8, 0
  br i1 %9, label %10, label %13

10:                                               ; preds = %7
  %11 = tail call i16 @epic_serial_read(ptr noundef nonnull @main.buf, i16 noundef 8) #11
  %12 = icmp sgt i16 %11, 0
  br i1 %12, label %14, label %13

13:                                               ; preds = %14, %10, %7
  br label %7, !llvm.loop !37

14:                                               ; preds = %10
  %15 = tail call i16 @epic_serial_write(ptr noundef nonnull @main.buf, i16 noundef %11) #11
  br label %13
}

attributes #0 = { nofree norecurse nounwind memory(inaccessiblemem: readwrite) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #2 = { nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { mustprogress nofree norecurse nounwind willreturn "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #4 = { nofree norecurse nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #5 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #6 = { noinline nounwind "interrupt"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #7 = { nofree norecurse nounwind memory(readwrite, argmem: none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #8 = { nofree norecurse nounwind memory(readwrite, argmem: write) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #9 = { mustprogress nofree norecurse nounwind willreturn memory(readwrite, argmem: none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #10 = { noreturn nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #11 = { nobuiltin nounwind "no-builtins" }
attributes #12 = { nobuiltin "no-builtins" }

!llvm.ident = !{!0, !0, !0, !0, !0, !0, !0}
!llvm.module.flags = !{!1}

!0 = !{!"clang version 20.1.8"}
!1 = !{i32 1, !"wchar_size", i32 2}
!2 = !{!3, !3, i64 0}
!3 = !{!"long", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!4, !4, i64 0}
!7 = !{!8, !8, i64 0}
!8 = !{!"any pointer", !4, i64 0}
!9 = !{!10, !4, i64 2}
!10 = !{!"", !4, i64 0, !4, i64 1, !4, i64 2, !4, i64 3}
!11 = !{!10, !4, i64 1}
!12 = !{!10, !4, i64 3}
!13 = !{!10, !4, i64 0}
!14 = !{!15, !15, i64 0}
!15 = !{!"int", !4, i64 0}
!16 = !{!17, !17, i64 0}
!17 = !{!"short", !4, i64 0}
!18 = !{!19, !17, i64 8}
!19 = !{!"", !15, i64 0, !15, i64 2, !15, i64 4, !15, i64 6, !17, i64 8, !4, i64 10, !8, i64 12, !8, i64 14}
!20 = !{!19, !15, i64 0}
!21 = !{!19, !15, i64 4}
!22 = !{!19, !15, i64 6}
!23 = !{!19, !8, i64 12}
!24 = !{!19, !8, i64 14}
!25 = !{!19, !4, i64 10}
!26 = distinct !{!26, !27, !28}
!27 = !{!"llvm.loop.mustprogress"}
!28 = !{!"llvm.loop.unroll.disable"}
!29 = distinct !{!29, !27, !28}
!30 = distinct !{!30, !27, !28}
!31 = distinct !{!31, !27, !28}
!32 = distinct !{!32, !27, !28}
!33 = distinct !{!33, !27, !28}
!34 = distinct !{!34, !27, !28}
!35 = distinct !{!35, !27, !28}
!36 = distinct !{!36, !27, !28}
!37 = distinct !{!37, !28}
