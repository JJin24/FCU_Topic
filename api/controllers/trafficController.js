const trafficService = require('../services/trafficService');

const handleGetOneHourSum = async (req, res) => {
  const result = await trafficService.getOneHourSum();

  if (result) {
    res.json(result);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve one hour traffic sum data' });
  }
};

module.exports = {
  handleGetOneHourSum
};