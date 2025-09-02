const express = require('express');
const router = express.Router();

const protocolListController = require('../controllers/protocolListController');

/**
 * @swagger
 * tags:
 *   name:  protocolList
 *   description: 協定維護 (protocolList) 相關的 API
 */

/**
 * @swagger
 * /protocolList/:
 *   get:
 *     summary: 取得所有協定
 *     description: 回傳一個包含所有網路協定的陣列。
 *     tags: [protocolList]
 *     responses:
 *       200:
 *         description: 成功取得協定列表。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 $ref: '#/components/schemas/Protocol'
 *       404:
 *         description: Failed to retrieve protocol list data.
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
 *                   example: Failed to retrieve protocol list data
 */
router.get('/', protocolListController.handleGetProtocolList);

module.exports = router;

/**
 * @swagger
 * components:
 *   schemas:
 *     Protocol:
 *       type: object
 *       properties:
 *         id:
 *           type: integer
 *           example: 1
 *         name:
 *           type: string
 *           example: "TCP"
 */
