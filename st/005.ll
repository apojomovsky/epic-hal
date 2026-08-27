; ModuleID = 'epic-serial/src/epic_serial.c'
source_filename = "epic-serial/src/epic_serial.c"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

%struct.USART_HandleTypeDef = type { i16, i16, i16, i16, i8, ptr, ptr }

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

; Function Attrs: nounwind
define dso_local void @epic_serial_init(i32 noundef %0, i32 noundef %1) local_unnamed_addr #0 {
  %3 = tail call zeroext i16 @USART_ComputeSPBRG(i32 noundef %0, i32 noundef %1, i16 noundef 0, i16 noundef 1) #5
  %4 = trunc i16 %3 to i8
  store i16 0, ptr @epic_serial_init.s_usart, align 2, !tbaa !2
  store i16 1, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 2), align 2, !tbaa !2
  store i16 1, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 4), align 2, !tbaa !2
  store i16 0, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 6), align 2, !tbaa !2
  store i8 %4, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 8), align 2, !tbaa !6
  store i8 0, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 9), align 1
  store ptr @epic_serial_on_tx, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 10), align 2, !tbaa !7
  store ptr @epic_serial_on_rx, ptr getelementptr inbounds nuw (i8, ptr @epic_serial_init.s_usart, i16 12), align 2, !tbaa !7
  %5 = tail call i16 @EPIC_USART_Init(ptr noundef nonnull @epic_serial_init.s_usart) #5
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #5
  store volatile i8 0, ptr @g_tx_count, align 1, !tbaa !6
  store volatile i8 0, ptr @g_tx_tail, align 1, !tbaa !6
  store volatile i8 0, ptr @g_tx_head, align 1, !tbaa !6
  store volatile i8 0, ptr @g_rx_count, align 1, !tbaa !6
  store volatile i8 0, ptr @g_rx_tail, align 1, !tbaa !6
  store volatile i8 0, ptr @g_rx_head, align 1, !tbaa !6
  ret void
}

declare dso_local zeroext i16 @USART_ComputeSPBRG(i32 noundef, i32 noundef, i16 noundef, i16 noundef) local_unnamed_addr #1

; Function Attrs: nofree norecurse nounwind memory(readwrite, argmem: none)
define internal void @epic_serial_on_rx(i8 noundef zeroext %0) #2 {
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
define internal void @epic_serial_on_tx() #0 {
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
  tail call void @EPIC_IRQ_DisableSrc(i16 noundef 9) #5
  br label %14

14:                                               ; preds = %13, %3
  ret void
}

declare dso_local i16 @EPIC_USART_Init(ptr noundef) local_unnamed_addr #1

declare dso_local void @EPIC_IRQ_DisableSrc(i16 noundef) local_unnamed_addr #1

; Function Attrs: nounwind
define dso_local noundef i16 @epic_serial_write(ptr nocapture noundef readonly %0, i16 noundef returned %1) local_unnamed_addr #0 {
  %3 = icmp sgt i16 %1, 0
  br i1 %3, label %4, label %8

4:                                                ; preds = %2, %12
  %5 = phi i16 [ %23, %12 ], [ 0, %2 ]
  %6 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %7 = icmp ugt i8 %6, 31
  br i1 %7, label %9, label %12

8:                                                ; preds = %12, %2
  ret i16 %1

9:                                                ; preds = %4, %9
  tail call void @epic_dispatch_all_irqs() #5
  %10 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %11 = icmp ugt i8 %10, 31
  br i1 %11, label %9, label %12, !llvm.loop !9

12:                                               ; preds = %9, %4
  %13 = getelementptr inbounds nuw i8, ptr %0, i16 %5
  %14 = load i8, ptr %13, align 1, !tbaa !6
  %15 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %16 = zext i8 %15 to i16
  %17 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %16
  store volatile i8 %14, ptr %17, align 1, !tbaa !6
  %18 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %19 = add i8 %18, 1
  %20 = and i8 %19, 31
  store volatile i8 %20, ptr @g_tx_head, align 1, !tbaa !6
  %21 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %22 = add i8 %21, 1
  store volatile i8 %22, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  %23 = add nuw nsw i16 %5, 1
  %24 = icmp eq i16 %23, %1
  br i1 %24, label %8, label %4, !llvm.loop !12
}

