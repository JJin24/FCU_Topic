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

module.exports = router;

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