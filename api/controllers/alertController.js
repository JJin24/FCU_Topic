const alertService = require('../services/alertService');

const handleGetAllAlert = async (req, res) => {
  console.log("Get all alert");
  const alert = await alertService.getAllAlert();

  if (alert) {
    res.json(alert);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve alert data' });
  }
};

const handleGetNotDealAlert = async (req, res) => {
  console.log("Get not deal alert");
  const alert = await alertService.getNotDealAlert();
  if (alert) {
    res.json(alert);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve alert data' });
  }
};

const handlePostNewAlert = async (req, res) => {
  console.log("Post new alert");
  const alertData = req.body;

  const result = await alertService.postNewAlert(alertData);

  if (result) {
    res.json({ message: 'New alert posted successfully' });
  } else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to record this alert' });
  }
};

module.exports = {
  handleGetAllAlert,
  handleGetNotDealAlert,
  handlePostNewAlert
};