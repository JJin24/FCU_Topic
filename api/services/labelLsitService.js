const pool = require('./database');

async function getAllLabelList() {
  var conn;
  try{
    conn = await pool.getConnection();

    const label_list = await conn.query(
      "SELECT * FROM label_list"
    );
    console.log(label_list);
    return label_list;
  }
  catch(err){
    console.error('Error in getAllLabelList', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getLabelNameByID(id){
  var conn;

  try{
    conn = await pool.getConnection();

    const labelName = await conn.query(
      "SELECT name FROM label_list WHERE label_id = ?", [id]
    );
    console.log(labelName);
    return labelName;
  }
  catch(err){
    console.error('Error in getLabelNameByID', err);
  }
  finally{
    if (conn) conn.release();
  }
}

/**
 * 獲取指定地點「近一小時」收到的「攻擊類型列表」(不重複)
 * @param {string} location - 要篩選的裝置地點 (例如: "資電大樓")
 * @returns {Promise<Array>} - 攻擊類型陣列 (例如 [ { "label": 3, "label_name": "DoS_GoldenEye" }, ... ])
 * @throws {Error} - 如果 location 缺失或資料庫查詢失敗
 */
async function getAttackTypesByLocation(location) {
  var conn;
  try {
    // 1. 驗證 location 參數
    if (!location) {
        const error = new Error("缺少 'location' 查詢參數");
        error.isValidationError = true;
        throw error;
    }
    
    conn = await pool.getConnection();

    // 2. 這是您的新 SQL 查詢
    const sql = `
      SELECT
          DISTINCT ah.label,
          ll.name AS label_name
      FROM
          flow AS f
      JOIN
          alert_history AS ah ON f.id = ah.id
      JOIN
          host AS h ON f.dst_ip = h.ip
      JOIN
          label_list AS ll ON ah.label = ll.label_id
      WHERE
          f.timestamp >= NOW() - INTERVAL 1 HOUR
          AND h.location = ?
      ORDER BY
          ah.label;
    `;
    
    // 3. 準備參數
    const params = [ location ];
    
    // 4. 執行查詢
    const rows = await conn.query(sql, params); 

    console.log(`[Service] 成功獲取 ${location} 在近一小時的攻擊類型列表。`);
    console.log(rows); // <-- 這將回傳 [ { "label": 3, "label_name": "DoS_GoldenEye" }, ... ]
    return rows;

  }
  catch(err){
    console.error('Error in getAttackTypesByLocation', err);
    throw err; 
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getAttackTypesByLocation(location) {
  var conn;
  try {
    // 1. 驗證 location 參數
    if (!location) {
        const error = new Error("缺少 'location' 查詢參數");
        error.isValidationError = true;
        throw error;
    }
    
    conn = await pool.getConnection();

    // 2. 這是您的新 SQL 查詢
    const sql = `
      SELECT
          ah.label,
          ll.name AS label_name,
          COUNT(*) AS count
      FROM
          flow AS f
      JOIN
          alert_history AS ah ON f.id = ah.id
      JOIN
          host AS h ON f.dst_ip = h.ip
      JOIN
          label_list AS ll ON ah.label = ll.label_id
      WHERE
          f.timestamp >= '2025-09-07 17:45:30'
          AND h.location = ?
      GROUP BY
          ah.label, ll.name
      ORDER BY
          count DESC;
    `;
    
    // 3. 準備參數
    const params = [ location ];
    
    // 4. 執行查詢
    const rows = await conn.query(sql, params); 

    console.log(`[Service] 成功獲取 ${location} 在近一小時的攻擊類型列表。`);
    console.log(rows); // <-- 這將回傳 [ { "label": 3, "label_name": "DoS_GoldenEye" }, ... ]
    return rows;

  }
  catch(err){
    console.error('Error in getAttackTypesByLocation', err);
    throw err; 
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

module.exports = {
  getAllLabelList,
  getLabelNameByID,
  getAttackTypesByLocation
};