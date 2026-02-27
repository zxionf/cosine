// eslint.config.js
import pluginJs from '@eslint/js';
import pluginTs from '@typescript-eslint/eslint-plugin';
import parserTs from '@typescript-eslint/parser';

export default [
    {
        files: ['**/*.ts'],
        languageOptions: {
            parser: parserTs,
            ecmaVersion: 2020,
            sourceType: 'module',
            globals: {
                Thenable: 'readonly', // 声明 Thenable 为只读全局变量
            },
        },
        plugins: {
            '@typescript-eslint': pluginTs,
        },
        rules: {
            // 示例规则
            'no-console': 'warn',
            '@typescript-eslint/no-unused-vars': 'error',
        },
    },
    pluginJs.configs.recommended, // 继承推荐规则
];