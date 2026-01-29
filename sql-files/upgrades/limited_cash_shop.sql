-- Limited Cash Shop System
-- Adds support for per-account limited sales tracking

-- Modify sales table to add id as primary key
ALTER TABLE `sales` DROP PRIMARY KEY;
ALTER TABLE `sales` ADD `id` INT NOT NULL FIRST;
ALTER TABLE `sales` ADD PRIMARY KEY (`id`);
ALTER TABLE `sales` CHANGE `id` `id` INT NOT NULL AUTO_INCREMENT;
ALTER TABLE `sales` ADD UNIQUE KEY `nameid` (`nameid`);
ALTER TABLE `sales` ADD `rentalTime` INT NOT NULL DEFAULT '0' AFTER `amount`;

-- Create table for tracking individual account purchases
-- Note: sales_id references sales.id (not nameid) to properly track purchases per promotion period
CREATE TABLE IF NOT EXISTS `sales_limited_acc` (
  `sales_id` INT NOT NULL,
  `account_id` INT NOT NULL,
  `amount` INT NOT NULL DEFAULT '0'
) ENGINE=MyISAM;

ALTER TABLE `sales_limited_acc`
  ADD PRIMARY KEY (`sales_id`, `account_id`);
