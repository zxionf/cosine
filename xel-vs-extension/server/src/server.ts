// server/src/server.ts
import {
    createConnection,
    TextDocuments,
    Diagnostic,
    DiagnosticSeverity,
    ProposedFeatures,
    // InitializeParams,
    CompletionItem,
    CompletionItemKind,
    // TextDocumentPositionParams,
    TextDocumentSyncKind,
    InitializeResult,
    Hover,
    Definition,
    Range,
    SymbolInformation,
    SymbolKind
} from 'vscode-languageserver/node';
import { TextDocument } from 'vscode-languageserver-textdocument';

// 创建连接
const connection = createConnection(ProposedFeatures.all);
const documents: TextDocuments<TextDocument> = new TextDocuments(TextDocument);

// 语言关键字
const KEYWORDS = new Set([
    'var', 'let', 'const', 'if', 'else', 'while', 'for',
    'func', 'return', 'import', 'export', 'class', 'struct'
]);

// 内置函数
const BUILTINS = new Map([
    ['print', 'Print to console'],
    ['len', 'Get length of string/array'],
    ['type', 'Get type of value'],
    ['parseInt', 'Parse string to integer'],
    ['toString', 'Convert to string']
]);

// 存储变量定义信息
interface VariableInfo {
    name: string;
    uri: string;
    range: Range;
    type?: string;
    documentation?: string;
}

const variables: Map<string, VariableInfo[]> = new Map();

connection.onInitialize((): InitializeResult => {
    return {
        capabilities: {
            textDocumentSync: TextDocumentSyncKind.Incremental,
            completionProvider: {
                resolveProvider: true,
                triggerCharacters: ['.', ':']
            },
            hoverProvider: true,
            definitionProvider: true,
            documentSymbolProvider: true,
            workspaceSymbolProvider: true,
            referencesProvider: true,
            renameProvider: true
        }
    };
});

// 文档变化时分析
documents.onDidChangeContent(change => {
    validateDocument(change.document);
    analyzeDocument(change.document);
});

// 验证文档（错误检查）
function validateDocument(document: TextDocument): void {
    const text = document.getText();
    const diagnostics: Diagnostic[] = [];
    const lines = text.split('\n');

    // 1. 检查未闭合的字符串
    let inString = false;
    let stringStart = 0;
    
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        
        for (let j = 0; j < line.length; j++) {
            if (line[j] === '"' && (j === 0 || line[j-1] !== '\\')) {
                if (!inString) {
                    inString = true;
                    stringStart = j;
                } else {
                    inString = false;
                }
            }
        }
        
        if (inString) {
            diagnostics.push({
                severity: DiagnosticSeverity.Error,
                range: {
                    start: { line: i, character: stringStart },
                    end: { line: i, character: line.length }
                },
                message: 'Unclosed string literal',
                source: 'xel'
            });
        }
    }

    // 2. 检查未声明的变量
    const declaredVars = new Set<string>();
    const usedVars = new Set<string>();
    
    // 简单解析变量声明
    const varDeclRegex = /\b(var|let|const)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[=;]/g;
    const varUseRegex = /\b([a-zA-Z_][a-zA-Z0-9_]*)\b(?!\s*[=])/g;
    
    let match;
    while ((match = varDeclRegex.exec(text)) !== null) {
        declaredVars.add(match[2]);
    }
    
    while ((match = varUseRegex.exec(text)) !== null) {
        const varName = match[1];
        if (!KEYWORDS.has(varName) && !BUILTINS.has(varName)) {
            usedVars.add(varName);
        }
    }
    
    for (const usedVar of usedVars) {
        if (!declaredVars.has(usedVar)) {
            const index = text.indexOf(usedVar);
            const line = text.substring(0, index).split('\n').length - 1;
            const column = index - text.lastIndexOf('\n', index) - 1;
            
            diagnostics.push({
                severity: DiagnosticSeverity.Warning,
                range: {
                    start: { line, character: column },
                    end: { line, character: column + usedVar.length }
                },
                message: `Variable '${usedVar}' is not declared`,
                source: 'xel'
            });
        }
    }

    connection.sendDiagnostics({ uri: document.uri, diagnostics });
}

// 分析文档中的变量定义
function analyzeDocument(document: TextDocument): void {
    const text = document.getText();
    const uri = document.uri;
    
    // 清空该文件的旧变量定义
    for (const [name, infos] of variables) {
        variables.set(name, infos.filter(info => info.uri !== uri));
    }
    
    // 查找变量定义
    const varDeclRegex = /\b(var|let|const)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[=;]/g;
    let match;
    
    while ((match = varDeclRegex.exec(text)) !== null) {
        const varName = match[2];
        const startPos = document.positionAt(match.index + match[0].indexOf(varName));
        const endPos = document.positionAt(match.index + match[0].indexOf(varName) + varName.length);
        
        const info: VariableInfo = {
            name: varName,
            uri: uri,
            range: Range.create(startPos, endPos),
            type: 'variable'
        };
        
        if (!variables.has(varName)) {
            variables.set(varName, []);
        }
        variables.get(varName)!.push(info);
    }
}

