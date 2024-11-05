/**
 *
 * Copyright (C) 2023 Asheraf
 *
 * Hercules is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "emotes.hpp"

#include "../common/conf.hpp"
#include "../common/nullpo.hpp"
#include "../common/random.hpp"
#include "../common/showmsg.hpp"
#include "../common/timer.hpp"
#include "../common/utils.hpp"
#include "../common/socket.hpp" // last_tick

#include "battle.hpp"
#include "clif.hpp"
#include "log.hpp"
#include "packets.hpp"
#include "script.hpp"

#include <stdlib.h>

using namespace brhades;

CashEmotesDatabase cash_emotes_db;

std::time_t convertToUnixTimestamp(uint64_t dateValue)
{
    std::tm tmStruct = {};
    tmStruct.tm_year = static_cast<int>(dateValue / 10000) - 1900;
    tmStruct.tm_mon = static_cast<int>((dateValue / 100) % 100) - 1; 
    tmStruct.tm_mday = static_cast<int>(dateValue % 100); 

    return std::mktime(&tmStruct);
}

const std::string CashEmotesDatabase::getDefaultLocation() {
	return std::string(db_path) + "/cash_emotes_db.yml";
}

uint64 CashEmotesDatabase::parseBodyNode(const ryml::NodeRef& node)
{
	uint16 packId;
	if (!this->asUInt16(node, "PackId", packId))
		return 0;

	std::shared_ptr<s_cash_emotes_db> ce = this->find(packId);
	bool exists = ce != nullptr;
	if (!exists) {
		ce = std::make_shared<s_cash_emotes_db>();
		ce->packId = packId;
	}

	if (!this->asUInt16(node, "PackType", ce->packType))
		return 0;

	if (!this->asUInt16(node, "PackPrice", ce->packPrice))
		return 0;

	uint64 sale_start;
	if (!this->asUInt64(node, "SaleStart", sale_start))
		return 0;
	ce->sale_start = !sale_start ? 0 : convertToUnixTimestamp(sale_start);

	uint64 sale_end;
	if (!this->asUInt64(node, "SaleEnd", sale_end))
		return 0;
	ce->sale_end = !sale_end ? 0 : convertToUnixTimestamp(sale_end);

	if (!this->asUInt64(node, "RentalPeriod", ce->rental_period))
		return 0;
	ce->rental_period *= (60 * 60 * 24); // Convert from days to seconds

	for (const ryml::NodeRef& emoteNode : node["EmotesList"]) {
		if (!emoteNode.is_val()) {
			this->invalidWarning(node["EmotesList"], "Unknown format.\n");
			continue;
		}
		// Get the name of the node
		std::string emotename = emoteNode.val().data();
		size_t newline_pos = emotename.find('\n');
		if (newline_pos != std::string::npos) {
			emotename = emotename.substr(0, newline_pos);
		}		
		// Get the value of the constant
		int64 val;
		script_get_constant(emotename.c_str(), &val);

		// Verify it's a valid emotion
		if (val < 0 || val >= ET_MAX) {
			this->invalidWarning(node["EmotesList"], "Unknown emotion /%s/.\n", emotename.c_str());
			continue;
		}

		// Append to the pack
		ce->emoteIds.push_back(static_cast<e_emotion_type>(val));
	}
	if (!exists)
		this->put(packId, ce);
	return 1;
}

void emotes_use(map_session_data *sd, int16 packId, int16 emoteId)
{

	nullpo_retv(sd);

	// Check if the player can use any emotes at all
	if (battle_config.basic_skill_check != 0 && pc_checkskill(sd, NV_BASIC) < 2 && pc_checkskill(sd, SU_BASIC_SKILL) < 1) {
		clif_emotion_fail(sd, packId, emoteId, EMSG_EMOTION_USE_FAIL_SKILL_LEVEL);
		return;
	}

	// Verify emotes delay
	if (sd->emotionlasttime + 1 >= time(NULL)) { // not more than 1 per second
		sd->emotionlasttime = time(NULL);
		clif_emotion_fail(sd, packId, emoteId, EMSG_EMOTION_EXPANTION_USE_FAIL_UNKNOWN);
		return;
	}
	sd->emotionlasttime = time(NULL);

	// Update idle times
	if (battle_config.idletime_option & IDLE_EMOTION)
		sd->idletime = last_tick;
	if (battle_config.hom_idle_no_share && sd->hd && battle_config.idletime_hom_option & IDLE_EMOTION)
		sd->idletime_hom = last_tick;
	if (battle_config.mer_idle_no_share && sd->md && battle_config.idletime_mer_option & IDLE_EMOTION)
		sd->idletime_mer = last_tick;

	// Verify blocking state
	if (sd->state.block_action & PCBLOCK_EMOTION) {
		clif_emotion_fail(sd, packId, emoteId, EMSG_EMOTION_EXPANTION_USE_FAIL_UNKNOWN);
		return;
	}

	// Verify the pack exists
	auto ce = cash_emotes_db.find(packId);
	if (ce == nullptr) {
		clif_emotion_fail(sd, packId, emoteId, EMSG_EMOTION_EXPANTION_USE_FAIL_UNKNOWN);
		return;
	}

	// Verify the emote exists
	if (!util::vector_exists(ce->emoteIds, static_cast<e_emotion_type>(emoteId))) {
		clif_emotion_fail(sd, packId, emoteId, EMSG_EMOTION_EXPANTION_USE_FAIL_UNKNOWN);
		return;
	}

	// Pack 0 is freely available without needing to be purchased
	if (ce->packId != 0) {
		// Verify the player owns the pack or the packId is 0
		{
			std::string var_name = ce->packType == 1 ? "#cashemote_" : "cashemote_";
			var_name += std::to_string(ce->packId);

			int64 has_the_pack = pc_readglobalreg(sd, add_str(var_name.c_str()));
			if (has_the_pack == 0) {
				clif_emotion_fail(sd, packId, emoteId, EMSG_EMOTION_EXPANTION_USE_FAIL_UNPURCHASED);
				return;
			}
		}

		// Verify that the pack didn't expire if it has an expiration period
		{
			std::string var_name = ce->packType == 1 ? "#cashemoteexpire_" : "cashemoteexpire_";
			var_name += std::to_string(ce->packId);

			int64 expire_time = pc_readglobalreg(sd, add_str(var_name.c_str()));
			if (ce->rental_period != 0 && time(NULL) > expire_time) {
				clif_emotion_fail(sd, packId, emoteId, EMSG_EMOTION_EXPANTION_USE_FAIL_DATE);
				return;
			}
		}
	}

	// Reroll the dice if the config is enabled
	if (battle_config.client_reshuffle_dice && emoteId >= ET_DICE1 && emoteId <= ET_DICE6)
		emoteId = static_cast<e_emotion_type>(rnd() % 6 + ET_DICE1);

	// All good.. congrats!
	clif_emotion_success(&sd->bl, packId, emoteId);
}

void emotes_expantion_buy(map_session_data *sd, int16 packId, int16 itemId, int8 amount)
{
	nullpo_retv(sd);

	// Verify the item id client-hardcoded to 6909 Nyangvine_Fruit
	if (itemId != 6909) {
		clif_emotion_expansion_response_fail(sd, packId, EMSG_EMOTION_EXPANTION_FAIL_UNKNOWN);
		return;
	}

	// Verify the player has the skill requirements to use emotes
	if (battle_config.basic_skill_check != 0 && pc_checkskill(sd, NV_BASIC) < 2 && pc_checkskill(sd, SU_BASIC_SKILL) < 1) {
		clif_emotion_expansion_response_fail(sd, packId, EMSG_EMOTION_EXPANTION_NOT_ENOUGH_BASICSKILL_LEVEL);
		return;
	}

	// Verify the pack exists
	auto ce = cash_emotes_db.find(packId);
	if (ce == nullptr) {
		clif_emotion_expansion_response_fail(sd, packId, EMSG_EMOTION_EXPANTION_FAIL_UNKNOWN);
		return;
	}

	// Verify the amount in the packet equals the price of the pack
	if (amount != ce->packPrice) {
		clif_emotion_expansion_response_fail(sd, packId, EMSG_EMOTION_EXPANTION_FAIL_UNKNOWN);
		return;
	}

	// Avoid multiple calls
	time_t now = time(NULL);

	// Verify the pack sale date has commenced
	if (ce->sale_start != 0 && ce->sale_start > now) {
		clif_emotion_expansion_response_fail(sd, packId, EMSG_NOT_YET_SALE_START_TIME);
		return;
	}

	// Verify the sale end date has not passed
	if (ce->sale_end != 0 && ce->sale_end < now) {
		clif_emotion_expansion_response_fail(sd, packId, EMSG_EMOTION_EXPANTION_FAIL_DATE);
		return;
	}

	// Verify the player doesn't already own the pack
	{
		std::string var_name = ce->packType == 1 ? "#cashemote_" : "cashemote_";
		var_name += std::to_string(ce->packId);

		int64 has_the_pack = pc_readglobalreg(sd, add_str(var_name.c_str()));
		if (has_the_pack != 0) {
			clif_emotion_expansion_response_fail(sd, packId, EMSG_EMOTION_EXPANTION_FAIL_ALREADY_BUY);
			return;
		}
	}

	if (amount > 0) {
		// Verify the player has enough Nyangvine
		int inv_index = pc_search_inventory(sd, itemId);
		if (inv_index < 0 || sd->inventory.u.items_inventory[inv_index].amount < amount) {
			clif_emotion_expansion_response_fail(sd, packId, EMSG_EMOTION_EXPANTION_NOT_ENOUGH_NYANGVINE);
			return;
		}

		// Delete the items
		pc_delitem(sd, inv_index, amount, 0, 0, LOG_TYPE_CONSUME);
	}

	// Give the player the pack
	{
		std::string var_name = ce->packType == 1 ? "#cashemote_" : "cashemote_";
		var_name += std::to_string(ce->packId);

		pc_setglobalreg(sd, add_str(var_name.c_str()), 1);
	}

	// If the pack has a rental period apply that
	if (ce->rental_period != 0) {
		std::string var_name = ce->packType == 1 ? "#cashemoteexpire_" : "cashemoteexpire_";
		var_name += std::to_string(ce->packId);

		int64 expire_time = now + ce->rental_period;
		pc_setglobalreg(sd, add_str(var_name.c_str()), expire_time);

		// All good, update the client with the expiry time.
		clif_emotion_expansion_response_success(sd, packId, true, (uint32)expire_time);
		return;
	}

	// All good.. congrats! Unlimited pack
	clif_emotion_expansion_response_success(sd, packId, false, 0);
}

void emotes_get_player_packs(map_session_data *sd)
{
	nullpo_retv(sd);

	std::vector<PACKET_ZC_EMOTION_EXPANTION_LIST_sub> packs;

	// Iterate throught every pack in the database and get the player's owned packs from the variables
	//std::pair<uint16,std::shared_ptr<s_cash_emotes_db>>;
	for (std::pair<const uint16,std::shared_ptr<s_cash_emotes_db>> k : cash_emotes_db) {
		std::string var_name = k.second->packType == 1 ? "#cashemote_" : "cashemote_";
		var_name += std::to_string(k.second->packId);
		uint64 has_the_pack = pc_readglobalreg(sd, add_str(var_name.c_str()));

		std::string var_name2 = k.second->packType == 1 ? "#cashemoteexpire_" : "cashemoteexpire_";
		var_name2 += std::to_string(k.second->packId);
		uint64 expire_time = pc_readglobalreg(sd, add_str(var_name2.c_str()));

		// If the pack has already expired, remove it from the player's
		if (has_the_pack != 0 && k.second->rental_period != 0 && time(NULL) > expire_time) {
			pc_setglobalreg(sd, add_str(var_name.c_str()), 0);
			pc_setglobalreg(sd, add_str(var_name2.c_str()), 0);
			continue;
		}

		// Otherwise append to the list of available packs
		if (has_the_pack != 0)
			packs.push_back({ k.first, (k.second->rental_period != 0), (uint32)expire_time });
	}
	clif_emotion_expansion_list(sd, packs);
}

void do_init_emotes(void)
{
	cash_emotes_db.load();
}

void do_final_emotes(void)
{
	cash_emotes_db.clear();
}
