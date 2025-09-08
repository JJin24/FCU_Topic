const pool = require('./database');
const fcm = require('./notification');

async function postNotification(title, body) {
  let conn;
  try{
    console.log("Post notification in alertService");
    conn = await pool.getConnection();

    const tokenObjects = await conn.query(
      "SELECT notification_token.FCMToken AS Token FROM notification_token;"
    );
    const tokens = tokenObjects.map(item => item.Token);
    console.log("tokens: " + JSON.stringify(tokens));
    return await fcm.sendNotificationToMulti(tokens, title, body);
  }
  catch(err){
    console.error('Error in postNotification', err);
    return false;
  }
  finally {
    if (conn) conn.release(); // Release the connection back to the pool
  }  
};

async function addNewDevice(deviceName, fcmToken) {
  var conn;
  try{
    console.log("Add new device in notifyService");
    conn = await pool.getConnection();

    const result = await conn.query(
      "INSERT INTO notification_token (notification_token.name, FCMToken) VALUES (?, ?);",
      [deviceName, fcmToken]
    );
    console.log("Insert result: " + JSON.stringify(result));
    return true;
  }
  catch(err){
    console.error('Error in addNewDevice', err);
    return false;
  }
  finally {
    if (conn) conn.release(); // Release the connection back to the pool
  }
};

module.exports = {
  postNotification,
  addNewDevice
};