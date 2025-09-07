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

module.exports = {
  handlePostAlert
};