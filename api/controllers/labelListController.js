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
};

// ===== START: 新增這個函數 =====
const handleGetHourlyAttackTypesByLocation = async (req, res) => {
  // 1. 記錄操作
  console.log("Attempting to get unique attack types by location for the last hour.");

  try {
    // 2. 從 req.query (GET 請求) 中獲取 location
    const { location } = req.query;

    // 3. 呼叫服務層
    const attackTypes = await labelListService.getAttackTypesByLocation(location);

    // 4. 成功回應 (200 OK)
    // (如果沒有攻擊，attackTypes 會是一個空陣列 [])
    res.status(200).json(attackTypes); 

  } catch (error) {
    // 5. 錯誤處理
    console.error("Error fetching attack types by location:", error.message);

    if (error.isValidationError) {
      return res.status(400).json({
          title: 'Validation Error',
          message: error.message || 'The provided data failed validation.'
      });
    }

    return res.status(500).json({
        title: 'Server Error',
        message: 'Failed to retrieve data from MariaDB.'
    });
  }
};

const handleGetAllAttackTypesByLocation = async (req, res) => {
  // 1. 記錄操作
  console.log("Attempting to get unique attack types by location for the last hour.");

  try {
    // 2. 從 req.query (GET 請求) 中獲取 location
    const { location } = req.query;

    // 3. 呼叫服務層
    const attackTypes = await labelListService.getAttackTypesByLocation(location);

    // 4. 成功回應 (200 OK)
    // (如果沒有攻擊，attackTypes 會是一個空陣列 [])
    res.status(200).json(attackTypes); 

  } catch (error) {
    // 5. 錯誤處理
    console.error("Error fetching attack types by location:", error.message);

    if (error.isValidationError) {
      return res.status(400).json({
          title: 'Validation Error',
          message: error.message || 'The provided data failed validation.'
      });
    }

    return res.status(500).json({
        title: 'Server Error',
        message: 'Failed to retrieve data from MariaDB.'
    });
  }
};

module.exports = {
  handleGetAllLabelList,
  handleGetLabelNameByID,
  handleGetHourlyAttackTypesByLocation,
  handleGetAllAttackTypesByLocation,
};
