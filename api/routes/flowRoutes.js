const express = require('express');
const router = express.Router();

const flowController = require('../controllers/flowController');

/**
 * @swagger
 * tags:
 *   name:  flow
 *   description: 流量 (flow) 相關的 API
 */


/**
 * @swagger
 * /flow/:
 *   get:
 *     summary: 取得全部流量
 *     description: 回傳全部時間的網路流量陣列。
 *     tags: [flow]
 *     responses:
 *       200:
 *         description: 成功取得流量陣列。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 $ref: '#/components/schemas/Flow'
 *       404:
 *         description: Failed to retrieve flow data.
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
 *                   example: Failed to retrieve flow data
 */
router.get('/', flowController.handleGetAllFlow);

/**
 * @swagger
 * /flow/uuid/{uuid}:
 *   get:
 *     summary: 透過 UUID 取得特定流量
 *     description: 根據提供的 UUID 回傳單一筆網路流量資料。
 *     tags: [flow]
 *     parameters:
 *       - in: path
 *         name: uuid
 *         schema:
 *           type: string
 *         required: true
 *         description: 要查詢的流量 UUID
 *         example: "1ffd7038-80fa-11f0-8f78-0672876e5546"
 *     responses:
 *       200:
 *         description: 成功取得指定的流量資料。
 *         content:
 *           application/json:
 *             schema:
 *               $ref: '#/components/schemas/Flow'
 *       404:
 *         description: Failed to retrieve flow data.
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
 *                   example: Failed to retrieve flow data
 */
router.get('/uuid/:uuid', flowController.handleGetFlowByUUID);

/**
 * @swagger
 * /flow/ip/{ip}:
 *   get:
 *     summary: 透過 IP 取得相關流量
 *     description: 根據提供的 IP 位址，回傳所有相關的網路流量資料陣列 (包含來源或目的 IP)。
 *     tags: [flow]
 *     parameters:
 *       - in: path
 *         name: ip
 *         schema:
 *           type: string
 *           format: ipv6
 *         required: true
 *         description: 要查詢的 IP 位址
 *         example: "2001::0370"
 *     responses:
 *       200:
 *         description: 成功取得指定 IP 的相關流量陣列。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 $ref: '#/components/schemas/Flow'
 *       404:
 *         description: Failed to retrieve flow data.
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
 *                   example: Failed to retrieve flow data
 */
router.get('/ip/:ip', flowController.handleGetFlowByIP);

/**
 * @swagger
 * /flow/ipCount:
 *   get:
 *     summary: 取得每個 IP 地址的出現次數
 *     description: 回傳一個陣列，其中包含每個 IP 地址及其在流量中出現的次數。
 *     tags: [flow]
 *     responses:
 *       200:
 *         description: 成功取得 IP 出現次數。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 type: object
 *                 properties:
 *                   ip:
 *                     type: string
 *                     description: IP 地址
 *                     example: "::ffff:192.168.159.128"
 *                   count:
 *                     type: number
 *                     description: 該 IP 地址出現的次數
 *                     example: 118
 *                 required:
 *                   - ip
 *                   - count
 *               example:
 *                 - ip: "::ffff:10.231.2.2"
 *                   count: 1
 *                 - ip: "::ffff:98.11.235.11"
 *                   count: 1
 *                 - ip: "::ffff:192.168.159.128"
 *                   count: 118
 *                 - ip: "::ffff:192.168.159.129"
 *                   count: 118
 *       404:
 *         description: Failed to retrieve IP count data.
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
 *                   example: Failed to retrieve IP count data
 */
router.get('/ipCount', flowController.handleGetAllFlowIPCount);

