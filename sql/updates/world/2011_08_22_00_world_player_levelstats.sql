DELETE FROM `player_levelstats` WHERE `race`=1 AND `class`=2 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=1 AND `class`=3 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=5 AND `class`=3 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=6 AND `class`=2 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=6 AND `class`=5 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=7 AND `class`=5 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=8 AND `class`=9 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=8 AND `class`=11 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=9 AND `class`=1 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=9 AND `class`=3 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=9 AND `class`=4 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=9 AND `class`=5 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=9 AND `class`=7 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=9 AND `class`=8 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=9 AND `class`=9 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=22 AND `class`=1 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=22 AND `class`=3 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=22 AND `class`=4 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=22 AND `class`=5 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=22 AND `class`=8 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=22 AND `class`=9 AND `level`=1;
DELETE FROM `player_levelstats` WHERE `race`=22 AND `class`=11 AND `level`=1;
INSERT INTO `player_levelstats`(`race`,`class`,`level`,`str`,`agi`,`sta`,`inte`,`spi`) VALUES
(1,2,1,20,23,21,20,21),
(1,3,1,20,23,21,20,21),
(5,3,1,19,21,22,18,26),
(6,2,1,27,15,24,15,23),
(6,5,1,25,15,22,17,25),
(7,5,1,15,23,19,25,23),
(8,9,1,21,22,21,18,23),
(8,11,1,22,22,21,18,23),
(9,1,1,20,22,22,23,18),
(9,3,1,17,25,21,23,19),
(9,4,1,18,25,21,23,18),
(9,5,1,17,22,20,25,21),
(9,7,1,18,22,21,24,20),
(9,8,1,17,22,20,26,20),
(9,9,1,17,22,21,25,20),
(22,1,1,26,22,22,16,19),
(22,3,1,23,25,21,16,20),
(22,4,1,23,25,21,16,19),
(22,5,1,23,22,20,18,22),
(22,8,1,23,22,20,19,21),
(22,9,1,23,22,21,18,21),
(22,11,1,24,22,20,18,21);