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

module.exports = {
  handleGetAllHost,
  handleGetHostByIP,
  handleGetHostNameByIP,
  handleGetHostStatus,
<<<<<<< HEAD
  handlePostNewDevices,
  handleBuildingList
=======
  handleBuildingList,
  handleGetHostNameByBuilding
>>>>>>> f84d8da (Feat: getHostNameByBuilding())
};
