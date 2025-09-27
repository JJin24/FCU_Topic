const pool = require('./database');

async function getOneHourSum() {
  var conn;
  try {
    conn = await pool.getConnection();

    const [rows] = await conn.query(
      `SELECT
        COALESCE(SUM(traffic), 0) AS total_traffic
      FROM
        traffic_table
      WHERE
        timestamp >= NOW() - INTERVAL 1 HOUR;`
    );
    console.log(rows);
    return rows;
  }
  catch (err) {
    console.error('Error in getOneHourSum', err);
  }
  finally {
    if (conn) conn.release();
  }
}

module.exports = {
  getOneHourSum
};