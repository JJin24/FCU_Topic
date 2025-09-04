const express = require('express');
const router = express.Router();

const alertController = require('../controllers/alertController');

/**
 * @swagger
 * tags:
 *   name:  alert
 *   description: 警告 (alert) 相關的 API
 */

/**
 * @swagger
 * /alert/:
 *   get:
 *     summary: 取得全部警告流量
 *     description: 回傳全部時間網路流量中有發生警告的流量陣列。同一個 flow 如果 src_ip 與 dst_ip 都有被記錄在 host 中的話，其會顯示兩筆的資訊在此。若兩者皆未被記錄在 host 中，則不會顯示該筆 alert。
 *     tags: [alert]
 *     responses:
 *       200:
 *         description: 成功取得警告流量陣列。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 type: object
 *                 properties:
 *                   uuid:
 *                     type: string
 *                     example: "1ffd7038-80fa-11f0-8f78-0672876e5546"
 *                   hostname:
 *                     type: string
 *                     example: "VM1"
 *                   timestamp:
 *                     type: string
 *                     format: date-time
 *                     example: "2025-08-24T14:47:43.000Z"
 *                   src_ip:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::0370"
 *                   src_port:
 *                     type: integer
 *                     example: 443
 *                   dst_ip:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::7334"
 *                   dst_port:
 *                     type: integer
 *                     example: 51515
 *                   protocol:
 *                     type: string
 *                     example: "TCP"
 *                   label:
 *                     type: string
 *                     example: "DDoS_LOIC"
 *                   score:
 *                     type: number
 *                     format: float
 *                     example: 0.85
 *                   status:
 *                     type: boolean
 *                     example: false
 *       404:
 *         description: Failed to retrieve alert data.
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
 *                   example: Failed to retrieve alert data
 */
router.get('/', alertController.handleGetAllAlert);

/**
 * @swagger
 * /alert/status/notDeal:
 *   get:
 *     summary: 取得尚未處理的警告流量
 *     description: 回傳 status 為 false (尚未處理) 的警告流量陣列。同一個 flow 如果 src_ip 與 dst_ip 都有被記錄在 host 中的話，其會顯示兩筆的資訊在此。若兩者皆未被記錄在 host 中，則不會顯示該筆 alert。
 *     tags: [alert]
 *     responses:
 *       200:
 *         description: 成功取得尚未處理的警告流量陣列。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 type: object
 *                 properties:
 *                   uuid:
 *                     type: string
 *                     example: "1ffd7038-80fa-11f0-8f78-0672876e5546"
 *                   hostname:
 *                     type: string
 *                     example: "VM1"
 *                   timestamp:
 *                     type: string
 *                     format: date-time
 *                     example: "2025-08-24T14:47:43.000Z"
 *                   src_ip:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::0370"
 *                   src_port:
 *                     type: integer
 *                     example: 443
 *                   dst_ip:
 *                     type: string
 *                     format: ipv6
 *                     example: "2001::7334"
 *                   dst_port:
 *                     type: integer
 *                     example: 51515
 *                   protocol:
 *                     type: string
 *                     example: "TCP"
 *                   label:
 *                     type: string
 *                     example: "DDoS_LOIC"
 *                   score:
 *                     type: number
 *                     format: float
 *                     example: 0.85
 *                   status:
 *                     type: boolean
 *                     example: false
 *       404:
 *         description: Failed to retrieve alert data.
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
 *                   example: Failed to retrieve alert data
 */
router.get('/status/notDeal', alertController.handleGetNotDealAlert);


module.exports = router;