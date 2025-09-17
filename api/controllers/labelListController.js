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

const handleGetLabelNameByID = async (req, res) => {
  const { id } = req.params;
  console.log("Find label ID \'", + id, "\''s name");
  const labelName = await labelListService.getLabelNameByID(id);

  if (labelName){
    res.json(labelName);
  }
  else{
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve label id data' });
  }
}

module.exports = {
  handleGetAllLabelList,
  handleGetLabelNameByID
};
