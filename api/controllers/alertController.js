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

module.exports = {
  handleGetAllAlert,
  handleGetNotDealAlert
};