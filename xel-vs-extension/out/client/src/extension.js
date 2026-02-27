"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
exports.deactivate = exports.activate = void 0;
// client/src/extension.ts
const path = require("path");
const vscode = require("vscode");
const node_1 = require("vscode-languageclient/node");
let client;
function activate(context) {
    // 服务器路径
    const serverModule = context.asAbsolutePath(path.join('server', 'out', 'server.js'));
    // 服务器调试选项
    const debugOptions = { execArgv: ['--nolazy', '--inspect=6009'] };
    // 服务器配置
    const serverOptions = {
        run: { module: serverModule, transport: node_1.TransportKind.ipc },
        debug: {
            module: serverModule,
            transport: node_1.TransportKind.ipc,
            options: debugOptions
        }
    };
    // 客户端选项
    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'xel' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.xel')
        },
        initializationOptions: {
            languageId: 'xel'
        }
    };
    // 创建客户端
    client = new node_1.LanguageClient('xelLanguageServer', 'Xel Language Server', serverOptions, clientOptions);
    // 启动客户端
    client.start();
    // 注册自定义命令
    context.subscriptions.push(vscode.commands.registerCommand('xel.showDocumentation', () => {
        vscode.window.showInformationMessage('Xel Language Support v0.1.0');
    }));
    // 注册状态栏项
    const statusBarItem = vscode.window.createStatusBarItem(vscode.StatusBarAlignment.Right, 100);
    statusBarItem.text = "$(code) Xel";
    statusBarItem.tooltip = "Xel Language Support";
    statusBarItem.command = 'xel.showDocumentation';
    statusBarItem.show();
    context.subscriptions.push(statusBarItem);
}
exports.activate = activate;
function deactivate() {
    if (!client) {
        return undefined;
    }
    return client.stop();
}
exports.deactivate = deactivate;
//# sourceMappingURL=extension.js.map