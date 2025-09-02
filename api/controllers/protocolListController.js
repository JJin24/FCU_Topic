const protocolListService = require('../services/protocolListService');

const handleGetProtocolList = async (req, res) => {
  console.log("Get protocol list");
  const protocolList = await protocolListService.getProtocolList();

  if (protocolList) {
    res.json(protocolList);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve protocol list data' });
  }
};

module.exports = {
  handleGetProtocolList
};
