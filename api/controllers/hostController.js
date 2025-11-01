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

const handleBuildingList = async (req, res) => {
  console.log("Get host status");
  const host = await hostService.getBuildingList();

  if (host) {
    res.json(host);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve host status data' });
  }
};

const handleGetHostNameByBuilding = async (req, res) => {
  const { building } = req.params;
  console.log("Get host status");
  const host = await hostService.getHostNameByBuilding(building);

  if (host) {
    res.json(host);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve host name' });
  }
};

const handleGetSearchHistory = async (req, res) => {
  const { start_time, end_time, label, building, host } = req.body;

  if (!start_time || !end_time || !building || !Array.isArray(host) || !Array.isArray(label)) {
    return res.status(400).json({ error: 'post data error' });
  }
  console.log("Get search history with filters:", req.body);

  const final_alert_names = label
    .filter(name => name !== 'Good');

  const is_good_requested = label.includes('Good') ? 1 : 0;
  
  const result = await hostService.getSearchHistory(building, host, start_time, end_time, final_alert_names, is_good_requested);

  if (result) {
    res.json(result);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve search history data' });
  }
};

module.exports = {
  handleGetAllHost,
  handleGetHostByIP,
  handleGetHostNameByIP,
  handleGetHostStatus,
  handleBuildingList,
  handleGetHostNameByBuilding,
  handleGetSearchHistory
};