// 代码补全
connection.onCompletion((): CompletionItem[] => {
    const completions: CompletionItem[] = [];
    
    // 1. 添加关键字
    for (const keyword of KEYWORDS) {
        completions.push({
            label: keyword,
            kind: CompletionItemKind.Keyword,
            detail: 'keyword',
            documentation: `${keyword} - language keyword`
        });
    }
    
    // 2. 添加内置函数
    for (const [name, desc] of BUILTINS) {
        completions.push({
            label: name,
            kind: CompletionItemKind.Function,
            detail: 'built-in',
            documentation: desc
        });
    }
    
    // 3. 添加变量（来自所有文件）
    for (const [varName, infos] of variables) {
        const locations = infos.map(info => 
            `${info.uri.split('/').pop()}:${info.range.start.line + 1}`
        ).join(', ');
        
        completions.push({
            label: varName,
            kind: CompletionItemKind.Variable,
            detail: 'variable',
            documentation: `Defined in: ${locations}`
        });
    }
    
    return completions;
});

// 悬停提示
connection.onHover((params): Hover | null => {
    const document = documents.get(params.textDocument.uri);
    if (!document) return null;
    
    // const position = params.position;
    // const wordRange = document.getWordRangeAtPosition(position);
    // if (!wordRange) return null;
    
    // const word = document.getText(wordRange);
    
    // // 检查是否是关键字
    // if (KEYWORDS.has(word)) {
    //     return {
    //         contents: {
    //             kind: 'markdown',
    //             value: `**${word}**\n\nLanguage keyword`
    //         }
    //     };
    // }
    
    // // 检查是否是内置函数
    // if (BUILTINS.has(word)) {
    //     return {
    //         contents: {
    //             kind: 'markdown',
    //             value: `**${word}**\n\nBuilt-in function\n\n${BUILTINS.get(word)}`
    //         }
    //     };
    // }
    
    // // 检查是否是变量
    // if (variables.has(word)) {
    //     const infos = variables.get(word)!;
    //     const locations = infos.map(info => 
    //         `- ${info.uri.split('/').pop()}: line ${info.range.start.line + 1}`
    //     ).join('\n');
        
    //     return {
    //         contents: {
    //             kind: 'markdown',
    //             value: `**${word}**\n\nVariable\n\nDefined in:\n${locations}`
    //         }
    //     };
    // }
    
    return null;
});

// 定义跳转
connection.onDefinition((params): Definition | null => {
    const document = documents.get(params.textDocument.uri);
    if (!document) return null;
    
    // const position = params.position;
    // const wordRange = document.getWordRangeAtPosition(position);
    // if (!wordRange) return null;
    
    // const word = document.getText(wordRange);
    
    // if (variables.has(word)) {
        // const infos = variables.get(word)!;
        // 返回第一个定义位置
        // if (infos.length > 0) {
            // return {
                // uri: infos[0].uri,
                // range: infos[0].range
            // };
        // }
    // }
    
    return null;
});

// 文档符号
connection.onDocumentSymbol((params): SymbolInformation[] => {
    const document = documents.get(params.textDocument.uri);
    if (!document) return [];
    
    const symbols: SymbolInformation[] = [];
    const text = document.getText();
    
    // 查找函数定义
    const funcRegex = /\bfunc\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(/g;
    let match;
    
    while ((match = funcRegex.exec(text)) !== null) {
        const funcName = match[1];
        const startPos = document.positionAt(match.index);
        const endPos = document.positionAt(match.index + match[0].length);
        
        symbols.push({
            name: funcName,
            kind: SymbolKind.Function,
            location: {
                uri: document.uri,
                range: Range.create(startPos, endPos)
            }
        });
    }
    
    // 查找变量定义
    const varRegex = /\b(var|let|const)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*[=;]/g;
    
    while ((match = varRegex.exec(text)) !== null) {
        const varName = match[2];
        const startPos = document.positionAt(match.index);
        const endPos = document.positionAt(match.index + match[0].length);
        
        symbols.push({
            name: varName,
            kind: SymbolKind.Variable,
            location: {
                uri: document.uri,
                range: Range.create(startPos, endPos)
            }
        });
    }
    
    return symbols;
});

documents.listen(connection);
connection.listen();