declare dso_local void @epic_dispatch_all_irqs() local_unnamed_addr #1

declare dso_local void @EPIC_IRQ_Enable(i16 noundef) local_unnamed_addr #1

; Function Attrs: nofree norecurse nounwind memory(readwrite, argmem: write)
define dso_local i16 @epic_serial_read(ptr nocapture noundef writeonly %0, i16 noundef %1) local_unnamed_addr #3 {
  %3 = icmp sgt i16 %1, 0
  br i1 %3, label %4, label %21

4:                                                ; preds = %2, %8
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
  br i1 %20, label %21, label %4, !llvm.loop !13

21:                                               ; preds = %4, %8, %2
  %22 = phi i16 [ 0, %2 ], [ %5, %4 ], [ %1, %8 ]
  ret i16 %22
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn memory(readwrite, argmem: none)
define dso_local range(i16 0, 256) i16 @epic_serial_available() local_unnamed_addr #4 {
  %1 = load volatile i8, ptr @g_rx_count, align 1, !tbaa !6
  %2 = zext i8 %1 to i16
  ret i16 %2
}

; Function Attrs: mustprogress nofree norecurse nounwind willreturn memory(readwrite, argmem: none)
define dso_local range(i16 0, 256) i16 @epic_serial_tx_pending() local_unnamed_addr #4 {
  %1 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %2 = zext i8 %1 to i16
  ret i16 %2
}

; Function Attrs: nounwind
define dso_local void @epic_serial_flush() local_unnamed_addr #0 {
  %1 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %2 = icmp eq i8 %1, 0
  br i1 %2, label %3, label %6

3:                                                ; preds = %6, %0
  %4 = tail call zeroext i8 @EPIC_USART_IsTxShiftRegisterEmpty() #5
  %5 = icmp eq i8 %4, 0
  br i1 %5, label %9, label %12

6:                                                ; preds = %0, %6
  tail call void @epic_dispatch_all_irqs() #5
  %7 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %8 = icmp eq i8 %7, 0
  br i1 %8, label %3, label %6, !llvm.loop !14

9:                                                ; preds = %3, %9
  tail call void @epic_dispatch_all_irqs() #5
  %10 = tail call zeroext i8 @EPIC_USART_IsTxShiftRegisterEmpty() #5
  %11 = icmp eq i8 %10, 0
  br i1 %11, label %9, label %12, !llvm.loop !15

12:                                               ; preds = %9, %3
  ret void
}

declare dso_local zeroext i8 @EPIC_USART_IsTxShiftRegisterEmpty() local_unnamed_addr #1

; Function Attrs: nounwind
define dso_local void @putch(i8 noundef zeroext %0) local_unnamed_addr #0 {
  %2 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %3 = icmp ugt i8 %2, 31
  br i1 %3, label %4, label %7

4:                                                ; preds = %1, %4
  tail call void @epic_dispatch_all_irqs() #5
  %5 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %6 = icmp ugt i8 %5, 31
  br i1 %6, label %4, label %7, !llvm.loop !9

7:                                                ; preds = %4, %1
  %8 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %9 = zext i8 %8 to i16
  %10 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %9
  store volatile i8 %0, ptr %10, align 1, !tbaa !6
  %11 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %12 = add i8 %11, 1
  %13 = and i8 %12, 31
  store volatile i8 %13, ptr @g_tx_head, align 1, !tbaa !6
  %14 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %15 = add i8 %14, 1
  store volatile i8 %15, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_char(i8 noundef zeroext %0) local_unnamed_addr #0 {
  %2 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %3 = icmp ugt i8 %2, 31
  br i1 %3, label %4, label %7

