-- MySQL dump 10.11
--
-- Host: localhost    Database: realmd
-- ------------------------------------------------------
-- Server version	5.0.45-Debian_1ubuntu3.1-log

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

-- 
-- Table structure for table `account_access`
-- 

DROP TABLE IF EXISTS `account_access`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `account_access` (
  `id` INT(11) UNSIGNED NOT NULL,
  `gmlevel` TINYINT(3) UNSIGNED NOT NULL,
  `RealmID` INT(11) NOT NULL DEFAULT '-1',
  PRIMARY KEY  (`id`,`RealmID`)
) ENGINE=MYISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC;
/*!40101 SET character_set_client = @saved_cs_client */;

-- 
-- Dumping data for table `account_access`
-- 

LOCK TABLES `account_access` WRITE;
/*!40000 ALTER TABLE `account_access` DISABLE KEYS */;
/*!40000 ALTER TABLE `account_access` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `account`
--

DROP TABLE IF EXISTS `account`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `account` (
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT COMMENT 'Identifier',
  `username`      VARCHAR(32) NOT NULL DEFAULT '',
  `sha_pass_hash` VARCHAR(40) NOT NULL DEFAULT '',
  `sessionkey` LONGTEXT,
  `v` LONGTEXT,
  `s` LONGTEXT,
  `email` TEXT,
  `joindate` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `last_ip` VARCHAR(30) NOT NULL DEFAULT '127.0.0.1',
  `failed_logins` INT(11) UNSIGNED NOT NULL DEFAULT '0',
  `locked` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  `last_login` TIMESTAMP NOT NULL DEFAULT '0000-00-00 00:00:00',
  `online` TINYINT(4) NOT NULL DEFAULT '0',
  `expansion` TINYINT(3) UNSIGNED NOT NULL DEFAULT '2',
  `mutetime` BIGINT(40) UNSIGNED NOT NULL DEFAULT '0',
  `locale` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  `recruiter` INT(11) NOT NULL DEFAULT '0',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `idx_username` (`username`)
) ENGINE=MYISAM AUTO_INCREMENT=5 DEFAULT CHARSET=utf8 COLLATE=utf8_general_ci ROW_FORMAT=DYNAMIC COMMENT='Account System';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account`
--

LOCK TABLES `account` WRITE;
/*!40000 ALTER TABLE `account` DISABLE KEYS */;
/*!40000 ALTER TABLE `account` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `account_banned`
--

DROP TABLE IF EXISTS `account_banned`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `account_banned` (
  `id` INT(11) NOT NULL DEFAULT '0' COMMENT 'Account id',
  `bandate` BIGINT(40) NOT NULL DEFAULT '0',
  `unbandate` BIGINT(40) NOT NULL DEFAULT '0',
  `bannedby` VARCHAR(50) NOT NULL,
  `banreason` VARCHAR(255) NOT NULL,
  `active` TINYINT(4) NOT NULL DEFAULT '1',
  PRIMARY KEY  (`id`,`bandate`)
) ENGINE=MYISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Ban List';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `account_banned`
--

LOCK TABLES `account_banned` WRITE;
/*!40000 ALTER TABLE `account_banned` DISABLE KEYS */;
/*!40000 ALTER TABLE `account_banned` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `ip_banned`
--

DROP TABLE IF EXISTS `ip_banned`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `ip_banned` (
  `ip` VARCHAR(32) NOT NULL DEFAULT '127.0.0.1',
  `bandate` BIGINT(40) NOT NULL,
  `unbandate` BIGINT(40) NOT NULL,
  `bannedby` VARCHAR(50) NOT NULL DEFAULT '[Console]',
  `banreason` VARCHAR(255) NOT NULL DEFAULT 'no reason',
  PRIMARY KEY  (`ip`,`bandate`)
) ENGINE=MYISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Banned IPs';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `ip_banned`
--

LOCK TABLES `ip_banned` WRITE;
/*!40000 ALTER TABLE `ip_banned` DISABLE KEYS */;
/*!40000 ALTER TABLE `ip_banned` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `logs`
--

DROP TABLE IF EXISTS `logs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `logs` (
  `time` INT(14) NOT NULL,
  `realm` INT(4) NOT NULL,
  `type` INT(4) NOT NULL,
  `string` TEXT
) ENGINE=MYISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `logs`
--

LOCK TABLES `logs` WRITE;
/*!40000 ALTER TABLE `logs` DISABLE KEYS */;
/*!40000 ALTER TABLE `logs` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `realmcharacters`
--

DROP TABLE IF EXISTS `realmcharacters`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `realmcharacters` (
  `realmid` INT(11) UNSIGNED NOT NULL DEFAULT '0',
  `acctid` BIGINT(20) UNSIGNED NOT NULL,
  `numchars` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  PRIMARY KEY  (`realmid`,`acctid`),
  KEY (acctid)
) ENGINE=MYISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Realm Character Tracker';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `realmcharacters`
--

LOCK TABLES `realmcharacters` WRITE;
/*!40000 ALTER TABLE `realmcharacters` DISABLE KEYS */;
/*!40000 ALTER TABLE `realmcharacters` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `realmlist`
--

DROP TABLE IF EXISTS `realmlist`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `realmlist` (
  `id` INT(11) UNSIGNED NOT NULL AUTO_INCREMENT,
  `name` VARCHAR(32) NOT NULL DEFAULT '',
  `address` VARCHAR(32) NOT NULL DEFAULT '127.0.0.1',
  `port` INT(11) NOT NULL DEFAULT '8085',
  `icon` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  `color` TINYINT(3) UNSIGNED NOT NULL DEFAULT '2',
  `timezone` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  `allowedSecurityLevel` TINYINT(3) UNSIGNED NOT NULL DEFAULT '0',
  `population` FLOAT UNSIGNED NOT NULL DEFAULT '0',
  `gamebuild` INT(11) UNSIGNED NOT NULL DEFAULT '14480',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `idx_name` (`name`)
) ENGINE=MYISAM AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Realm System';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `realmlist`
--

LOCK TABLES `realmlist` WRITE;
/*!40000 ALTER TABLE `realmlist` DISABLE KEYS */;
INSERT INTO `realmlist` (`id`,`name`,`address`,`port`,`icon`,`color`,`timezone`,`allowedSecurityLevel`,`population`,`gamebuild`) VALUES
(1,'Singularity','127.0.0.1',8085,1,0,1,0,0,14480);
/*!40000 ALTER TABLE `realmlist` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `uptime`
--

DROP TABLE IF EXISTS `uptime`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `uptime` (
  `realmid` INT(11) UNSIGNED NOT NULL,
  `starttime` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
  `startstring` VARCHAR(64) NOT NULL DEFAULT '',
  `uptime` BIGINT(20) UNSIGNED NOT NULL DEFAULT '0',
  `maxplayers` SMALLINT(5) UNSIGNED NOT NULL DEFAULT '0',
  `revision` VARCHAR(255) NOT NULL DEFAULT 'Singularity',
  PRIMARY KEY  (`realmid`,`starttime`)
) ENGINE=MYISAM DEFAULT CHARSET=utf8 ROW_FORMAT=DYNAMIC COMMENT='Uptime system';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `uptime`
--

LOCK TABLES `uptime` WRITE;
/*!40000 ALTER TABLE `uptime` DISABLE KEYS */;
/*!40000 ALTER TABLE `uptime` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2008-01-10 11:37:06

-- Updated on 2010-01-29 23:21:45 GMT+1
