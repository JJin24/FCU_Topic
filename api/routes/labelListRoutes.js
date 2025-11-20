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

/**
 * @swagger
 * /labelList/HourlyAttackTypesByLocation:
 *   get:
 *     summary: 取得指定地點「近一小時」收到的「攻擊類型列表」(不重複)
 *     description: 回傳一個包含指定地點在近一小時內收到的所有不重複攻擊類型標籤的陣列。
 *     tags: [labelList]
 *     parameters:
 *       - in: query
 *         name: location
 *         schema:
 *           type: string
 *         required: true
 *         description: 要篩選的裝置地點 (例如 "資電大樓")
 *         example: "資電大樓"
 *     responses:
 *       200:
 *         description: 成功取得攻擊類型列表。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 type: object
 *                 properties:
 *                   label:
 *                     type: integer
 *                     example: 3
 *                   label_name:
 *                     type: string
 *                     example: "DoS_GoldenEye"
 *       400:
 *         description: 缺少或無效的查詢參數。
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: Validation Error
 *                 message:
 *                   type: string
 *                   example: 缺少 'location' 查詢參數
 *       500:
 *         description: 伺服器錯誤，無法從資料庫取得資料。
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: Server Error
 *                 message:
 *                   type: string
 *                   example: Failed to retrieve data from MariaDB.
 */

router.get('/HourlyAttackTypesByLocation', labelListController.handleGetHourlyAttackTypesByLocation);
module.exports = router;

/**
 * @swagger
 * components:
 *   schemas:
 *     AttackType:
 *       type: object
 *       properties:
 *          label:
 *            type: integer
 *            example: 3
 *          label_name:
 *            type: string
 *            example: "DoS_GoldenEye"
 */

/**
 * @swagger
 * /labelList/AllAttackTypesByLocation:
 *   get:
 *     summary: 取得指定地點在過去所收到的所有「攻擊類型列表」(不重複)
 *     description: 回傳一個包含指定地點在過去收到的所有不重複攻擊類型標籤的陣列。
 *     tags: [labelList]
 *     parameters:
 *       - in: query
 *         name: location
 *         schema:
 *           type: string
 *         required: true
 *         description: 要篩選的裝置地點 (例如 "資電大樓")
 *         example: "資電大樓"
 *     responses:
 *       200:
 *         description: 成功取得攻擊類型列表。
 *         content:
 *           application/json:
 *             schema:
 *               type: array
 *               items:
 *                 type: object
 *                 properties:
 *                   label:
 *                     type: integer
 *                     example: 3
 *                   label_name:
 *                     type: string
 *                     example: "DoS_GoldenEye"
 *       400:
 *         description: 缺少或無效的查詢參數。
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: Validation Error
 *                 message:
 *                   type: string
 *                   example: 缺少 'location' 查詢參數
 *       500:
 *         description: 伺服器錯誤，無法從資料庫取得資料。
 *         content:
 *           application/json:
 *             schema:
 *               type: object
 *               properties:
 *                 title:
 *                   type: string
 *                   example: Server Error
 *                 message:
 *                   type: string
 *                   example: Failed to retrieve data from MariaDB.
 */

router.get('/AllAttackTypesByLocation', labelListController.handleGetAllAttackTypesByLocation);
module.exports = router;

/**
 * @swagger
 * components:
 *   schemas:
 *     AttackType:
 *       type: object
 *       properties:
 *          label:
 *            type: integer
 *            example: 3
 *          label_name:
 *            type: string
 *            example: "DoS_GoldenEye"
 */