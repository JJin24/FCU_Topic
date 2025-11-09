const { get } = require('../routes/flowRoutes');
const pool = require('./database');

async function getAllHost() {
  var conn;
  try{
    conn = await pool.getConnection();

    const host = await conn.query(
      "SELECT * FROM host"
    )
    console.log(host);
    return host;
  }
  catch(err){
    console.error('Error in getHost', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getHostByIP(ip) {
  var conn;
  try{
    conn = await pool.getConnection();

    const Host = await conn.query(
      "SELECT * FROM host WHERE ip = ?",
      [ip]
    )
    console.log(Host);
    return Host;
  }
  catch(err){
    console.error('Error in getHostByIP', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getHostNameByIP(ip) {
  var conn;
  try{
    conn = await pool.getConnection();

    const ipHost = await conn.query(
      "SELECT name FROM host WHERE ip = ?",
      [ip]
    )
    console.log(ipHost);
    return ipHost;
  }
  catch(err){
    console.error('Error in getHostNameByIP', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getHostStatus() {
  console.log("!!!!!!!!!! EXECUTING getHostStatus in hostService.js !!!!!!!!!!");
  var conn;
  try{
    conn = await pool.getConnection();

    const hostStatus = await conn.query(
      "SELECT location, \
      SUM(CASE WHEN status = 0 THEN 1 ELSE 0 END) AS nornal, \
      SUM(CASE WHEN status = 1 THEN 1 ELSE 0 END) AS warn, \
      SUM(CASE WHEN status = 2 THEN 1 ELSE 0 END) AS alert \
      FROM host GROUP BY location;"
    )
    console.log(hostStatus);
    return hostStatus;
  }
  catch(err){
    console.error('Error in getHostStatus', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};


async function getBuildingList() {
  var conn;
  try{
    conn = await pool.getConnection();

    const ipHost = await conn.query(`
      SELECT
        location
      FROM
        host
      GROUP BY
        location;`
    )
    console.log(ipHost);
    return ipHost;
  }
  catch(err){
    console.error('Error in getBuildingList', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function postNewDevice(deviceData) {
  var conn;
  try {
    conn = await pool.getConnection();

    const result = await conn.query(
      "INSERT INTO host (name, location, ip, gateway, mac_addr, status, department, importance) VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
      [
        deviceData.name,
        deviceData.location,
        deviceData.ip,
        deviceData.gateway,
        deviceData.mac_addr,
        deviceData.status,
        deviceData.department,
        deviceData.importance
      ]
    );
    console.log("New device added:", result);
    return result.insertId;
  }
  catch (err) {
    console.error('Error in postNewDevice', err);
    throw err;
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getHostNameByBuilding(building) {
  var conn;
  try{
    conn = await pool.getConnection();

    const host = await conn.query(`
      SELECT
        location, name AS HostName
      FROM
        host
      where
        location = ?;`,
      [building]
    )
    console.log(host);
    return host;
  }
  catch(err){
    console.error('Error in getHostNameByIP', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

/* From Gemini */
async function getSearchHistory(building, host_names, start_time, end_time, final_alert_ids, is_good_requested) {
  var conn;
  try{
    conn = await pool.getConnection();

    const result = await conn.query(`
      WITH TargetHosts AS (
        -- 1. 根據 building 和 host 列表，找出所有相關的主機 IP、名稱及位置
        SELECT
          ip,
          name AS host_name,
          location
        FROM host
        WHERE
          location = ?            -- [1] building
          AND name IN (?)         -- [2] host_names 陣列
      ),
      FilteredFlows AS (
        -- 2. 過濾 flow 紀錄：時間範圍內 AND (src_ip OR dst_ip 匹配任一 TargetHost IP)
        SELECT
          f.*
        FROM flow f
        INNER JOIN TargetHosts th ON f.src_ip = th.ip OR f.dst_ip = th.ip
        WHERE
          f.timestamp BETWEEN ? AND ? -- [3] start_time, [4] end_time
      )

      -- --- 3. 壞流量 (Alerts) 查詢 ---
      SELECT
        th.host_name,
        th.location,
        ff.timestamp,
        ff.src_ip,
        ff.dst_ip,
        pl.name AS protocol_name,
        ll.name AS label, -- 修正: 將 ll.name 的別名設為 label
        -- 將 status 欄位轉換為中文描述
        CASE ah.status
          WHEN 0 THEN '尚未處理'
          WHEN 1 THEN '已處理'
          ELSE '未知'
        END AS status,
        CAST(ah.score AS CHAR) AS score -- 告警分數
      FROM FilteredFlows ff
      INNER JOIN alert_history ah ON ff.id = ah.id -- 僅選取有告警紀錄的流量
      INNER JOIN protocol_list pl ON ff.protocol = pl.protocol
      INNER JOIN label_list ll ON ah.label = ll.label_id
      INNER JOIN TargetHosts th ON ff.src_ip = th.ip OR ff.dst_ip = th.ip
      WHERE
        ll.name IN (?)

      UNION ALL

      -- --- 4. 好流量 (Good Flows) 查詢 ---
      SELECT
        th.host_name,
        th.location,
        ff.timestamp,
        ff.src_ip,
        ff.dst_ip,
        pl.name AS protocol_name,
        'Good' AS label,
        '正常' AS status,
        '-' AS score -- Good 流量無分數
      FROM FilteredFlows ff
      LEFT JOIN alert_history ah ON ff.id = ah.id
      INNER JOIN protocol_list pl ON ff.protocol = pl.protocol
      INNER JOIN TargetHosts th ON ff.src_ip = th.ip OR ff.dst_ip = th.ip
      WHERE
        ah.id IS NULL           -- 確保該流量沒有在 alert_history 中
        AND ? = 1               -- [6] is_good_requested (僅在請求 Good 流量時啟用)
      ;
   `, [building, host_names, start_time, end_time, final_alert_ids, is_good_requested]
    );
    console.log(result);
    return result;
  }
  catch(err){
    console.error('Error in getSearchHistory', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getHourlyFlowCountByLocationAndHost() {
  var conn;
  try{

    conn = await pool.getConnection();

    // 2. SQL 查詢：將 '資電大樓' 替換為 '?'
    const sql = `
      SELECT
          h.name,
          h.location,
          h.ip,
          h.importance,
          ah.label AS attack_label,
          COUNT(*) AS attack_count
      FROM
          flow AS f
      JOIN
          alert_history AS ah ON f.id = ah.id
      JOIN
          host AS h ON f.dst_ip = h.ip
      WHERE
          f.timestamp >= NOW() - INTERVAL 1 HOUR
      GROUP BY
          h.name, h.location, h.ip, h.importance, ah.label
      ORDER BY
          attack_count DESC;
    `;

    // 3. 執行查詢：將 [location] 變數作為第二個參數傳入
    const host = await conn.query(sql);
    console.log(host);
    return host;
  }
  catch(err){
    console.error('Error in getFlowCountByLocationAndHost', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getAllFlowCountByLocationAndHost() {
  var conn;
  try{

    conn = await pool.getConnection();

    // 2. SQL 查詢：將 '資電大樓' 替換為 '?'
    const sql = `
      SELECT
          h.name,
          h.location,
          h.ip,
          h.importance,
          ah.label AS attack_label,
          COUNT(*) AS attack_count
      FROM
          flow AS f
      JOIN
          alert_history AS ah ON f.id = ah.id
      JOIN
          host AS h ON f.dst_ip = h.ip
      WHERE
          f.timestamp >= '2025-09-07 17:45:30'
      GROUP BY
          h.name, h.location, h.ip, h.importance, ah.label
      ORDER BY
          attack_count DESC;
    `;

    // 3. 執行查詢：將 [location] 變數作為第二個參數傳入
    const host = await conn.query(sql);
    console.log(host);
    return host;
  }
  catch(err){
    console.error('Error in getFlowCountByLocationAndHost', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

module.exports = {
  getAllHost,
  getHostByIP,
  getHostNameByIP,
  getHostStatus,
  postNewDevice,
  getHostStatus,
  postNewDevice,
  getHostStatus,
  getBuildingList,
  getHostNameByBuilding,
  getSearchHistory,
  getHourlyFlowCountByLocationAndHost,
  getAllFlowCountByLocationAndHost
};

