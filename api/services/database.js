const mariadb = require('mariadb');
require('dotenv').config();

// 建立資料庫的 Pool
// 其中由於 id 為 BIGINT
// 所以需要設定 supportBigNumbers 以及 bigNumberStrings
// 來避免資料過大無法正確取得
// multipleStatements 則是允許在同一個 query 中執行多個 SQL 指令
const pool = mariadb.createPool({
  host: process.env.DBHOST,
  port: process.env.DBPORT,
  user: process.env.DBUSER,
  password: process.env.DBPASSWORD,
  database: process.env.DBDATABASE,
  supportBigNumbers: true,
  bigNumberStrings: true,
  multipleStatements: true
});

// 開放 pool 讓其他檔案可以使用
module.exports = pool;