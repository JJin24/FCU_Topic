const { post } = require('../routes/hostRoutes');
const pool = require('./database');

async function getAllAlert() {
  var conn;
  try{
    conn = await pool.getConnection();

    const alert_history = await conn.query(
      "WITH RelevantHostIPs AS ( \
          SELECT src_ip AS ip FROM flow \
          UNION \
          SELECT dst_ip AS ip FROM flow \
      ), \
      HostIPToNameMap AS ( \
          SELECT rh.ip, host.name AS hostname \
          FROM RelevantHostIPs AS rh \
          INNER JOIN host ON rh.ip = host.ip \
      ) \
      SELECT flow.uuid, COALESCE(h_src.hostname, h_dst.hostname) AS hostname, \
      flow.timestamp, flow.src_ip, flow.src_port, flow.dst_ip, flow.dst_port, \
      protocol_list.name AS protocol, label_list.name AS label, ah.score, ah.status \
      FROM alert_history AS ah \
      LEFT JOIN flow ON ah.id = flow.id \
      LEFT JOIN label_list ON ah.label = label_list.label_id \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      LEFT JOIN HostIPToNameMap AS h_src ON flow.src_ip = h_src.ip \
      LEFT JOIN HostIPToNameMap AS h_dst ON flow.dst_ip = h_dst.ip \
      WHERE ah.status = 0;"
    );
    console.log(alert_history);
    return alert_history;;
  }
  catch(err){
    console.error('Error in getAlert', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function getNotDealAlert(){
  var conn;
  try{
    conn = await pool.getConnection();

    const topXDayFlow = await conn.query(
      "WITH RelevantHostIPs AS ( \
      SELECT src_ip AS ip FROM flow \
      UNION \
      SELECT dst_ip AS ip FROM flow \
      ), \
      HostIPToNameMap AS ( \
          SELECT rh.ip, host.name AS hostname \
          FROM RelevantHostIPs AS rh \
          INNER JOIN host ON rh.ip = host.ip \
      ) \
      SELECT flow.uuid, COALESCE(h_src.hostname, h_dst.hostname) AS hostname, \
      flow.timestamp, flow.src_ip, flow.src_port, flow.dst_ip, flow.dst_port, \
      protocol_list.name AS protocol, label_list.name AS label, ah.score, ah.status \
      FROM flow \
      LEFT JOIN alert_history AS ah ON flow.id = ah.id \
      LEFT JOIN label_list ON ah.label = label_list.label_id \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      LEFT JOIN HostIPToNameMap AS h_src ON flow.src_ip = h_src.ip \
      LEFT JOIN HostIPToNameMap AS h_dst ON flow.dst_ip = h_dst.ip \
      WHERE ah.status = 0;"
    );
    console.log(topXDayFlow);
    return topXDayFlow;
  }
  catch(err){
    console.error('Error in getTopXDayFlow', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

async function postNewAlert(alertData) {
  var conn;

  try{
    conn = await pool.getConnection();

    const timestamp = alertData.timestamp || new Date().toISOString();
    const src_ip = alertData.src_ip;
    const src_port = alertData.src_port;
    const dst_ip = alertData.dst_ip;
    const dst_port = alertData.dst_port;
    const protocol = alertData.protocol;
    const label = alertData.label;
    const score = alertData.score;
    const pcap = alertData.pcap || null;

    const result = await conn.query(
      "INSERT INTO flow (flow.timestamp, flow.src_ip, flow.src_port, flow.dst_ip, flow.dst_port, flow.protocol) \
       VALUES (?, ?, ?, ?, ?, ?); \
       INSERT INTO alert_history (id, pcap, score, label) \
       VALUES (LAST_INSERT_ID(), ?, ?, ?);",
      [timestamp, src_ip, src_port, dst_ip, dst_port, protocol, pcap, score, label]
    );
    console.log(result);
    return result;
  }
  catch(err){
    console.error('Error in postNewAlert', err);
    return null;
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
};

module.exports = {
  getAllAlert,
  getNotDealAlert,
  postNewAlert
};