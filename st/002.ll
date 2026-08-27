; ModuleID = 'pic16f87xa-hal/src/epiccc/pic16_isr_vector.c'
source_filename = "pic16f87xa-hal/src/epiccc/pic16_isr_vector.c"
target datalayout = "e-m:e-p:16:16-i32:16-i64:16-f32:16-f64:16-a:8-n8:16-S16"
target triple = "msp430"

@llvm.compiler.used = appending global [1 x ptr] [ptr @PIC16_IRQ_Handler], section "llvm.metadata"

; Function Attrs: noinline nounwind
define dso_local msp430_intrcc void @PIC16_IRQ_Handler() #0 {
  tail call void @epic_dispatch_all_irqs() #2
  ret void
}

declare dso_local void @epic_dispatch_all_irqs() local_unnamed_addr #1

attributes #0 = { noinline nounwind "interrupt"="0" "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #1 = { "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" }
attributes #2 = { nobuiltin nounwind "no-builtins" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 2}
!1 = !{!"clang version 20.1.8"}
