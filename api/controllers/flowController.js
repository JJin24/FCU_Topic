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

module.exports = {
  handleGetAllFlow,
  handleGetFlowByUUID,
  handleGetFlowByIP,
  handleGetAllFlowProtocolCount,
  handleGetTopXDayFlow
};
