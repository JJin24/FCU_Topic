const express = require('express');
const swaggerJSDoc = require('swagger-jsdoc');
const swaggerUi = require('swagger-ui-express');
const index = express();
const port = 3000;

// Swagger 定義
const swaggerDefinition = {
  openapi: '3.0.0',
  info: {
    title: 'FCU_Topic API 文件',
    version: '1.2.0',
    description: '這一個 API 文件包含了提示所有 API 的使用方式 (對於與後端資料庫的通訊)。',
  }
};

const options = {
  swaggerDefinition,
  // 指向包含 API 註解的檔案路徑
    apis: ['./index.js', './routes/*.js'], // 指定掃描 index.js 和 routes 資料夾下所有的 .js 檔案
};

const swaggerSpec = swaggerJSDoc(options);

index.use(express.json());

const flowRouter = require('./routes/flowRoutes');
const hostRouter = require('./routes/hostRoutes');
const alertRouter = require('./routes/alertRoutes');
const labelListRouter = require('./routes/labelListRoutes');
const protocolListRouter = require('./routes/protocolListRoutes');
const dayRouter = require('./routes/dayRoutes');
const notifyRouter = require('./routes/notifyRoutes');


// 只要是 /flow 開頭的路由都交給 flowRouter 處理
index.use('/api-docs', swaggerUi.serve, swaggerUi.setup(swaggerSpec));
index.use('/flow', flowRouter);
index.use('/host', hostRouter);
index.use('/alert', alertRouter);
index.use('/labelList', labelListRouter);
index.use('/protocolList', protocolListRouter);
index.use('/day', dayRouter);
index.use('/notify', notifyRouter);

index.get('/', (req, res) => {
  res.send('The server is running');
});

/**
 * @swagger
 * /*:
 *   all:
 *     tags:
 *       - Default
 *     summary: Handles all undefined routes (404 Not Found).
 *     description: This endpoint catches all requests to routes that are not defined. It returns a 404 Not Found error.
 *     responses:
 *       404:
 *         description: The requested resource was not found.
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
 *                   example: The requested resource was not found on this server.
 */
index.use((req, res, next) => {
  res.status(404).json({
    title: '404 Not Found',
    message: 'The requested resource was not found on this server.'
  });
});

index.listen(port, () =>{
  console.log("Server is running on port " + port);
});