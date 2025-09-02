const pool = require('./database');

async function getAllAlert() {
  var conn;
  try{
    conn = await pool.getConnection();

    const alert_history = await conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, status \
      FROM alert_history \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id  \
      LEFT JOIN flow ON alert_history.id = flow.id \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      WHERE alert_history.id = flow.id;"
    )
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
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, status \
      FROM flow \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id  \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      WHERE status = 0;"
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
};

module.exports = {
  getAllAlert,
  getNotDealAlert
};