const pool = require('./database');

async function getXDayAIP(xDay, IP){
  var conn;
  try {
    conn = await pool.getConnection();
    const flow = conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, alert_history.status \
      FROM flow \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      WHERE `timestamp` >= NOW() - INTERVAL ? DAY \
      AND (src_ip = ? OR dst_ip = ?);",
      [xDay, IP, IP]
    );

    console.log(flow);
    return flow;
  }
  catch (err) {
    console.error('Error in getXDayAIP', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getXDayIPCount(xDay){
  var conn;
  try {
    conn = await pool.getConnection();
    const flow = conn.query(
      "SELECT ip_address, COUNT(*) AS occurrence_count \
      FROM ( \
      SELECT src_ip AS ip_address \
      FROM flow \
      WHERE `timestamp` >= NOW() - INTERVAL ? DAY \
      \
      UNION ALL \
      \
      SELECT dst_ip AS ip_address \
      FROM flow \
      WHERE `timestamp` >= NOW() - INTERVAL ? DAY \
      ) \
      AS combined_ips GROUP BY ip_address \
      ORDER BY occurrence_count DESC;",
      [xDay, xDay]
    );
    console.log(flow);
    return flow;
  }
  catch (err) {
    console.error('Error in getXDayIPCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getXDayGoodMalCount(xDay){
  var conn;
  try {
    conn = await pool.getConnection();
    const flow = conn.query(
      "SELECT COUNT(CASE WHEN AH.ID IS NULL THEN F.ID END) AS goodFlowCount, COUNT(AH.ID) AS badFlowCount \
      FROM flow F \
      LEFT JOIN alert_history AH ON F.ID = AH.ID \
      WHERE F.TIMESTAMP >= NOW() - INTERVAL ? DAY;",
      [xDay]
    );

    console.log(flow);
    return flow;
  }
  catch (err) {
    console.error('Error in getXDayAIP', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getXDayAllCount(xDay){
  var conn;
  try {
    conn = await pool.getConnection();

    
  }
  catch (err) {
    console.error('Error in getXDayAllCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getXDayProtocolCount(xDay){

};

async function getXDayHostAllCount(xDay){
  var conn;
  try {
    conn = await pool.getConnection();
    const flow = conn.query(
      "SELECT host.`name` AS hostName, host.ip, gateway, \
      COALESCE(ip_counts.occurrence_count, 0) AS totalFlowCount \
      FROM host \
      LEFT JOIN( \
        SELECT ip_address, COUNT(*) AS occurrence_count \
        FROM ( \
          SELECT src_ip AS ip_address \
          FROM flow \
          WHERE `timestamp` >= NOW() - INTERVAL ? DAY \
          \
          UNION ALL \
          \
          SELECT dst_ip AS ip_address \
          FROM flow \
          WHERE `timestamp` >= NOW() - INTERVAL ? DAY \
        ) AS combined_ips \
        GROUP BY ip_address \
      ) AS ip_counts ON host.ip = ip_counts.ip_address;",
      [xDay, xDay]
    );
    console.log(flow);
    return flow;
  }
  catch (err) {
    console.error('Error in getXDayHostAllCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getXDayHostMalCount(xDay){
  var conn;
  try {
    conn = await pool.getConnection();
    const flow = conn.query(
      "SELECT host.`name` AS hostName, host.ip, gateway, \
      COALESCE(ip_counts.occurrence_count, 0) AS badFlowCount \
      FROM host \
      LEFT JOIN( \
        SELECT ip_address, COUNT(*) AS occurrence_count \
        FROM ( \
          SELECT flow.src_ip AS ip_address FROM alert_history \
          LEFT JOIN flow ON alert_history.id = flow.id \
          WHERE `timestamp` >= NOW() - INTERVAL ? DAY \
          UNION ALL \
          SELECT flow.src_ip AS ip_address FROM alert_history \
          LEFT JOIN flow ON alert_history.id = flow.id \
          WHERE `timestamp` >= NOW() - INTERVAL ? DAY \
        ) AS combined_ips \
        GROUP BY ip_address \
      ) AS ip_counts ON host.ip = ip_counts.ip_address;",
      [xDay, xDay]
    );
    console.log(flow);
    return flow;
  }
  catch (err) {
    console.error('Error in getXDayHostAllCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getXDayHostAttackCount(xDay){

}

module.exports = {
    getXDayAIP,
    getXDayIPCount,
    getXDayGoodMalCount,
    getXDayAllCount,
    getXDayProtocolCount,
    getXDayHostAllCount,
    getXDayHostMalCount,
    getXDayHostAttackCount
};