/**
 * @swagger
 * /flow/protocolCount:
 *   get:
 *     summary: 取得各種協定的流量計數
 *     description: 回傳一個物件，其中包含每種網路協定 (如 TCP, UDP) 的流量數量。
 *     tags: [flow]
 *     responses:
 *       200:
 *         description: 成功取得協定計數。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 type: object
 *                 properties:
 *                   protocol:
 *                     type: string
 *                     description: 協定名稱
 *                     example: "TCP"
 *                   count:
 *                     type: number
 *                     description: 該協定出現的次數
 *                     example: 118
 *                 required:
 *                   - porotocol
 *                   - count
 *               example:
 *                 - protocol: "TCP"
 *                   count: 320
 *                 - protocol: "UDP"
 *                   count: 20
 *       404:
 *         description: Failed to retrieve protocol count data.
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
 *                   example: Failed to retrieve protocol count data
 */
router.get('/protocolCount', flowController.handleGetAllFlowProtocolCount);

/**
 * @swagger
 * /flow/day/{xDay}:
 *   get:
 *     summary: 取得最近 X 天的流量數據
 *     description: 根據提供的天數，回傳最近 X 天內的流量數據。
 *     tags: [flow]
 *     parameters:
 *       - in: path
 *         name: xDay
 *         schema:
 *           type: integer
 *         required: true
 *         description: 要查詢的最近天數
 *         example: 7
 *     responses:
 *       200:
 *         description: 成功取得最近 X 天的流量數據。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 $ref: '#/components/schemas/Flow'
 *       404:
 *         description: Failed to retrieve top X days flow data.
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
 *                   example: Failed to retrieve top X days flow data
 */
router.get('/day/:xDay', flowController.handleGetTopXDayFlow);

/**
 * @swagger
 * /flow/24Hour/everyFlowCount:
 *   get:
 *     summary: 取得過去 24 小時內每個 Flow 的流量計數
 *     description: 回傳過去 24 小時內每個 Flow 的流量計數，包含兩端 IP 及其對應的流量數量。
 *     tags: [flow]
 *     responses:
 *       200:
 *         description: 成功取得過去 24 小時內每個 Flow 的流量計數。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 type: object
 *                 properties:
 *                   pointA:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::0370"
 *                   pointB:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::7334"
 *                   count:
 *                     type: integer
 *                     example: 15
 *       404:
 *         description: Failed to retrieve top X days flow data.
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
 *                   example: Failed to retrieve top X days flow data
 */
router.get('/24Hour/everyFlowCount', flowController.handleGet24HourFlowCount);

/**
 * @swagger
 * /flow/24Hour/perHourAllFlowCount:
 *   get:
 *     summary: 取得過去 24 小時內每個小時的總流量計數
 *     description: 回傳過去 24 小時內每個小時的總流量計數，已經自動填滿沒有流量的時間點為 0，並依照目前的時間點前 24 小時的小時排序。
 *     tags: [flow]
 *     responses:
 *       200:
 *         description: 成功取得過去 24 小時內每個小時的流量計數。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 type: object
 *                 properties:
 *                   pointA:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::0370"
 *                   pointB:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::7334"
 *                   count:
 *                     type: integer
 *                     example: 15
 *       404:
 *         description: Failed to retrieve top X days flow data.
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
 *                   example: Failed to retrieve top X days flow data
 */
router.get('/24Hour/perHourAllFlowCount', flowController.handleGetPerHourAllFlowCount);

module.exports = router;

/**
 * @swagger
 * components:
 *   schemas:
 *     Flow:
 *       type: object
 *       properties:
 *         uuid:
 *           type: string
 *           example: "1ffd7038-80fa-11f0-8f78-0672876e5546"
 *         timestamp:
 *           type: string
 *           format: date-time
 *           example: "2025-08-24T14:47:43.000Z"
 *         src_ip:
 *           type: string
 *           format: ipv6
 *           example: "2001::0370"
 *         src_port:
 *           type: integer
 *           example: 443
 *         dst_ip:
 *           type: string
 *           format: ipv6
 *           example: "2001::7334"
 *         dst_port:
 *           type: integer
 *           example: 51515
 *         protocol:
 *           type: string
 *           example: "TCP"
 *         label:
 *           type: string
 *           example: "DDoS_LOIC"
 *         score:
 *           type: number
 *           format: float
 *           example: 0.85
 *         status:
 *           type: boolean
 *           example: false
 */
