-- Limited Cash Shop System
-- Adds support for per-account limited sales tracking

-- Create table for tracking individual account purchases
CREATE TABLE IF NOT EXISTS `sales_limited_acc` (
  `sales_id` INT(11) NOT NULL,
  `account_id` INT(11) NOT NULL,
  `amount` INT(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`sales_id`, `account_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- Add new columns to sales table
ALTER TABLE `sales` 
  ADD COLUMN IF NOT EXISTS `id` INT(11) NOT NULL AUTO_INCREMENT FIRST,
  ADD COLUMN IF NOT EXISTS `rentalTime` INT(11) NOT NULL DEFAULT '0' AFTER `amount`,
  DROP PRIMARY KEY,
  ADD PRIMARY KEY (`id`),
  ADD UNIQUE KEY `nameid` (`nameid`);
