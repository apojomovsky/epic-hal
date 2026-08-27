; ModuleID = 'epic-serial/examples/example_serial.c'
source_filename = "epic-serial/examples/example_serial.c"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

@.str = private unnamed_addr constant [34 x i8] c"epic-serial ready at 115200 8N1\0D\0A\00", align 1
@main.buf = internal global [8 x i8] zeroinitializer, align 1

; Function Attrs: noreturn nounwind
define dso_local noundef i16 @main() local_unnamed_addr #0 {
  tail call void @epic_serial_init(i32 noundef 20000000, i32 noundef 115200) #2
  tail call void @EPIC_IRQ_Restore(i8 noundef zeroext 1) #2
  tail call void @epic_serial_put_str(ptr noundef nonnull @.str) #2
  tail call void @epic_serial_flush() #2
  br label %1

1:                                                ; preds = %7, %0
  %2 = tail call i16 @epic_serial_available() #2
  %3 = icmp sgt i16 %2, 0
  br i1 %3, label %4, label %7

4:                                                ; preds = %1
  %5 = tail call i16 @epic_serial_read(ptr noundef nonnull @main.buf, i16 noundef 8) #2
  %6 = icmp sgt i16 %5, 0
  br i1 %6, label %8, label %7

7:                                                ; preds = %4, %8, %1
  br label %1, !llvm.loop !2

8:                                                ; preds = %4
  %9 = tail call i16 @epic_serial_write(ptr noundef nonnull @main.buf, i16 noundef %5) #2
  br label %7
}

declare dso_local void @epic_serial_init(i32 noundef, i32 noundef) local_unnamed_addr #1

declare dso_local void @EPIC_IRQ_Restore(i8 noundef zeroext) local_unnamed_addr #1

declare dso_local void @epic_serial_put_str(ptr noundef) local_unnamed_addr #1

declare dso_local void @epic_serial_flush() local_unnamed_addr #1

declare dso_local i16 @epic_serial_available() local_unnamed_addr #1

declare dso_local i16 @epic_serial_read(ptr noundef, i16 noundef) local_unnamed_addr #1

declare dso_local i16 @epic_serial_write(ptr noundef, i16 noundef) local_unnamed_addr #1

attributes #0 = { noreturn nounwind "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { nobuiltin nounwind "no-builtins" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{!"clang version 20.1.8"}
!2 = distinct !{!2, !3}
!3 = !{!"llvm.loop.unroll.disable"}
