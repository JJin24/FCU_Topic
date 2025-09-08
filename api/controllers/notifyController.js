const { notify } = require('../routes/hostRoutes');
const notifyService = require('../services/notifyService');

const handlePostAlert = async (req, res) => {
  console.log("Post alert notification");
  const notifyData = req.body;
  const title = `受到 ${notifyData.label} 攻擊`;
    const body = `${notifyData.hostName} (${notifyData.dst_ip}) 在 ${notifyData.timestamp} 受到 ${notifyData.label} 攻擊，分數為 ${notifyData.score} 分，請注意是否有異常流量產生。`;
  console.log("title: " + title);
  console.log("body: " + body);
  const result = await notifyService.postNotification(title, body);

  if (result) {
    res.json({ message: 'Alert notification sent successfully' });
  } else {
    res.status(500).json({ title: '500 Internal Server Error', message: 'Failed to send alert notification' });
  }
};

const handlePostNewDevice = async (req, res) => {
  console.log("Post new device notification");
  const notifyData = req.body;

  const deviceName = notifyData.deviceName || "未知裝置";
  const fcmToken = notifyData.fcmToken || "-";

  const result = await notifyService.addNewDevice(deviceName, fcmToken);

  if (result) {
    res.json({ message: 'New device added successfully' });
  } else {
    res.status(500).json({ title: '500 Internal Server Error', message: 'Failed to add new device' });
  }
};

module.exports = {
  handlePostAlert,
  handlePostNewDevice
};