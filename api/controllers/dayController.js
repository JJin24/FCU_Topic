const dayService = require('../services/dayService');

const handleGetXDayAIP = async (req, res) => {
  const { xDay, IP } = req.params;
  console.log(`Get ${xDay} day AIP for IP: ${IP}`);
  const data = await dayService.getXDayAIP(xDay, IP);

  if (data) {
    res.json(data);
  } else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve data' });
  }
};

const handleGetXDayGoodMalCount = async (req, res) => {
  const { xDay } = req.params;
  console.log(`Get ${xDay} day good/malicious count`);
  const data = await dayService.getXDayGoodMalCount(xDay);

  if (data) {
    res.json(data);
  } else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve data' });
  }
};

const handleGetXDayAllCount = async (req, res) => {
  const { xDay } = req.params;
  console.log(`Get ${xDay} day all count`);
  const data = await dayService.getXDayAllCount(xDay);

  if (data) {
    res.json(data);
  } else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve data' });
  }
};

const handleGetXDayProtocolCount = async (req, res) => {
  const { xDay } = req.params;
  console.log(`Get ${xDay} day protocol count`);
  const data = await dayService.getXDayProtocolCount(xDay);

  if (data) {
    res.json(data);
  } else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve data' });
  }
};

const handleGetXDayHostAllCount = async (req, res) => {
  const { xDay } = req.params;
  console.log(`Get ${xDay} day host malicious count`);
  const data = await dayService.getXDayHostAllCount(xDay);

  if (data) {
    res.json(data);
  } else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve data' });
  }
};

const handleGetXDayHostMalCount = async (req, res) => {
  const { xDay } = req.params;
  console.log(`Get ${xDay} day host malicious count`);
  const data = await dayService.getXDayHostMalCount(xDay);

  if (data) {
    res.json(data);
  } else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve data' });
  }
};

const handleGetXDayHostAttackCount = async (req, res) => {
  const { xDay } = req.params;
  console.log(`Get ${xDay} day host attack count`);
  const data = await dayService.getXDayHostAttackCount(xDay);

  if (data) {
    res.json(data);
  } else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve data' });
  }
};

module.exports = {
  handleGetXDayAIP,
  handleGetXDayGoodMalCount,
  handleGetXDayAllCount,
  handleGetXDayProtocolCount,
  handleGetXDayHostAllCount,
  handleGetXDayHostMalCount,
  handleGetXDayHostAttackCount,
};