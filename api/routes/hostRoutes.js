const express = require('express');
const router = express.Router();

const hostController = require('../controllers/hostController');

/**
 * @swagger
 * tags:
 *   name:  host
 *   description: 主機資訊 (host) 相關的 API
 */


/**
 * @swagger
 * /host/:
 *   get:
 *     summary: 取得所有主機資訊
 *     description: 回傳一個包含所有已知主機資訊的陣列。
 *     tags: [host]
 *     responses:
 *       200:
 *         description: 成功取得主機資訊陣列。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 $ref: '#/components/schemas/Host'
 *       404:
 *         description: Failed to retrieve host data.
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: 404 Not Found
 *                 message:
 *                   type: string
 *                   example: Failed to retrieve host data
 */
router.get('/', hostController.handleGetAllHost);

/**
 * @swagger
 * /host/ip/{ip}:
 *   get:
 *     summary: 透過 IP 取得特定主機資訊
 *     description: 根據提供的 IP 位址，回傳單一主機的詳細資訊。
 *     tags: [host]
 *     parameters:
 *       - in: path
 *         name: ip
 *         schema:
 *           type: string
 *           format: ipv4
 *         required: true
 *         description: 要查詢的 IP 位址
 *         example: "::ffff:192.168.1.100"
 *     responses:
 *       200:
 *         description: 成功取得指定主機的資訊。
 *         content:
 *           application/json:
 *             schema:
 *               $ref: '#/components/schemas/Host'
 *       404:
 *         description: Failed to retrieve host data.
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: 404 Not Found
 *                 message:
 *                   type: string
 *                   example: Failed to retrieve host data
 */
router.get('/ip/:ip', hostController.handleGetHostByIP);

/**
 * @swagger
 * /host/name/{ip}:
 *   get:
 *     summary: 透過 IP 取得主機名稱
 *     description: 根據提供的 IP 位址，回傳該主機的名稱。
 *     tags: [host]
 *     parameters:
 *       - in: path
 *         name: ip
 *         schema:
 *           type: string
 *           format: ipv4
 *         required: true
 *         description: 要查詢的 IP 位址
 *         example: "::ffff:192.168.1.100"
 *     responses:
 *       200:
 *         description: 成功取得主機名稱。
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 name:
 *                   type: string
 *                   example: "Main-Server"
 *       404:
 *         description: Failed to retrieve host name data.
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: 404 Not Found
 *                 message:
 *                   type: string
 *                   example: Failed to retrieve host name data
 */
router.get('/name/:ip', hostController.handleGetHostNameByIP);


/**
 * @swagger
 * /host/status:
 *   get:
 *     summary: 取得所有區域裝置數量
 *     description: 每個區域的正常/異常裝置數量。
 *     tags: [host]
 *     responses:
 *       200:
 *         description: 成功取得各區域裝置狀態統計。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 $ref: '#/components/schemas/HostStatus'
 *       404:
 *         description: Failed to retrieve host status data.
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: 404 Not Found
 *                 message:
 *                   type: string
 *                   example: Failed to retrieve host status data
 */
router.get('/status', hostController.handleGetHostStatus);

/**
 * @swagger
 *  /host/AddNewDevices:
 *    post:
 *      summary: 新增單筆主機裝置資訊
 *      description: 接收手機 App 傳入的裝置詳細資訊（名稱、位置、IP、MAC等），並寫入 MariaDB 的 host 資料表。
 *      tags: [host]
 * 
 *      requestBody:
 *        required: true
 *        content:
 *          application/json:
 *            schema:
 *              # 引用或定義一個 Host Schema，定義了 POST 請求必須包含的欄位
 *              $ref: '#/components/schemas/HostInput'
 *      responses:
 * 
 *        201:
 *          description: 成功新增主機資訊
 *          content:
 *            application/json:
 *              schema:
 *                type: object
 *                properties:
 *                  title:
 *                    type: string
 *                    example: Success
 *                  message:
 *                    type: string
 *                    example: Device information saved successfully.
 *                  device_id: # 假設資料庫新增後會回傳新 ID
 *                    type: integer
 *                    example: 1024
 * 
 *        400:
 *          description: 資料驗證失敗 (例如欄位遺失或格式錯誤)
 *          content:
 *            application/json:
 *              schema:
 *                type: object
 *                properties:
 *                  title:
 *                    type: string
 *                    example: Validation Error
 *                  message:
 *                    type: string
 *                    example: The provided MAC address format is invalid.
 * 
 *        500:
 *          description: 伺服器內部錯誤 (例如資料庫連線失敗)
 *          content:
 *            application/json:
 *              schema:
 *                type: object
 *                properties:
 *                  title:
 *                    type: string
 *                    example: Server Error
 *                  message:
 *                    type: string
 *                    example: Failed to insert data into MariaDB.
 */

