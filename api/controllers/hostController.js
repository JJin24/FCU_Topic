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

const handlePostNewDevices = async (req, res) => {
  // 1. 記錄操作
  console.log("Attempting to add new device data from Flutter App.");
  // console.log("Received data for new device:", deviceData);

  try {
      // 2. 呼叫服務層，處理資料轉換和資料庫寫入
      // hostService.addNewDevice 會處理 IP/Gateway 格式和 MAC 地址到 BIGINT 的轉換
      const newDeviceId = await hostService.postNewDevice(req.body);

      // 3. 成功回應 (201 Created)
      res.status(201).json({
          title: 'Success',
          message: 'Device information saved successfully.',
          device_id: newDeviceId // 回傳新增資料的 ID
      });

  } catch (error) {
      // 4. 錯誤處理
      console.error("Error adding new device:", error.message);

      // 檢查是否為資料驗證錯誤 (400 Bad Request)
      if (error.isValidationError) {
          return res.status(400).json({
              title: 'Validation Error',
              message: error.message || 'The provided data failed validation.'
          });
      }

      // 預設為伺服器內部錯誤 (500 Internal Server Error)
      return res.status(500).json({
          title: 'Server Error',
          message: 'Failed to insert data into MariaDB.'
      });
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

const handleHourlyAlertFlowCount = async (req, res) => {
  // 1. 記錄操作
  console.log("Attempting to get hourly host summary by location.");

  try {
    // 2. 從 req.query (GET 請求) 中獲取 location
    //    我們把 "location 存不存在" 的驗證交給 Service 層處理
    const { location } = req.query;

    // 3. 呼叫服務層，處理所有資料庫邏輯
    const summaryData = await hostService.getHourlyFlowCountByLocationAndHost();

    // 4. 成功回應 (200 OK)
    //    (我們假設 summaryData 是一個陣列)
    res.status(200).json(summaryData); 
  } catch (error) {
    // 5. 錯誤處理
    console.error("Error fetching host summary:", error.message);

    // 檢查是否為資料驗證錯誤 (例如 location 缺失)
    if (error.isValidationError) {
      return res.status(400).json({
          title: 'Validation Error',
          message: error.message || 'The provided data failed validation.'
      });
    }

    // 預設為伺服器內部錯誤 (500 Internal Server Error)
    return res.status(500).json({
        title: 'Server Error',
        message: 'Failed to retrieve data from MariaDB.'
    });
  }
};

const handleAllAlertFlowCount = async (req, res) => {
  // 1. 記錄操作
  console.log("Attempting to get hourly host summary by location.");

  try {
    // 2. 從 req.query (GET 請求) 中獲取 location
    //    我們把 "location 存不存在" 的驗證交給 Service 層處理
    const { location } = req.query;

    // 3. 呼叫服務層，處理所有資料庫邏輯
    const summaryData = await hostService.getAllFlowCountByLocationAndHost();

    // 4. 成功回應 (200 OK)
    //    (我們假設 summaryData 是一個陣列)
    res.status(200).json(summaryData); 
  } catch (error) {
    // 5. 錯誤處理
    console.error("Error fetching host summary:", error.message);

    // 檢查是否為資料驗證錯誤 (例如 location 缺失)
    if (error.isValidationError) {
      return res.status(400).json({
          title: 'Validation Error',
          message: error.message || 'The provided data failed validation.'
      });
    }

    // 預設為伺服器內部錯誤 (500 Internal Server Error)
    return res.status(500).json({
        title: 'Server Error',
        message: 'Failed to retrieve data from MariaDB.'
    });
  }
};

const handleSpecifiedTimeAlertFlowCount = async (req, res) => {
  // 1. 記錄操作
  console.log("Attempting to get hourly host summary by location.");

  try {
    // 2. 從 req.query (GET 請求) 中獲取 location
    //    我們把 "location 存不存在" 的驗證交給 Service 層處理
    const { start_time, end_time } = req.query;

    // 3. 呼叫服務層，處理所有資料庫邏輯
    const summaryData = await hostService.getSpecifiedTimeFlowCount(start_time, end_time);

    // 4. 成功回應 (200 OK)
    //    (我們假設 summaryData 是一個陣列)
    res.status(200).json(summaryData); 
  } catch (error) {
    // 5. 錯誤處理
    console.error("Error fetching host summary:", error.message);

    // 檢查是否為資料驗證錯誤 (例如 location 缺失)
    if (error.isValidationError) {
      return res.status(400).json({
          title: 'Validation Error',
          message: error.message || 'The provided data failed validation.'
      });
    }

    // 預設為伺服器內部錯誤 (500 Internal Server Error)
    return res.status(500).json({
        title: 'Server Error',
        message: 'Failed to retrieve data from MariaDB.'
    });
  }
};

const handleSetMalHost = async (req, res) => {
  const { host } = req.params;
  console.log("Find ip: ", host, "'s host info.");
  const res_1 = await hostService.setMalHost(host);

  if (res_1) {
    res.json("Success set host to malicious");
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve host data' });
  }
};

const handleSetGoodHost = async (req, res) => {
  const { host } = req.params;
  console.log("Find ip: ", host, "'s host info.");
  const res_1 = await hostService.setGoodHost(host);

  if (res_1) {
    res.json("Success set host to good");
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve host data' });
  }
};

module.exports = {
  handleGetAllHost,
  handleGetHostByIP,
  handleGetHostNameByIP,
  handleGetHostStatus,
  handlePostNewDevices,
  handleBuildingList,
  handleGetHostNameByBuilding,
  handleGetSearchHistory,
  handleHourlyAlertFlowCount,
  handleAllAlertFlowCount,
  handleSpecifiedTimeAlertFlowCount,
  handleSetMalHost,
  handleSetGoodHost
};