4:                                                ; preds = %1, %4
  tail call void @epic_dispatch_all_irqs() #5
  %5 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %6 = icmp ugt i8 %5, 31
  br i1 %6, label %4, label %7, !llvm.loop !9

7:                                                ; preds = %4, %1
  %8 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %9 = zext i8 %8 to i16
  %10 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %9
  store volatile i8 %0, ptr %10, align 1, !tbaa !6
  %11 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %12 = add i8 %11, 1
  %13 = and i8 %12, 31
  store volatile i8 %13, ptr @g_tx_head, align 1, !tbaa !6
  %14 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %15 = add i8 %14, 1
  store volatile i8 %15, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_str(ptr nocapture noundef readonly %0) local_unnamed_addr #0 {
  br label %2

2:                                                ; preds = %2, %1
  %3 = phi i16 [ 0, %1 ], [ %7, %2 ]
  %4 = getelementptr inbounds nuw i8, ptr %0, i16 %3
  %5 = load i8, ptr %4, align 1, !tbaa !6
  %6 = icmp eq i8 %5, 0
  %7 = add nuw nsw i16 %3, 1
  br i1 %6, label %8, label %2, !llvm.loop !16

8:                                                ; preds = %2
  %9 = icmp eq i16 %3, 0
  br i1 %9, label %30, label %10

10:                                               ; preds = %8, %17
  %11 = phi i16 [ %28, %17 ], [ 0, %8 ]
  %12 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %13 = icmp ugt i8 %12, 31
  br i1 %13, label %14, label %17

14:                                               ; preds = %10, %14
  tail call void @epic_dispatch_all_irqs() #5
  %15 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %16 = icmp ugt i8 %15, 31
  br i1 %16, label %14, label %17, !llvm.loop !9

17:                                               ; preds = %14, %10
  %18 = getelementptr inbounds nuw i8, ptr %0, i16 %11
  %19 = load i8, ptr %18, align 1, !tbaa !6
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
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  %28 = add nuw nsw i16 %11, 1
  %29 = icmp eq i16 %28, %3
  br i1 %29, label %30, label %10, !llvm.loop !12

30:                                               ; preds = %17, %8
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_u16(i16 noundef zeroext %0) local_unnamed_addr #0 {
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
  br i1 %16, label %18, label %3, !llvm.loop !17

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
  %26 = load i8, ptr %25, align 1, !tbaa !6
  %27 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %28 = icmp ugt i8 %27, 31
  br i1 %28, label %29, label %32

29:                                               ; preds = %22, %29
  tail call void @epic_dispatch_all_irqs() #5
  %30 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %31 = icmp ugt i8 %30, 31
  br i1 %31, label %29, label %32, !llvm.loop !9

32:                                               ; preds = %29, %22
  %33 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %34 = zext i8 %33 to i16
  %35 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %34
  store volatile i8 %26, ptr %35, align 1, !tbaa !6
  %36 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %37 = add i8 %36, 1
  %38 = and i8 %37, 31
  store volatile i8 %38, ptr @g_tx_head, align 1, !tbaa !6
  %39 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %40 = add i8 %39, 1
  store volatile i8 %40, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  %41 = icmp eq i16 %24, 0
  br i1 %41, label %42, label %22, !llvm.loop !18

42:                                               ; preds = %32, %18
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_u32(i32 noundef %0) local_unnamed_addr #0 {
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
  br i1 %15, label %17, label %2, !llvm.loop !17

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
  %25 = load i8, ptr %24, align 1, !tbaa !6
  %26 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %27 = icmp ugt i8 %26, 31
  br i1 %27, label %28, label %31

28:                                               ; preds = %21, %28
  tail call void @epic_dispatch_all_irqs() #5
  %29 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %30 = icmp ugt i8 %29, 31
  br i1 %30, label %28, label %31, !llvm.loop !9

31:                                               ; preds = %28, %21
  %32 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %33 = zext i8 %32 to i16
  %34 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %33
  store volatile i8 %25, ptr %34, align 1, !tbaa !6
  %35 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %36 = add i8 %35, 1
  %37 = and i8 %36, 31
  store volatile i8 %37, ptr @g_tx_head, align 1, !tbaa !6
  %38 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %39 = add i8 %38, 1
  store volatile i8 %39, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  %40 = icmp eq i16 %23, 0
  br i1 %40, label %41, label %21, !llvm.loop !18

