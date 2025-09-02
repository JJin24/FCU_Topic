const { get } = require('../routes/hostRoutes');
const pool = require('./database');

// getFlow 會取得 flow 資料表的後 1000 筆資料
// 並且使會取得 uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol
async function getAllFlow() {
  var conn;
  try{
    conn = await pool.getConnection();

    // 取得資料庫的資料
    const rows = await conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, status \
      FROM flow \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id  \
      ORDER BY flow.id ASC;"
    );
    console.log(rows);
    return rows;
  }
  catch(err){
    console.error('Error in getFlow', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getFlowByUUID(uuid) {
  var conn;
  try{
    conn = await pool.getConnection();

    const [rows] = await conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, status \
      FROM flow \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id  \
      WHERE flow.uuid = ? \
      ORDER BY flow.id ASC \;",
      [uuid]
    );

    console.log(rows); // 這裡會印出查詢到的資料陣列
    return rows;       // 4. 回傳真正的查詢結果
  }
  catch(err){
    console.error('Error in getAFlow', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getFlowByIP(ip){
  var conn;
  try{
    conn = await pool.getConnection();

    const ipFlow = await conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, status \
      FROM flow \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id  \
      FROM flow, alert_history WHERE src_ip = ? OR dst_ip = ?",
      [ip, ip]
    )
    console.log(ipFlow);
    return ipFlow;
  }
  catch(err){
    console.error('Error in getIP', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getFlowProtocolCount(){
  var conn;
  try{
    conn = await pool.getConnection();

    const protocolCount = await conn.query(
      "SELECT protocol_list.name AS protocol, COUNT(flow.protocol) as count FROM flow LEFT JOIN protocol_list \
      ON flow.protocol = protocol_list.protocol GROUP BY protocol_list.protocol"
    )
    console.log(protocolCount);
    return protocolCount;
  }
  catch(err){
    console.error('Error in getProtocolCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getTopXDayFlow(x){
  var conn;
  try{
    conn = await pool.getConnection();

    const topXDayFlow = await conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, status \
      FROM flow \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id  \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      WHERE `timestamp` >= NOW() - INTERVAL ? DAY ORDER BY timestamp ASC;",
      [x]
    )
    console.log(topXDayFlow);
    return topXDayFlow;
  }
  catch(err){
    console.error('Error in getTopXDayFlow', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

module.exports = {
  getAllFlow,
  getFlowByUUID,
  getFlowByIP,
  getFlowProtocolCount,
  getTopXDayFlow
};