router.post('/AddNewDevices', hostController.handlePostNewDevices);

/** 
 * components:
 *   schemas:
 *     HostInput:
 *       type: object
 *       required:
 *         - name
 *         - location
 *         - ip_address
 *         - gateway
 *         - mac_address
 *         - department_id
 *         - status
 *       properties:
 *         name:
 *           type: string
 *           description: 裝置名稱 (CHAR(32))
 *           maxLength: 32
 *           example: "Server-Rack-05"
 *         location:
 *           type: string
 *           description: 裝置所在位置 (CHAR(32))
 *           maxLength: 32
 *           example: "Data Center A"
 *         ip:
 *           type: string
 *           format: "ipv4 or ipv6" # 提醒開發者格式
 *           description: IP 地址 (INET6)
 *           example: "192.168.1.50"
 *         gateway:
 *           type: string
 *           format: "ipv4 or ipv6"
 *           description: 閘道器地址 (INET6)
 *           example: "192.168.1.1"
 *         mac_addr:
 *           type: string
 *           description: MAC 地址 (需後端轉換為 BIGINT)
 *           example: "A1:B2:C3:D4:E5:F6"
 *         department:
 *           type: integer
 *           format: int64 # 對應 MariaDB 的 INT(10) UNSIGNED
 *           description: 所屬部門 ID (INT(10) UNSIGNED)
 *           example: 105
 *         status:
 *           type: integer
 *           format: int32
 *           description: 狀態 (TINYINT(1), 1=啟用, 0=停用)
 *           example: 1
 *        importance:
 *          type: integer
 *          format: int32
 *         description: 重要性等級 (TINYINT(1), 1~5)
 */

/**
 * @swagger
 * /host/building:
 *   get:
 *     summary: 取得建築物列表
 *     description: 取得 host 中已記錄 location 位置。
 *     tags: [host]
 *     responses:
 *       200:
 *         description: 成功取得取得建築物列表。
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 location:
 *                   type: string
 *                   example: "資電大樓"
 *       404:
 *         description: Failed to retrieve building list data.
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: 404 Not Found
 *                 message:
 *                   type: string
 *                   example: Failed to retrieve building list data
 */
router.get('/building', hostController.handleBuildingList);

/**
 * @swagger
 * /host/building/{:building}:
 *   get:
 *     summary: 取得建築物列表 (API-DOCS 未完成)
 *     description: 取得 host 中已記錄 location 位置。
 *     tags: [host]
 *     parameters:
 *       - in: path
 *         name: building
 *         schema:
 *           type: string
 *         required: true
 *         description: 要查詢的建築物名稱
 *         example: "資電大樓"
 *     responses:
 *       200:
 *         description: 成功取得取得建築物列表。
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 HostName:
 *                   type: string
 *                   example: "Server1"
 *       404:
 *         description: Failed to retrieve building list data.
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: 404 Not Found
 *                 message:
 *                   type: string
 *                   example: Failed to retrieve building list data
 */
router.get('/building/:building', hostController.handleGetHostNameByBuilding);