41:                                               ; preds = %31, %17
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_i16(i16 noundef signext %0) local_unnamed_addr #0 {
  %2 = sext i16 %0 to i32
  tail call fastcc void @epic_serial_put_idec(i32 noundef %2) #6
  ret void
}

; Function Attrs: nounwind
define internal fastcc void @epic_serial_put_idec(i32 noundef %0) unnamed_addr #0 {
  %2 = icmp slt i32 %0, 0
  br i1 %2, label %3, label %58

3:                                                ; preds = %1
  %4 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %5 = icmp ugt i8 %4, 31
  br i1 %5, label %6, label %9

6:                                                ; preds = %3, %6
  tail call void @epic_dispatch_all_irqs() #5
  %7 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %8 = icmp ugt i8 %7, 31
  br i1 %8, label %6, label %9, !llvm.loop !9

9:                                                ; preds = %6, %3
  %10 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %11 = zext i8 %10 to i16
  %12 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %11
  store volatile i8 45, ptr %12, align 1, !tbaa !6
  %13 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %14 = add i8 %13, 1
  %15 = and i8 %14, 31
  store volatile i8 %15, ptr @g_tx_head, align 1, !tbaa !6
  %16 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %17 = add i8 %16, 1
  store volatile i8 %17, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
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
  store i8 %28, ptr %30, align 1, !tbaa !6
  %31 = add i8 %22, 1
  %32 = icmp ult i32 %21, 10
  %33 = add i8 %20, 1
  br i1 %32, label %34, label %19, !llvm.loop !17

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
  %42 = load i8, ptr %41, align 1, !tbaa !6
  %43 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %44 = icmp ugt i8 %43, 31
  br i1 %44, label %45, label %48

45:                                               ; preds = %38, %45
  tail call void @epic_dispatch_all_irqs() #5
  %46 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %47 = icmp ugt i8 %46, 31
  br i1 %47, label %45, label %48, !llvm.loop !9

48:                                               ; preds = %45, %38
  %49 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %50 = zext i8 %49 to i16
  %51 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %50
  store volatile i8 %42, ptr %51, align 1, !tbaa !6
  %52 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %53 = add i8 %52, 1
  %54 = and i8 %53, 31
  store volatile i8 %54, ptr @g_tx_head, align 1, !tbaa !6
  %55 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %56 = add i8 %55, 1
  store volatile i8 %56, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  %57 = icmp eq i16 %40, 0
  br i1 %57, label %97, label %38, !llvm.loop !18

58:                                               ; preds = %1, %58
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
  store i8 %67, ptr %69, align 1, !tbaa !6
  %70 = add i8 %61, 1
  %71 = icmp samesign ult i32 %60, 10
  %72 = add i8 %59, 1
  br i1 %71, label %73, label %58, !llvm.loop !17

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
  %81 = load i8, ptr %80, align 1, !tbaa !6
  %82 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %83 = icmp ugt i8 %82, 31
  br i1 %83, label %84, label %87

84:                                               ; preds = %77, %84
  tail call void @epic_dispatch_all_irqs() #5
  %85 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %86 = icmp ugt i8 %85, 31
  br i1 %86, label %84, label %87, !llvm.loop !9

87:                                               ; preds = %84, %77
  %88 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %89 = zext i8 %88 to i16
  %90 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %89
  store volatile i8 %81, ptr %90, align 1, !tbaa !6
  %91 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %92 = add i8 %91, 1
  %93 = and i8 %92, 31
  store volatile i8 %93, ptr @g_tx_head, align 1, !tbaa !6
  %94 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %95 = add i8 %94, 1
  store volatile i8 %95, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  %96 = icmp eq i16 %79, 0
  br i1 %96, label %97, label %77, !llvm.loop !18

