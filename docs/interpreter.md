file.xel

token       // 定义符号

lexer       // 词法分析，生成符号

ast         // 抽象语法树

parser      // 语法分析

parse       // 解析

object      // 对象->值类型

evaluator   // 评估->用来进行树递归下降求值

```
interpreter/
|---token.h         // 符号定义
|---lexer.h         // 词法分析，生成符号
|---ast/            // 抽象语法树
|   |---node.h      // 抽象语法树节点定义
|---parser/         // 语法分析
|   |---parser.h    // 解析
|---object/         // 对象->值类型
|   |---object.h    // 值类型定义
|---evaluator/      // 用来进行树递归下降求值
|   |---evaluator.h // 树递归下降求值
```