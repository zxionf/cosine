// client/src/extension.ts
import * as path from 'path';
import * as vscode from 'vscode';
import {
    LanguageClient,
    LanguageClientOptions,
    ServerOptions,
    TransportKind
} from 'vscode-languageclient/node';

let client: LanguageClient;

export function activate(context: vscode.ExtensionContext) {
    // 服务器路径
    const serverModule = context.asAbsolutePath(
        path.join('server', 'out', 'server.js')
    );

    // 服务器调试选项
    const debugOptions = { execArgv: ['--nolazy', '--inspect=6009'] };

    // 服务器配置
    const serverOptions: ServerOptions = {
        run: { module: serverModule, transport: TransportKind.ipc },
        debug: {
            module: serverModule,
            transport: TransportKind.ipc,
            options: debugOptions
        }
    };

    // 客户端选项
    const clientOptions: LanguageClientOptions = {
        documentSelector: [{ scheme: 'file', language: 'xel' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.xel')
        },
        initializationOptions: {
            languageId: 'xel'
        }
    };

    // 创建客户端
    client = new LanguageClient(
        'xelLanguageServer',
        'Xel Language Server',
        serverOptions,
        clientOptions
    );

    // 启动客户端
    client.start();

    // 注册自定义命令
    context.subscriptions.push(
        vscode.commands.registerCommand('xel.showDocumentation', () => {
            vscode.window.showInformationMessage('Xel Language Support v0.1.0');
        })
    );

    // 注册状态栏项
    const statusBarItem = vscode.window.createStatusBarItem(
        vscode.StatusBarAlignment.Right,
        100
    );
    statusBarItem.text = "$(code) Xel";
    statusBarItem.tooltip = "Xel Language Support";
    statusBarItem.command = 'xel.showDocumentation';
    statusBarItem.show();
    context.subscriptions.push(statusBarItem);
}

export function deactivate(): Thenable<void> | undefined {
    if (!client) {
        return undefined;
    }
    return client.stop();
}