97:                                               ; preds = %87, %48, %73, %34
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_i32(i32 noundef %0) local_unnamed_addr #0 {
  tail call fastcc void @epic_serial_put_idec(i32 noundef %0) #6
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_hex8(i8 noundef zeroext %0) local_unnamed_addr #0 {
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
  %16 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %17 = icmp ugt i8 %16, 31
  br i1 %17, label %18, label %21

18:                                               ; preds = %3, %18
  tail call void @epic_dispatch_all_irqs() #5
  %19 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %20 = icmp ugt i8 %19, 31
  br i1 %20, label %18, label %21, !llvm.loop !9

21:                                               ; preds = %18, %3
  %22 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %23 = zext i8 %22 to i16
  %24 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %23
  store volatile i8 %15, ptr %24, align 1, !tbaa !6
  %25 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %26 = add i8 %25, 1
  %27 = and i8 %26, 31
  store volatile i8 %27, ptr @g_tx_head, align 1, !tbaa !6
  %28 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %29 = add i8 %28, 1
  store volatile i8 %29, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  %30 = icmp ugt i16 %4, 1
  br i1 %30, label %3, label %31, !llvm.loop !19

31:                                               ; preds = %21
  ret void
}

; Function Attrs: nounwind
define dso_local void @epic_serial_put_hex16(i16 noundef zeroext %0) local_unnamed_addr #0 {
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
  %16 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %17 = icmp ugt i8 %16, 31
  br i1 %17, label %18, label %21

18:                                               ; preds = %3, %18
  tail call void @epic_dispatch_all_irqs() #5
  %19 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %20 = icmp ugt i8 %19, 31
  br i1 %20, label %18, label %21, !llvm.loop !9

21:                                               ; preds = %18, %3
  %22 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %23 = zext i8 %22 to i16
  %24 = getelementptr inbounds nuw [32 x i8], ptr @g_tx_buf, i16 0, i16 %23
  store volatile i8 %15, ptr %24, align 1, !tbaa !6
  %25 = load volatile i8, ptr @g_tx_head, align 1, !tbaa !6
  %26 = add i8 %25, 1
  %27 = and i8 %26, 31
  store volatile i8 %27, ptr @g_tx_head, align 1, !tbaa !6
  %28 = load volatile i8, ptr @g_tx_count, align 1, !tbaa !6
  %29 = add i8 %28, 1
  store volatile i8 %29, ptr @g_tx_count, align 1, !tbaa !6
  tail call void @EPIC_IRQ_Enable(i16 noundef 9) #5
  %30 = icmp ugt i16 %4, 1
  br i1 %30, label %3, label %31, !llvm.loop !19

31:                                               ; preds = %21
  ret void
}

attributes #0 = { nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { nofree norecurse nounwind memory(readwrite, argmem: none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #3 = { nofree norecurse nounwind memory(readwrite, argmem: write) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #4 = { mustprogress nofree norecurse nounwind willreturn memory(readwrite, argmem: none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #5 = { nobuiltin nounwind "no-builtins" }
attributes #6 = { nobuiltin "no-builtins" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{!"clang version 20.1.8"}
!2 = !{!3, !3, i64 0}
!3 = !{!"int", !4, i64 0}
!4 = !{!"omnipotent char", !5, i64 0}
!5 = !{!"Simple C/C++ TBAA"}
!6 = !{!4, !4, i64 0}
!7 = !{!8, !8, i64 0}
!8 = !{!"any pointer", !4, i64 0}
!9 = distinct !{!9, !10, !11}
!10 = !{!"llvm.loop.mustprogress"}
!11 = !{!"llvm.loop.unroll.disable"}
!12 = distinct !{!12, !10, !11}
!13 = distinct !{!13, !10, !11}
!14 = distinct !{!14, !10, !11}
!15 = distinct !{!15, !10, !11}
!16 = distinct !{!16, !10, !11}
!17 = distinct !{!17, !10, !11}
!18 = distinct !{!18, !10, !11}
!19 = distinct !{!19, !10, !11}
