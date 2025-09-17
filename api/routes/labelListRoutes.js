const express = require('express');
const router = express.Router();

const labelListController = require('../controllers/labelListController');

/**
 * @swagger
 * tags:
 *   name:  labelList
 *   description: 模型可辨識標籤 (labelList) 相關的 API
 */


/**
 * @swagger
 * /labelList/:
 *   get:
 *     summary: 取得所有可辨識的攻擊標籤
 *     description: 回傳一個包含所有模型可辨識的攻擊類型標籤的陣列。
 *     tags: [labelList]
 *     responses:
 *       200:
 *         description: 成功取得標籤列表。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 $ref: '#/components/schemas/Label'
 *       404:
 *         description: Failed to retrieve label list data.
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
 *                   example: Failed to retrieve label list data
 */
router.get('/', labelListController.handleGetAllLabelList);

/**
 * @swagger
 * /labelList/id/{id}:
 *   get:
 *     summary: 使用 label ID 取得 label 名稱
 *     description: 回傳該 label ID 所對應的名稱。
 *     tags: [labelList]
 *     parameters:
 *       - in: path
 *         name: id
 *         schema:
 *           type: number
 *           format: int
 *         required: true
 *         description: 要查詢的 label ID
 *         example: "1"
 *     responses:
 *       200:
 *         description: 成功取得標籤名稱。
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 name:
 *                   type: string
 *                   example: DDoS_LOIC
 *       404:
 *         description: Failed to retrieve label list data.
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
 *                   example: Failed to retrieve label list data
 */
router.get('/id/:id', labelListController.handleGetLabelNameByID);

module.exports = router;

/**
 * @swagger
 * components:
 *   schemas:
 *     Label:
 *       type: object
 *       properties:
 *         id:
 *           type: integer
 *           example: 1
 *         name:
 *           type: string
 *           example: "DDoS_LOIC"
 */
