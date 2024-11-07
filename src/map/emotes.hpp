/**
+ *
+ * Copyright (C) 2023 Asheraf
+ *
+ * Hercules is free software: you can redistribute it and/or modify
+ * it under the terms of the GNU General Public License as published by
+ * the Free Software Foundation, either version 3 of the License, or
+ * (at your option) any later version.
+ *
+ * This program is distributed in the hope that it will be useful,
+ * but WITHOUT ANY WARRANTY; without even the implied warranty of
+ * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
+ * GNU General Public License for more details.
+ *
+ * You should have received a copy of the GNU General Public License
+ * along with this program.  If not, see <http://www.gnu.org/licenses/>.
+ */
#ifndef MAP_EMOTES_H
#define MAP_EMOTES_H

#include "../common/db.hpp"
#include "../common/database.hpp"

#include "pc.hpp"
#include "clif.hpp"

#include <vector>


enum emote_msg : int8 {
	EMSG_EMOTION_EXPANTION_USE_FAIL_DATE = 0,
	EMSG_EMOTION_EXPANTION_USE_FAIL_UNPURCHASED = 1,
	EMSG_EMOTION_USE_FAIL_SKILL_LEVEL = 2,
	EMSG_EMOTION_EXPANTION_USE_FAIL_UNKNOWN = 3, // default
};

enum emotion_expansion_msg : int8 {
	EMSG_EMOTION_EXPANTION_NOT_ENOUGH_NYANGVINE = 0,
	EMSG_EMOTION_EXPANTION_FAIL_DATE = 1,
	EMSG_EMOTION_EXPANTION_FAIL_ALREADY_BUY = 2,
	EMSG_EMOTION_EXPANTION_FAIL_ANOTHER_SALE_BUY = 3,
	EMSG_EMOTION_EXPANTION_NOT_ENOUGH_BASICSKILL_LEVEL = 4,
	EMSG_NOT_YET_SALE_START_TIME = 5,
	EMSG_EMOTION_EXPANTION_FAIL_UNKNOWN = 6, // default
};

struct s_cash_emotes_db
{
	uint16 packId;
	uint16 packType;
	uint16 packPrice;
	std::vector<e_emotion_type> emoteIds;
	time_t sale_start;
	time_t sale_end;
	uint64 rental_period;
};

class CashEmotesDatabase : public TypesafeYamlDatabase<int16, s_cash_emotes_db>
{
public:
	CashEmotesDatabase() : TypesafeYamlDatabase("CASH_EMOTES_DB", 1) {}

	const std::string getDefaultLocation() override;
	uint64 parseBodyNode(const ryml::NodeRef &node) override;
};

extern CashEmotesDatabase cash_emotes_db;

void emotes_use(map_session_data *sd, int16 packId, int16 emoteId);
void emotes_expantion_buy(map_session_data *sd, int16 packId, int16 itemId, int8 amount);
void emotes_get_player_packs(map_session_data *sd);

void do_init_emotes(void);
void do_final_emotes(void);

#endif /* MAP_EMOTES_H */
