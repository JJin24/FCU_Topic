const trafficService = require('../services/trafficService');

const handleGetOneHourSum = async (req, res) => {
  const result = await trafficService.getOneHourSum();

  if (result) {
    res.json(result);
  }
  else {
    res.status(404).json({ title: '404 Not Found', message: 'Failed to retrieve one hour traffic sum data' });
  }
};

// POST /traffic/report
const handleReportTraffic = async (req, res) => {
  try {
    const { src_ip,interval_seconds, bytes } = req.body;
    console.log(`Received traffic report: src_ip=${src_ip}, interval_seconds=${interval_seconds}, bytes=${bytes}`);
    if (typeof interval_seconds !== 'number' || typeof bytes !== 'number') {
      return res.status(400).json({ message: 'Bad request: interval_seconds and bytes are required as numbers' });
    }

    const ok = await trafficService.insertTraffic(src_ip,interval_seconds, bytes);
    if (ok) return res.status(201).json({ message: 'Recorded' });
    return res.status(500).json({ message: 'Failed to record traffic' });
  } catch (err) {
    console.error('handleReportTraffic error', err);
    return res.status(500).json({ message: 'Internal error' });
  }
};

module.exports = {
  handleGetOneHourSum
  , handleReportTraffic
};