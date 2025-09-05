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
 */
router.get('/:xDay/ip/:IP', dayController.handleGetXDayAIP);
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
 */
router.get('/:xDay/hostAllCount', dayController.handleGetXDayHostAllCount);
router.get('/:xDay/hostMalCount', dayController.handleGetXDayHostMalCount);
router.get('/:xDay/hostAttackCount', dayController.handleGetXDayHostAttackCount);

module.exports = router;
