-- --------------------------------------------------------
-- 主機:                           
-- 伺服器版本:                        10.11.13-MariaDB-0ubuntu0.24.04.1 - Ubuntu 24.04
-- 伺服器作業系統:                      debian-linux-gnu
-- HeidiSQL 版本:                  12.14.0.7165
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;


-- 傾印 Topic 的資料庫結構
CREATE DATABASE IF NOT EXISTS `Topic` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci */;
USE `Topic`;

-- 傾印  資料表 Topic.alert_history 結構
CREATE TABLE IF NOT EXISTS `alert_history` (
  `id` bigint(8) unsigned NOT NULL DEFAULT 0,
  `pcap` varbinary(8196) DEFAULT NULL,
  `score` float NOT NULL,
  `label` tinyint(1) unsigned NOT NULL DEFAULT 0,
  `status` tinyint(1) unsigned NOT NULL DEFAULT 0 COMMENT '0 是尚未處理，1 使已經處理',
  PRIMARY KEY (`id`),
  KEY `label` (`label`),
  CONSTRAINT `label` FOREIGN KEY (`label`) REFERENCES `label_list` (`label_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='所有告警結果會記錄到這一張表上面，同一個 flow 的資訊會以 id 與 flow 的 id 進行關聯。';

-- 取消選取資料匯出。

-- 傾印  資料表 Topic.Cred 結構
CREATE TABLE IF NOT EXISTS `Cred` (
  `Cred_Range` tinyint(1) unsigned NOT NULL DEFAULT 0,
  `Credibility` char(32) NOT NULL,
  `Score` float unsigned NOT NULL DEFAULT 0,
  PRIMARY KEY (`Cred_Range`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='依據模型辨識結果來判斷可信度（棄用）';

-- 取消選取資料匯出。

-- 傾印  資料表 Topic.flow 結構
CREATE TABLE IF NOT EXISTS `flow` (
  `id` bigint(8) unsigned NOT NULL AUTO_INCREMENT,
  `timestamp` datetime(3) NOT NULL DEFAULT utc_timestamp(3),
  `src_ip` inet6 DEFAULT '::',
  `dst_ip` inet6 DEFAULT '::',
  `src_port` smallint(2) unsigned DEFAULT 0,
  `dst_port` smallint(2) unsigned DEFAULT 0,
  `protocol` tinyint(1) unsigned DEFAULT 0,
  `uuid` uuid DEFAULT uuid(),
  UNIQUE KEY `uid` (`id`),
  KEY `id` (`id`),
  KEY `protocal` (`protocol`) USING BTREE,
  CONSTRAINT `protocol` FOREIGN KEY (`protocol`) REFERENCES `protocol_list` (`protocol`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=178204 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='裡面記錄著該 Flow 的基本資訊';

-- 取消選取資料匯出。

-- 傾印  資料表 Topic.host 結構
CREATE TABLE IF NOT EXISTS `host` (
  `name` char(32) NOT NULL DEFAULT 'HostName' COMMENT 'HostName',
  `location` char(32) NOT NULL DEFAULT 'Location' COMMENT '位置',
  `ip` inet6 NOT NULL DEFAULT '::' COMMENT 'IPv6 的位置，v4 使用 ::ffff: 來儲存',
  `gateway` inet6 NOT NULL DEFAULT '::',
  `mac_addr` bigint(8) unsigned NOT NULL DEFAULT 0 COMMENT 'MAC 地址需要轉換',
  `status` tinyint(1) DEFAULT NULL COMMENT '設定成 TINYINT(1) 模仿 Boolean 的欄位，',
  `department` int(10) unsigned DEFAULT 0,
  `importance` tinyint(1) DEFAULT 0 COMMENT '重要程度',
  CONSTRAINT `檢查 Location 的長度` CHECK (char_length(`location`) <= 32),
  CONSTRAINT `檢查 Name 的長度` CHECK (char_length(`name`) <= 32)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='記錄網路架構裡面的裝置';

-- 取消選取資料匯出。

-- 傾印  資料表 Topic.label_list 結構
CREATE TABLE IF NOT EXISTS `label_list` (
  `label_id` tinyint(1) unsigned NOT NULL DEFAULT 0,
  `name` char(32) NOT NULL COMMENT '類別 id 對應的名稱',
  PRIMARY KEY (`label_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='對應模型中每一個 ID 所對定的類型';

-- 取消選取資料匯出。

-- 傾印  資料表 Topic.notification_token 結構
CREATE TABLE IF NOT EXISTS `notification_token` (
  `id` bigint(8) unsigned NOT NULL AUTO_INCREMENT,
  `name` char(50) DEFAULT NULL,
  `FCMToken` char(255) DEFAULT NULL,
  KEY `id` (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=31 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- 取消選取資料匯出。

-- 傾印  資料表 Topic.protocol_list 結構
CREATE TABLE IF NOT EXISTS `protocol_list` (
  `protocol` tinyint(1) unsigned NOT NULL,
  `name` char(32) NOT NULL,
  PRIMARY KEY (`protocol`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='記錄每一個 protocol 所對定的 ID（可參考 wiki）';

-- 取消選取資料匯出。

-- 傾印  資料表 Topic.traffic_table 結構
CREATE TABLE IF NOT EXISTS `traffic_table` (
  `id` bigint(20) NOT NULL AUTO_INCREMENT,
  `traffic` bigint(20) NOT NULL DEFAULT 0,
  `interval_seconds` int(11) NOT NULL,
  `timestamp` datetime NOT NULL DEFAULT utc_timestamp(3),
  KEY `id` (`id`) USING BTREE
) ENGINE=InnoDB AUTO_INCREMENT=6955 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci COMMENT='每 x 秒的系統抓取的流量大小（MB）';

-- 取消選取資料匯出。

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
