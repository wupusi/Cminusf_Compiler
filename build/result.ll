; ModuleID = 'calculator'
declare void @output(i32)

define i32 @main() {
label_entry:
  %op0 = add i32 2, 5
  call void @output(i32 %op0)
  ret i32 0
}
