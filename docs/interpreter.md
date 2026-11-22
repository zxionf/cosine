# XEL 解释器

脚本语言 XEL 的解释器，位于 `interpreter/`，与 UI 解耦，M5 起作为可嵌入库接入动画内核。

## 结构

```
interpreter/
├── token.*              词法符号定义与词法分析（lexer）
├── lexer.*              词法分析器
├── ast/                 AST 节点
│   ├── expr.*           表达式
│   ├── stmt.*           语句
│   ├── callable.h       可调用对象（函数）
│   └── printer.h        AST 打印
├── parser/parser.*      递归下降语法分析
├── evaluator/evaluator.* 树递归下降求值
├── environment/environment.* 作用域与变量存储
├── xel_error.h          错误类型
├── var.h                运行时值
└── main.cpp             CLI 入口（作为独立可执行）
```

## 能力

- 词法：标识符、数字、字符串、运算符。
- 语法：表达式、语句、函数声明/调用、native 函数。
- 求值：四则运算、比较、变量、作用域、分支、循环。
- 测试：`test_interpreter_token` / `test_intepreter_tree_structure` / `test_intepreter_tree_evaluator`。
