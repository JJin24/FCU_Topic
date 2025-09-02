const pool = require('./database');

async function getAllLabelList() {
  var conn;
  try{
    conn = await pool.getConnection();

    const label_list = await conn.query(
      "SELECT * FROM label_list"
    )
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

module.exports = {
  getAllLabelList
};