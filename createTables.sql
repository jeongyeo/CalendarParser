CREATE TABLE `FILE` (
  `cal_id` int(11) NOT NULL AUTO_INCREMENT,
  `file_Name` varchar(60) NOT NULL,
  `version` int(11) NOT NULL,
  `prod_id` varchar(256) NOT NULL,
  PRIMARY KEY (`cal_id`)
) ENGINE=InnoDB AUTO_INCREMENT=53 DEFAULT CHARSET=latin1;
CREATE TABLE `EVENT` (
  `event_id` int(11) NOT NULL AUTO_INCREMENT,
  `summary` varchar(1024) DEFAULT NULL,
  `start_time` datetime NOT NULL,
  `location` varchar(60) DEFAULT NULL,
  `organizer` varchar(256) DEFAULT NULL,
  `cal_file` int(11) NOT NULL,
  PRIMARY KEY (`event_id`),
  KEY `cal_file` (`cal_file`),
  CONSTRAINT `EVENT_ibfk_1` FOREIGN KEY (`cal_file`) REFERENCES `FILE` (`cal_id`) ON DELETE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=56 DEFAULT CHARSET=latin1;
CREATE TABLE `ALARM` (
  `alarm_id` int(11) NOT NULL AUTO_INCREMENT,
  `action` varchar(256) NOT NULL,
  `trigger` varchar(256) NOT NULL,
  `event` int(11) NOT NULL,
  PRIMARY KEY (`alarm_id`),
  KEY `event` (`event`),
  CONSTRAINT `ALARM_ibfk_1` FOREIGN KEY (`event`) REFERENCES `EVENT` (`event_id`) ON DELETE CASCADE
) ENGINE=InnoDB AUTO_INCREMENT=9 DEFAULT CHARSET=latin1;

