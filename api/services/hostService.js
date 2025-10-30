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
      "INSERT INTO host (name, location, ip, gateway, mac_addr, status, department) VALUES (?, ?, ?, ?, ?, ?, ?)",
      [
        deviceData.name,
        deviceData.location,
        deviceData.ip,
        deviceData.gateway,
        deviceData.mac_addr,
        deviceData.status,
        deviceData.department,
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

module.exports = {
  getAllHost,
  getHostByIP,
  getHostNameByIP,
  getHostStatus,
  postNewDevice,
  getHostStatus,
  getBuildingList
};
