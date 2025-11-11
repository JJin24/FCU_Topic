const eslintJs = require('@eslint/js');
const globals = require('globals');

module.exports = [
  // 忽略不檢查的檔案 (必須)
  {
    ignores: ['node_modules/', 'package-lock.json', '.env'],
  },
  
  // 設置規則
  {
    files: ['**/*.js'],
    
    languageOptions: {
      ecmaVersion: 2022,
      sourceType: 'commonjs',
      
      globals: {
        ...globals.node,
        ...globals.es2022, 
      }
    },

    // 設定規則
    rules: {
      'no-console': 'warn', 
      'no-unused-vars': 'warn',
      'semi': ['error', 'always'],
    }
  }
];
