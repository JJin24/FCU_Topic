const hostService = require('../services/hostService');

const handleGetAllHost = async (req, res) => {
  console.log("Get all host");
  const host = await hostService.getAllHost();

  if (host) {
    res.json(host);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve host data' });
  }
};

const handleGetHostByIP = async (req, res) => {
  const { ip } = req.params;
  console.log("Find ip: ", ip, "'s host info.");
  const host = await hostService.getHostByIP(ip);

  if (host) {
    res.json(host);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve host data' });
  }
};

const handleGetHostNameByIP = async (req, res) => {
  const { ip } = req.params;
  console.log("Find ip: ", ip, "'s host name.");
  const hostName = await hostService.getHostNameByIP(ip);

  if (hostName) {
    res.json(hostName);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve host name data' });
  }
};

const handleGetHostStatus = async (req, res) => {
  console.log("Get host status");
  const host = await hostService.getHostStatus();

  if (host) {
    res.json(host);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve host status data' });
  }
};

module.exports = {
  handleGetAllHost,
  handleGetHostByIP,
  handleGetHostNameByIP,
  handleGetHostStatus
};