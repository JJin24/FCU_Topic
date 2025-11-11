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

async function insertTraffic(interval_seconds, bytes) {
  var conn;
  try {
    conn = await pool.getConnection();
    // Insert into traffic_table. Assumes table has columns: traffic (BIGINT), interval_seconds (INT), timestamp (DATETIME default NOW())
    const res = await conn.query(
      `INSERT INTO traffic_table (traffic, interval_seconds, timestamp) VALUES (?, ?, NOW())`,
      [bytes, interval_seconds]
    );
    return true;
  } catch (err) {
    console.error('Error in insertTraffic', err);
    return false;
  } finally {
    if (conn) conn.release();
  }
}

module.exports = {
  getOneHourSum
  , insertTraffic
};