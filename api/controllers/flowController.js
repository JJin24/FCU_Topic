const flowService = require('../services/flowService');

const handleGetAllFlow = async (req, res) => {
  console.log("Get all flow");
  const flow = await flowService.getAllFlow();

  if (flow) {
    res.json(flow);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve flow data' });
  }
};

const handleGetFlowByUUID = async (req, res) => {
  const { uuid } = req.params;
  console.log("Find uuid: ", uuid);
  const flow = await flowService.getFlowByUUID(uuid);

  if (flow) {
    res.json(flow);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve flow data' });
  }
};

const handleGetFlowByIP = async (req, res) => {
  const { ip } = req.params;
  console.log("Find ip: ", ip);
  const flow = await flowService.getFlowByIP(ip);

  if (flow) {
    res.json(flow);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve flow data' });
  }
};

const handleGetAllFlowIPCount = async (req, res) => {
  const ipCount = await flowService.getAllFlowIPCount();

  if (ipCount) {
    res.json(ipCount);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve IP count data' });
  }
};

const handleGetAllFlowProtocolCount = async (req, res) => {
  const protocolCount = await flowService.getFlowProtocolCount();

  if (protocolCount) {
    res.json(protocolCount);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve protocol count data' });
  }
};

const handleGetTopXDayFlow = async (req, res) => {
  const { xDay }  = req.params;
  
  console.log("Find ", xDay, " days flow");
  const topXDayFlow = await flowService.getTopXDayFlow(xDay);

  if (topXDayFlow) {
    res.json(topXDayFlow);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve top X days flow data' });
  }
};

const handleGet24HourFlowCount = async (req, res) => {
  const flowCount = await flowService.get24HourFlowCount();

  if (flowCount) {
    res.json(flowCount);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve 24 hour flow count data' });
  }
};

const handleGetPerHourAllFlowCount = async (req, res) => {
  const flowCount = await flowService.getPerHourAllFlowCount();

  if (flowCount) {
    res.json(flowCount);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve per hour all flow count data' });
  }
};

const handleGetGoodMalCount = async (req, res) => {
  const result = await flowService.getGoodMalCount();

  if (result) {
    res.json(result);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve good/malicious flow count data' });
  }
};

const handleGetLocationGraph = async (req, res) => {
  const { locationName } = req.params;
  const result = await flowService.getLocationGraph(locationName);

  if (result) {
    res.json(result);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve location graph data' });
  }
};

module.exports = {
  handleGetAllFlow,
  handleGetFlowByUUID,
  handleGetFlowByIP,
  handleGetAllFlowIPCount,
  handleGetAllFlowProtocolCount,
  handleGetTopXDayFlow,
  handleGet24HourFlowCount,
  handleGetPerHourAllFlowCount,
  handleGetGoodMalCount,
  handleGetLocationGraph
};
