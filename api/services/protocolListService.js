const pool = require('./database');

async function getProtocolList(){
  var conn;
  try{
    conn = await pool.getConnection();

    const protocol_list = await conn.query(
      "SELECT * FROM protocol_list"
    )
    console.log(protocol_list);
    return protocol_list;
  }
  catch(err){
    console.error('Error in getProtocolList', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

module.exports = {
  getProtocolList
};
