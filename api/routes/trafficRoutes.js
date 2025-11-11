const express = require('express');
const router = express.Router();

const trafficController = require('../controllers/trafficController');

/**
 * @swagger
 * tags:
 *   name:  traffic
 *   description: 流量 (traffic) 相關的 API
 */

/**
 * @swagger
 * /traffic/oneHourSum:
 *   get:
 *     summary: 取得最近一小時的總流量
 *     description: 回傳最近一小時經過抓包程式的流量總和。
 *     tags: [traffic]
 *     responses:
 *       200:
 *         description: 成功取得總流量。
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 total_traffic:
 *                   type: integer
 *                   example: 12345
 *       404:
 *         description: Failed to retrieve one hour traffic sum data.
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
 *                   example: Failed to retrieve one hour traffic sum data
 */
router.get('/oneHourSum', trafficController.handleGetOneHourSum);
// 新增：接收流量報告並存入資料庫
router.post('/report', trafficController.handleReportTraffic);

module.exports = router;