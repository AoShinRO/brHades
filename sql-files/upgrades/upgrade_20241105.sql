ALTER TABLE `char`
	ADD COLUMN `show_costumes` TINYINT(1) UNSIGNED NOT NULL DEFAULT '1' AFTER `disable_call`
;