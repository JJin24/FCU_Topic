const pool = require('./database');
const maxmind = require('maxmind');
const path = require('path');

let mmdbLookup = null;

// getFlow 會取得 flow 資料表的後 1000 筆資料
// 並且使會取得 uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol
async function getAllFlow() {
  var conn;
  try{
    conn = await pool.getConnection();

    // 取得資料庫的資料
    const rows = await conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, status \
      FROM flow \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id  \
      ORDER BY flow.id ASC;"
    );
    console.log(rows);
    return rows;
  }
  catch(err){
    console.error('Error in getFlow', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getFlowByUUID(uuid) {
  var conn;
  try{
    conn = await pool.getConnection();

    const [rows] = await conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, status \
      FROM flow \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id  \
      WHERE flow.uuid = ? \
      ORDER BY flow.id ASC \;",
      [uuid]
    );

    console.log(rows); // 這裡會印出查詢到的資料陣列
    return rows;       // 4. 回傳真正的查詢結果
  }
  catch(err){
    console.error('Error in getAFlow', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getFlowByIP(ip){
  var conn;
  try{
    conn = await pool.getConnection();

    const ipFlow = await conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, alert_history.status \
      FROM flow \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      WHERE src_ip = ? OR dst_ip = ?",
      [ip, ip]
    );
    console.log(ipFlow);
    return ipFlow;
  }
  catch(err){
    console.error('Error in getIP', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getAllFlowIPCount(){
  var conn;
  try{
    conn = await pool.getConnection();
    
    const ipCount = await conn.query(
      "SELECT ip, COUNT(*) AS count \
      FROM (SELECT src_ip AS ip FROM flow UNION ALL SELECT dst_ip AS ip FROM flow) AS combined_ips \
      GROUP BY ip\
      ORDER BY ip ASC;");

    console.log(ipCount);
    return ipCount;
  }
  catch(err){
    console.error('Error in getIPCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getFlowProtocolCount(){
  var conn;
  try{
    conn = await pool.getConnection();

    const protocolCount = await conn.query(
      "SELECT protocol_list.name AS protocol, COUNT(flow.protocol) as count FROM flow LEFT JOIN protocol_list \
      ON flow.protocol = protocol_list.protocol GROUP BY protocol_list.protocol"
    );
    console.log(protocolCount);
    return protocolCount;
  }
  catch(err){
    console.error('Error in getProtocolCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getTopXDayFlow(x){
  var conn;
  try{
    conn = await pool.getConnection();

    const topXDayFlow = await conn.query(
      "SELECT uuid, timestamp, src_ip, src_port, dst_ip, dst_port, protocol_list.name AS protocol, label_list.name AS label, score, status \
      FROM flow \
      LEFT JOIN alert_history ON flow.id = alert_history.id \
      LEFT JOIN label_list ON alert_history.label = label_list.label_id  \
      LEFT JOIN protocol_list ON flow.protocol = protocol_list.protocol \
      WHERE `timestamp` >= NOW() - INTERVAL ? DAY ORDER BY timestamp ASC;",
      [x]
    );
    console.log(topXDayFlow);
    return topXDayFlow;
  }
  catch(err){
    console.error('Error in getTopXDayFlow', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function get24HourFlowCount(){
  const conn = await pool.getConnection();
  try {
    const rows = await conn.query(
      "SELECT src_ip, dst_ip, COUNT(*) AS count \
      FROM flow \
      WHERE TIMESTAMP >= NOW() - INTERVAL 24 HOUR \
      GROUP BY src_ip, dst_ip;"
    );
    console.log(rows);
    return rows;
  }
  catch (err) {
    console.error('Error in get24HourFlowCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

/* From Gemini
 * 1. WITH RECURSIVE HourlySeries AS (...) (遞迴 CTE)：
 *    - 這是實現「自動填滿」和「依時間排序」的關鍵。它會生成一個包含過去 24 個完整小時的序列。
 *    - 錨定成員 (SELECT CAST(DATE_FORMAT(...) AS DATETIME) AS hour_floor)：
 *        NOW() - INTERVAL 23 HOUR：計算 23 小時前的時間點。
 *        DATE_FORMAT(..., '%Y-%m-%d %H:00:00')：將這個時間點截斷到小時的開始（例如，如果現在是 13:40，23 小時前是 14:40 昨天，截斷後就是昨天的 14:00:00）。
 *        CAST(... AS DATETIME)：確保結果是 DATETIME 類型，以便後續的時間計算。這個就是我們的起始小時。
 *    - 遞迴成員 (UNION ALL SELECT ... FROM HourlySeries WHERE ...)：
 *        hour_floor + INTERVAL 1 HOUR：在前一個小時的基礎上增加一小時。
 *        WHERE hour_floor + INTERVAL 1 HOUR <= CAST(DATE_FORMAT(NOW(), '%Y-%m-%d %H:00:00') AS DATETIME)：遞迴會一直進行，直到產生的小時的開始時間等於或早於「當前小時的開始時間」（例如，如果現在是 13:40，它會包含今天的 13:00:00，但不會包含 14:00:00，因為那還沒開始）。
 *    - 結果：這個 HourlySeries CTE 會產生從「23 小時前的那一小時的開始」到「當前小時的開始」共 24 個連續的小時時間點。例如，如果現在是 13:40，它會生成昨天的 14:00:00, 15:00:00, ..., 23:00:00，然後今天的 00:00:00, 01:00:00, ..., 13:00:00。
 * 2. LEFT JOIN (與流量數據合併)：
 *    - 我們在 LEFT JOIN 的右側使用一個子查詢 (flow_counts) 來計算實際的每小時流量。
 *    - flow_counts 子查詢：
 *        WHERE TIMESTAMP >= NOW() - INTERVAL 24 HOUR：修正了這裡為 24 小時，確保只計算最近 24 小時內的流量。
 *        DATE_FORMAT(TIMESTAMP, '%Y-%m-%d %H:00:00')：將 flow 表中的每個 TIMESTAMP 也截斷到小時，以便與 HourlySeries 中的 hour_floor 進行匹配。
 *        GROUP BY DATE_FORMAT(TIMESTAMP, '%Y-%m-%d %H:00:00')：根據截斷後的小時進行計數。
 *        ON h.hour_floor = flow_counts.flow_hour_floor：這將 HourlySeries 中的每個小時時間點與實際有流量的小時計數進行匹配。
 *        COALESCE(flow_counts.count, 0) AS count：這是「自動填滿 0」的關鍵。如果 LEFT JOIN 沒有找到匹配的流量數據（即某個小時沒有流量），那麼 flow_counts.count 將會是 NULL，COALESCE 函數會將 NULL 替換為 0。
 * 3. ORDER BY h.hour_floor ASC：
 *    - 這確保了結果是按照小時的完整時間戳記進行升序排列，從而實現了你要求的「前一天 22, 23, 今天 0, 1 ..., 21」的嚴格時間順序。
 */
async function getPerHourAllFlowCount(){
  var conn;
  try{
    conn = await pool.getConnection();

    const perHourAllFlowCount = await conn.query(
    "WITH RECURSIVE HourlySeries AS ( \
    SELECT \
        CAST(DATE_FORMAT(NOW() - INTERVAL 23 HOUR, '%Y-%m-%d %H:00:00') AS DATETIME) AS hour_floor \
    UNION ALL \
    SELECT \
        hour_floor + INTERVAL 1 HOUR \
    FROM \
        HourlySeries \
    WHERE \
        hour_floor + INTERVAL 1 HOUR <= CAST(DATE_FORMAT(NOW(), '%Y-%m-%d %H:00:00') AS DATETIME) \
    ) \
    SELECT \
        DATE_FORMAT(h.hour_floor, '%Y-%m-%d %H:00:00') AS full_hour_timestamp, \
        HOUR(h.hour_floor) AS hour, \
        COALESCE(flow_counts.count, 0) AS count \
    FROM \
        HourlySeries h \
    LEFT JOIN ( \
    SELECT \
        DATE_FORMAT(TIMESTAMP, '%Y-%m-%d %H:00:00') AS flow_hour_floor, \
        COUNT(*) AS count \
    FROM \
        flow \
    WHERE \
        TIMESTAMP >= NOW() - INTERVAL 24 HOUR \
    GROUP BY \
        DATE_FORMAT(TIMESTAMP, '%Y-%m-%d %H:00:00') \
    ) AS flow_counts ON h.hour_floor = flow_counts.flow_hour_floor \
    ORDER BY h.hour_floor ASC;"
    );
    console.log(perHourAllFlowCount);
    return perHourAllFlowCount;
  }
  catch(err){
    console.error('Error in getPerHourAllFlowCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getGoodMalCount(){
  var conn;
  try{
    conn = await pool.getConnection();

    const [goodMalCount] = await conn.query(
      "SELECT COUNT(CASE WHEN AH.ID IS NULL THEN F.ID END) AS goodFlowCount, COUNT(AH.ID) AS badFlowCount \
      FROM flow F \
      LEFT JOIN alert_history AH ON F.ID = AH.ID;"
    );
    console.log(goodMalCount);
    return goodMalCount;
  }
  catch(err){
    console.error('Error in getGoodMalCount', err);
  }
  finally {
    if (conn) conn.release(); // 釋放連線
  }
}

async function getLocationGraph(locationName) {
  var conn;
  try {
    conn = await pool.getConnection();
    console.log("Get Location Graph API");
    const query = `
    WITH RECURSIVE TimeSeries AS (
        SELECT 
            FROM_UNIXTIME(FLOOR(UNIX_TIMESTAMP(NOW() - INTERVAL 1 HOUR) / 30) * 30) AS interval_start
        UNION ALL
        SELECT 
            interval_start + INTERVAL 30 SECOND
        FROM TimeSeries
        WHERE interval_start + INTERVAL 30 SECOND < NOW()
    ),
    BucketedFlows AS (
        -- 2A. 原始流量資料與主機/告警聯接，並計算分桶時間
        SELECT 
            -- 計算並產生 30 秒間隔的起始時間戳
            FROM_UNIXTIME(FLOOR(UNIX_TIMESTAMP(f.timestamp) / 30) * 30) AS interval_start,
            h.location, 
            (ah.id IS NULL) AS is_good
        FROM flow f
        LEFT JOIN alert_history ah ON f.id = ah.id
        JOIN host h ON (f.src_ip = h.ip OR f.dst_ip = h.ip) 
        WHERE 
            f.timestamp >= NOW() - INTERVAL 1 HOUR
    ),
    AggregatedFlows AS (
        -- 2B. 聚合流量指標
        SELECT
            interval_start,
            COALESCE(SUM(CASE WHEN bf.location = ? THEN bf.is_good END), 0) AS location_good,
            COALESCE(SUM(CASE WHEN bf.location = ? AND NOT bf.is_good THEN 1 ELSE 0 END), 0) AS location_mal,
            COALESCE(SUM(bf.is_good), 0) AS all_good,
            COALESCE(SUM(CASE WHEN NOT bf.is_good THEN 1 ELSE 0 END), 0) AS all_mal
        FROM BucketedFlows bf
        GROUP BY interval_start
    ),
    AggregatedTraffic AS (
        -- 3. 聚合流量資料
        SELECT 
            FROM_UNIXTIME(FLOOR(UNIX_TIMESTAMP(t.timestamp) / 30) * 30) AS interval_start, 
            COALESCE(SUM(t.traffic), 0) AS all_traffic
        FROM traffic_table t
        WHERE 
            t.timestamp >= NOW() - INTERVAL 1 HOUR
            -- 核心優化: 對流量資料也應用「前 10 秒」的過濾規則
            AND (UNIX_TIMESTAMP(t.timestamp) % 30) < 10
        GROUP BY interval_start
    )
    -- 4. 最終結果集：使用精確的 interval_start 進行聯接
    SELECT 
        DATE_FORMAT(ts.interval_start, '%Y-%m-%dT%H:%i:%sZ') as interval_start,
        COALESCE(af.location_good, 0) AS location_good,
        COALESCE(af.location_mal, 0) AS location_mal,
        COALESCE(af.all_good, 0) AS all_good,
        COALESCE(af.all_mal, 0) AS all_mal,
        COALESCE(at.all_traffic, 0) AS all_traffic
    FROM TimeSeries ts
    LEFT JOIN AggregatedFlows af ON ts.interval_start = af.interval_start
    LEFT JOIN AggregatedTraffic at ON ts.interval_start = at.interval_start
    ORDER BY ts.interval_start;
    `;

    const rows = await conn.query(query, [locationName, locationName]);
    console.log(rows);
    return rows;
  } catch (err) {
    console.error('Error in getLocationGraph', err);
    throw err;
  } finally {
    if (conn) conn.release();
  }
}

async function getTopNFlows(){
  var conn;
  try{
    conn = await pool.getConnection();

    if (!mmdbLookup) {
      const dbPath = path.join(__dirname, '..', 'ipinfo.mmdb');
      mmdbLookup = await maxmind.open(dbPath);
    }

    const rows = await conn.query(
      `SELECT 
          derived.ip, 
          h.name AS hostname,
          derived.total_frequency
      FROM (
          SELECT ip, COUNT(*) AS total_frequency
          FROM (
              SELECT src_ip AS ip FROM flow WHERE timestamp >= NOW() - INTERVAL 324 HOUR
              UNION ALL
              SELECT dst_ip AS ip FROM flow WHERE timestamp >= NOW() - INTERVAL 324 HOUR
          ) AS union_ips
          GROUP BY ip
          ORDER BY total_frequency DESC
          LIMIT 20
      ) AS derived
      LEFT JOIN host h ON derived.ip = h.ip
      ORDER BY derived.total_frequency DESC;`
    );
    const result = rows.map(row => {
      let ip = row.ip;

      // [關鍵步驟] 在這裡處理 ::ffff:
      if (ip && ip.startsWith('::ffff:')) {
          ip = ip.substring(7); // 去掉前 7 個字元 (::ffff:)
      }

      let country = '';
      let asName = '';
      let finalHostname = row.db_hostname; // 優先使用內部資料庫的 hostname

      // 判斷 Private IP (內網)
      const isPrivate = ip.startsWith('10.') || 
                        ip.startsWith('192.168.') || 
                        ip.startsWith('127.') ||
                        (ip.startsWith('172.') && parseInt(ip.split('.')[1]) >= 16 && parseInt(ip.split('.')[1]) <= 31);

      if (isPrivate) {
          country = 'LAN';
          asName = 'Private Network';
          // 如果 DB 沒名字，給個預設值
          if (!finalHostname) finalHostname = 'Internal Device';
      } else {
          const info = mmdbLookup.get(ip);

          if (info) {
              // 取得 Country
              country = info.country || info.country_code || 'Unknown';

              // 取得 AS Name (ISP/組織) - IPinfo 通常在 'org' 欄位
              asName = info.org || info.asn_organization || 'Unknown ISP';

              // 如果 DB 沒名字，嘗試用 MMDB 補全
              if (!finalHostname) {
                  // 優先順序: MMDB的hostname > MMDB的org > Unknown
                  finalHostname = info.hostname || asName || 'External Host';
              }
          } else {
              country = 'Unknown';
              asName = 'Unknown';
              if (!finalHostname) finalHostname = 'Unknown Host';
          }
      }

      // ---------------------------------------------------------
      // 4. 回傳指定的欄位
      // ---------------------------------------------------------
      return {
          hostname: finalHostname,
          total_frequency: row.total_frequency,
          ip: ip,           // 乾淨的 IP
          country: country,
          as_name: asName
      };
        });

        return result;
            } catch (err) {
        console.error('getTopNFlows Error:', err);
        throw err;
    } finally {
        if (conn) conn.release();
    }
}

module.exports = {
  getAllFlow,
  getFlowByUUID,
  getFlowByIP,
  getAllFlowIPCount,
  getFlowProtocolCount,
  getTopXDayFlow,
  get24HourFlowCount,
  getPerHourAllFlowCount,
  getGoodMalCount,
  getLocationGraph,
  getTopNFlows
};
