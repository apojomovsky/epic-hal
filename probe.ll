; ModuleID = 'llvm-link'
source_filename = "llvm-link"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

@main.banner = internal unnamed_addr constant [34 x i8] c"epic-serial ready at 115200 8N1\0D\0A\00", align 1
@main.buf = internal global [8 x i8] zeroinitializer, align 1

; Function Attrs: noreturn nounwind
define dso_local noundef i16 @main() local_unnamed_addr #0 {
  tail call void @epic_serial_init(i32 noundef 20000000, i32 noundef 115200) #2
  tail call void @EPIC_IRQ_Restore(i8 noundef zeroext 1) #2
  br label %1

1:                                                ; preds = %1, %0
  %2 = phi i16 [ 0, %0 ], [ %5, %1 ]
  %3 = getelementptr inbounds nuw [34 x i8], ptr @main.banner, i16 0, i16 %2
  %4 = load i8, ptr %3, align 1, !tbaa !2
  tail call void @epic_serial_put_char(i8 noundef zeroext %4) #2
  %5 = add nuw nsw i16 %2, 1
  %6 = icmp eq i16 %5, 33
  br i1 %6, label %7, label %1, !llvm.loop !5

7:                                                ; preds = %13, %1
  %8 = tail call i16 @epic_serial_available() #2
  %9 = icmp sgt i16 %8, 0
  br i1 %9, label %10, label %13

10:                                               ; preds = %7
  %11 = tail call i16 @epic_serial_read(ptr noundef nonnull @main.buf, i16 noundef 8) #2
  %12 = icmp sgt i16 %11, 0
  br i1 %12, label %14, label %13

13:                                               ; preds = %14, %10, %7
  br label %7, !llvm.loop !8

14:                                               ; preds = %10
  %15 = tail call i16 @epic_serial_write(ptr noundef nonnull @main.buf, i16 noundef %11) #2
  br label %13
}

declare dso_local void @epic_serial_init(i32 noundef, i32 noundef) local_unnamed_addr #1

declare dso_local void @EPIC_IRQ_Restore(i8 noundef zeroext) local_unnamed_addr #1

declare dso_local void @epic_serial_put_char(i8 noundef zeroext) local_unnamed_addr #1

declare dso_local i16 @epic_serial_available() local_unnamed_addr #1

declare dso_local i16 @epic_serial_read(ptr noundef, i16 noundef) local_unnamed_addr #1

declare dso_local i16 @epic_serial_write(ptr noundef, i16 noundef) local_unnamed_addr #1

attributes #0 = { noreturn nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { nobuiltin nounwind "no-builtins" }

!llvm.ident = !{!0}
!llvm.module.flags = !{!1}

!0 = !{!"clang version 20.1.8"}
!1 = !{i32 1, !"wchar_size", i32 2}
!2 = !{!3, !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C/C++ TBAA"}
!5 = distinct !{!5, !6, !7}
!6 = !{!"llvm.loop.mustprogress"}
!7 = !{!"llvm.loop.unroll.disable"}
!8 = distinct !{!8, !7}
