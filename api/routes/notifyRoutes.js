const express = require('express');
const router = express.Router();

/**
 * @swagger
 * tags:
 *   name: notify
 *   description: 通知 (notify) 相關的 API
 */

const notifyController = require('../controllers/notifyController');

/**
 * @swagger
 * /notify/alert:
 *   post:
 *     summary: 發送警告通知
 *     description: 接收攻擊類型及相關資訊後傳送通知到以記錄在資料庫中的裝置。
 *     tags: [notify]
 *     requestBody:
 *       discription: 攻擊種類和 Host 相關資訊
 *       required: true
 *       content:
 *         application/json:
 *           schema:
 *             type: object
 *             properties:
 *               label:
 *                 type: string
 *                 example: DDoS_LOIC_UDP
 *               hostName:
 *                 type: string
 *                 format: hostname
 *                 example: 董事長辦公室
 *               dst_ip:
 *                 type: string
 *                 format: ipv6 
 *               timestamp:
 *                 type: string
 *                 format: date-time
 *               score:
 *                 type: number
 *                 format: float 
 *                 example: 0.95               
 *     responses:
 *       200:
 *         description: 成功發送警告通知。
 *      
 *       404:
 *         description: Failed to send notification.
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
 *                   example: Failed to retrieve notification data
 */
router.post('/alert', notifyController.handlePostAlert);

module.exports = router;