/**
 * @swagger
 * /host/history:
 *  post:
 *    summary: 根據時間、位置和標籤篩選流量紀錄
 *    description: 允許使用者提交多重條件，查詢符合時間範圍、裝置和流量標籤（好/壞）的流量資料。
 *    tags: [host]
 *    requestBody:
 *      required: true
 *      content:
 *        application/json:
 *          schema:
 *            type: object
 *            required:
 *              - start_time
 *              - end_time
 *              - label
 *              - building
 *              - host
 *            properties:
 *              start_time:
 *                type: string
 *                format: date-time
 *                description: 查詢的起始時間 (e.g., "2025-10-01T00:00:00.000Z")
 *                example: 2025-10-01T00:00:00.000Z
 *              end_time:
 *                type: string
 *                format: date-time
 *                description: 查詢的結束時間
 *                example: 2025-10-31T23:59:59.999Z
 *              label:
 *                type: array
 *                description: 流量標籤 ID 列表。可包含 'Good' 或 label_list 中的數字 ID。
 *                items:
 *                  type: string
 *                  example: "5"
 *                example:
 *                  - "2"
 *                  - "Good"
 *              building:
 *                type: string
 *                description: 裝置所在位置 (對應 host.location)。
 *                example: Server_Room_A
 *              host:
 *                type: array
 *                description: 裝置名稱列表 (對應 host.name)。
 *                items:
 *                  type: string
 *                  example: Host-DB
 *                example:
 *                  - Host-01
 *                  - Host-DB
 *    responses:
 *      '200':
 *        description: 成功的查詢結果
 *        content:
 *          application/json:
 *            schema:
 *              type: array
 *              items:
 *                $ref: '#/components/schemas/FlowQueryResult'
 *      '400':
 *        description: 請求參數格式錯誤
 */
router.post('/history', hostController.handleGetSearchHistory);


/**
 * @swagger
 * components:
 *   schemas:
 *     Host:
 *       type: object
 *       properties:
 *         ip:
 *           type: string
 *           format: ipv6
 *           example: "::ffff:192.168.1.100"
 *         name:
 *           type: string
 *           example: "Main-Server"
 *         location:
 *           type: string
 *           example: "Data Center 1"
 *         gateway:
 *           type: string
 *           format: ipv6
 *           example: "::ffff:192.168.1.254"
 *         mac_address:
 *           type: number
 *           format: mac
 *           example: "1657546"
 *         status:
 *           type: boolean
 *           example: true
 *     HostStatus:
 *       type: object
 *       properties:
 *         location:
 *           type: string
 *           example: "人言大樓"
 *         nornal:
 *           type: string
 *           example: "0"
 *         warn:
 *           type: string
 *           example: "0"
 *         alert:
 *           type: string
 *           example: "0"
 */

/**
 * 
 * @swagger
 * /host/alertflowcount:
 *    get:
 *      summary: 獲取「指定地點」近一小時內受攻擊的裝置摘要
 *      description: 查詢特定地點(Location)在過去一小時內所有被攻擊的裝置(Host)，並依據攻擊類型(Label)分類計數。
 *      tags: [host]
 *      parameters:
 *        - in: query
 *          name: location
 *          required: true
 *          schema:
 *            type: string
 *            description: "要篩選的裝置地點 (例如: 人言大樓)"
 *          example: "資電大樓"
 *      responses:
 *        200:
 *          description: 成功返回摘要列表 (如果沒有攻擊，則返回空陣列 [])
 *          content:
 *            application/json:
 *              schema:
 *                type: array
 *                items:
 *                  $ref: '#/components/schemas/HostSummaryItem'
 *        400:
 *          description: 缺少'location'查詢參數
 *          content:
 *            application/json:
 *              schema:
 *                $ref: '#/components/schemas/ErrorResponse'
 *        500:
 *          description: 伺服器內部錯誤
 *          content:
 *            application/json:
 *              schema:
 *                $ref: '#/components/schemas/ErrorResponse'
 */

router.get('/alertflowcount', hostController.handleAlertFlowCount);

module.exports = router;

/**
 * @swagger
 * components:
 *   schemas:
 *     HostSummaryItem:
 *       type: object
 *       properties:
 *         name:
 *           type: string
 *           description: 裝置名稱
 *           example: "Main-Server"
 *         location:
 *           type: string
 *           description: 裝置位置
 *           example: "資電大樓"
 *         ip:
 *           type: string
 *           format: ipv6
 *           description: 裝置 IP
 *           example: "::ffff:192.168.1.100"
 *         importance:
 *           type: integer
 *           description: 裝置重要性等級
 *           example: 3
 *         attack_label:
 *           type: integer
 *           description: 攻擊類型標籤 ID
 *           example: 5
 *         attack_count:
 *           type: integer
 *           description: 該裝置遭受此類攻擊的次數
 *           example: 10
 */