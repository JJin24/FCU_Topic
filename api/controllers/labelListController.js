const labelListService = require('../services/labelLsitService');

const handleGetAllLabelList = async (req, res) => {
  const labelList = await labelListService.getAllLabelList();

  if (labelList) {
    res.json(labelList);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve label list data' });
  }
};

module.exports = {
  handleGetAllLabelList
};
