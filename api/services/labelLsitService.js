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

async function getLabelNameByID(id){
  var conn;

  try{
    conn = await pool.getConnection();

    const labelName = await conn.query(
      "SELECT name FROM label_list WHERE label_id = ?", [id]
    )
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

module.exports = {
  getAllLabelList,
  getLabelNameByID
};