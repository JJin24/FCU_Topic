const express = require('express');
const router = express.Router();
const dayController = require('../controllers/dayController');

/**
 * @swagger
 * tags:
 *   name:  day
 *   description: 取得前 X 天的資料
 */


/**
 * @swagger
 * /day/{xDay}/ip/{IP}:
 *   get:
 *     summary: 取得前 xDay 天指定 IP 的所有流量
 *     description: 回傳前 xDay 天指定 IP 的所有流量資料。
 *     tags: [day]
 *     parameters:
 *       - in: path
 *         name: xDay
 *         schema:
 *           type: integer
 *         required: true
 *         description: 要查詢的最近天數
 *         example: 7
 *       - in: path
 *         name: IP  
 *         schema:
 *           type: string
 *           format: ipv6
 *         required: true
 *         description: 要查詢的 IP 位址
 *         example: "2001::0370"
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
router.get('/:xDay/ip/:IP', dayController.handleGetXDayAIP);

/**
 * @swagger
 * /day/{xDay}/goodMalCount:
 *   get:
 *     summary: 取得前 xDay 天指定 IP 的所有流量好壞總數
 *     description: 回傳前 xDay 天指定 IP 的所有流量的好壞總數。
 *     tags: [day]
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
 *                 type: object
 *                 properties:
 *                   goodFlowCount:
 *                     type: integer
 *                     example: 1200
 *                   badFlowCount:
 *                     type: integer
 *                     example: 300
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
router.get('/:xDay/goodMalCount', dayController.handleGetXDayGoodMalCount);
router.get('/:xDay/allCount', dayController.handleGetXDayAllCount);
router.get('/:xDay/protocolCount', dayController.handleGetXDayProtocolCount);

/**
 * @swagger
 * /day/{xDay}/hostAllCount:
 *   get:
 *     summary: 取得前 xDay 天各 Host 中總 Flow 數量
 *     description: 回傳前 xDay 天各 Host 中總 Flow 數量。
 *     tags: [day]
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
 *                 type: object
 *                 properties:
 *                   hostName:
 *                     type: string
 *                     example: "Host1"
 *                   ip:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::0370"
 *                   gateway:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::1"
 *                   totalFlowCount:
 *                     type: integer
 *                     example: 1500
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
router.get('/:xDay/hostAllCount', dayController.handleGetXDayHostAllCount);

/**
 * @swagger
 * /day/{xDay}/hostMalCount:
 *   get:
 *     summary: 取得前 xDay 天各 Host 中壞流量數量
 *     description: 回傳前 xDay 天各 Host 中壞流量數量。
 *     tags: [day]
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
 *                 type: object
 *                 properties:
 *                   hostName:
 *                     type: string
 *                     example: "Host1"
 *                   ip:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::0370"
 *                   gateway:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::1"
 *                   badFlowCount:
 *                     type: integer
 *                     example: 1500
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
router.get('/:xDay/hostMalCount', dayController.handleGetXDayHostMalCount);
router.get('/:xDay/hostAttackCount', dayController.handleGetXDayHostAttackCount);

module.exports = router;
