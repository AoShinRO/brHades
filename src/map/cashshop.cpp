// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "cashshop.hpp"

#include <cstdlib> // atoi
#include <cstring> // memset
#include <ctime>  // time functions

#include <common/cbasetypes.hpp> // uint16, uint32
#include <common/malloc.hpp> // CREATE, RECREATE, aFree
#include <common/showmsg.hpp> // ShowWarning, ShowStatus

#include "clif.hpp"
#include "log.hpp"
#include "pc.hpp" // s_map_session_data
#include "pet.hpp" // pet_create_egg

#if PACKETVER_SUPPORTS_SALES
struct sale_item_db sale_items;
#endif

extern char sales_table[32];

const std::string CashShopDatabase::getDefaultLocation(){
	return std::string( db_path ) + "/item_cash.yml";
}

uint64 CashShopDatabase::parseBodyNode( const ryml::NodeRef& node ){
	std::string name;

	if( !this->asString( node, "Tab", name ) ){
		return 0;
	}

	std::string tab_constant = "CASHSHOP_TAB_" + name;
	int64 constant;

	if( !script_get_constant( tab_constant.c_str(), &constant ) ){
		this->invalidWarning( node["Tab"], "Invalid tab %s, skipping.\n", tab_constant.c_str() );
		return 0;
	}

	if( constant < CASHSHOP_TAB_NEW || constant >= CASHSHOP_TAB_MAX ){
		this->invalidWarning( node["Tab"], "Tab %" PRId64 " is out of range, skipping.\n", constant );
		return 0;
	}

	e_cash_shop_tab tab = static_cast<e_cash_shop_tab>( constant );

	std::shared_ptr<s_cash_item_tab> entry = this->find( static_cast<uint16>( tab ) );
	bool exists = entry != nullptr;

	if( !exists ){
		if( !this->nodesExist( node, { "Items" } ) ){
			return 0;
		}

		entry = std::make_shared<s_cash_item_tab>();
		entry->tab = tab;
	}

	for( const ryml::NodeRef& it : node["Items"] ){
		std::string item_name;

		if( !this->asString( it, "Item", item_name ) ){
			return 0;
		}

		std::shared_ptr<item_data> item = item_db.search_aegisname( item_name.c_str() );

		if( item == nullptr ){
			this->invalidWarning( it["Item"], "Cash item %s does not exist, skipping.\n", item_name.c_str() );
			continue;
		}

		std::shared_ptr<s_cash_item> cash_item = nullptr;
		bool cash_item_exists = false;

		for( std::shared_ptr<s_cash_item> cash_it : entry->items ){
			if( cash_it->nameid == item->nameid ){
				cash_item = cash_it;
				cash_item_exists = true;
				break;
			}
		}

		if( !cash_item_exists ){
			cash_item = std::make_shared<s_cash_item>();
			cash_item->nameid = item->nameid;
		}

		uint32 price;

		if( !this->asUInt32( it, "Price", price ) ){
			return 0;
		}

		if( price == 0 ){
			this->invalidWarning( it["Price"], "Price has to be greater than zero." );
			return 0;
		}

		if( price > MAX_CASHPOINT ){
			this->invalidWarning( it["Price"], "Price has to be lower than MAX_CASHPOINT(%d).", MAX_CASHPOINT );
			return 0;
		}

		cash_item->price = price;

		if( !cash_item_exists ){
			entry->items.push_back( cash_item );
		}
	}

	if( !exists ){
		this->put( static_cast<uint16>( tab ), entry );
	}

	return 1;
}

std::shared_ptr<s_cash_item> CashShopDatabase::findItemInTab( e_cash_shop_tab tab, t_itemid nameid ){
	std::shared_ptr<s_cash_item_tab> cash_tab = this->find( static_cast<uint16>( tab ) );

	if( cash_tab == nullptr ){
		return nullptr;
	}

	for( std::shared_ptr<s_cash_item> cash_it : cash_tab->items ){
		if( cash_it->nameid == nameid ){
			return cash_it;
		}
	}

	return nullptr;
}

CashShopDatabase cash_shop_db;

#if PACKETVER_SUPPORTS_SALES
static bool sale_parse_dbrow( char* fields[], int32 columns, int32 current ){
	t_itemid nameid = strtoul(fields[0], nullptr, 10);
	int32 start = atoi(fields[1]), end = atoi(fields[2]), amount = atoi(fields[3]), id = atoi(fields[4]), rentalTime = atoi(fields[5]);
	time_t now = time(nullptr);
	struct sale_item_data* sale_item = nullptr;

	if( !item_db.exists(nameid) ){
		ShowWarning( "sale_parse_dbrow: ID invalido %u na linha '%d', pulando...\n",nameid, current );
		return false;
	}

	// Check if the item exists in the sales tab
	if( cash_shop_db.findItemInTab( CASHSHOP_TAB_SALE, nameid ) == nullptr ){
		ShowWarning( "sale_parse_dbrow: ID %u nao esta registrado na aba Venda na linha '%d', pulando...\n", nameid, current );
		return false;
	}

	// Check if the end is after the start
	if( start >= end ){
		ShowWarning( "sale_parse_dbrow: A venda do item %u foi ignorada porque o periodo nao estava correto.\n", nameid );
		return false;
	}

	// Check if it is already in the past
	if( end < now ){
		ShowWarning( "sale_parse_dbrow: Uma venda desatualizada para o item %u foi ignorada.\n", nameid );
		return false;
	}

	// Check if there is already an entry
	sale_item = sale_find_item(nameid,false);

	if( sale_item == nullptr ){
		RECREATE(sale_items.item, struct sale_item_data *, ++sale_items.count);
		CREATE(sale_items.item[sale_items.count - 1], struct sale_item_data, 1);
		sale_item = sale_items.item[sale_items.count - 1];
	}

	sale_item->id = id;
	sale_item->nameid = nameid;
	sale_item->start = start;
	sale_item->end = end;
	sale_item->amount = amount;
	sale_item->timer_start = INVALID_TIMER;
	sale_item->timer_end = INVALID_TIMER;
	sale_item->rentalTime = rentalTime;

	return true;
}

static void sale_read_db_sql( void ){
	uint32 lines = 0, count = 0;

	if( SQL_ERROR == Sql_Query( mmysql_handle, "SELECT `nameid`, UNIX_TIMESTAMP(`start`), UNIX_TIMESTAMP(`end`), `amount`, `id`, `rentalTime` FROM `%s` WHERE `end` > now()", sales_table ) ){
		Sql_ShowDebug(mmysql_handle);
		return;
	}

	while( SQL_SUCCESS == Sql_NextRow(mmysql_handle) ){
		char* str[6];
		char dummy[256] = "";
		int32 i;

		lines++;

		for( i = 0; i < 6; i++ ){
			Sql_GetData( mmysql_handle, i, &str[i], nullptr );

			if( str[i] == nullptr ){
				str[i] = dummy;
			}
		}

		if( !sale_parse_dbrow( str, 6, lines ) ){
			ShowError( "sale_read_db_sql: Nao e possivel processar a tabela '%s' na linha '%d', ignorando...\n", sales_table, lines );
			continue;
		}

		count++;
	}

	Sql_FreeResult(mmysql_handle);

	ShowStatus( "Concluida a leitura de '" CL_WHITE "%u" CL_RESET "' entradas em '" CL_WHITE "%s" CL_RESET "'.\n", count, sales_table );
}

static TIMER_FUNC(sale_end_timer){
	struct sale_item_data* sale_item = (struct sale_item_data*)data;

	// Remove the timer so the sale end is not sent out again
	delete_timer( sale_item->timer_end, sale_end_timer );
	sale_item->timer_end = INVALID_TIMER;
	
	map_foreachpc(clif_CashShopLimited_sub);

	sale_remove_item( sale_item->nameid );

	return 1;
}

static TIMER_FUNC(sale_start_timer){
	struct sale_item_data* sale_item = (struct sale_item_data*)data;

	map_foreachpc(clif_CashShopLimited_sub);

	// Clear the start timer
	if( sale_item->timer_start != INVALID_TIMER ){
		delete_timer( sale_item->timer_start, sale_start_timer );
		sale_item->timer_start = INVALID_TIMER;
	}

	// Init sale end
	sale_item->timer_end = add_timer( gettick() + (uint32)( sale_item->end - time(nullptr) ) * 1000, sale_end_timer, 0, (intptr_t)sale_item );

	return 1;
}

enum e_sale_add_result sale_add_item( t_itemid nameid, int32 count, time_t from, time_t to, time_t rent ){
	int32 id;
	char* data;
	int32 rent_time_temp = 0;
	
	// Check if the item exists in the sales tab
	if( cash_shop_db.findItemInTab( CASHSHOP_TAB_SALE, nameid ) == nullptr ){
		return SALE_ADD_FAILED;
	}

	// Adding a sale in the past is not possible
	if( from < time(nullptr) ){
		return SALE_ADD_FAILED;
	}

	// The end has to be after the start
	if( from >= to ){
		return SALE_ADD_FAILED;
	}

	// Amount has to be positive - this should be limited from the client too
	if( count == 0 ){
		return SALE_ADD_FAILED;
	}

	// Check if a sale of this item already exists
	if( sale_find_item(nameid, false) ){
		return SALE_ADD_DUPLICATE;
	}
	
	if( rent > 0 ){
		struct tm *ltm = gmtime(&rent);
		if(ltm->tm_mday >= 14 && ltm->tm_hour >= 0)
			rent_time_temp = 0;
		else
			rent_time_temp = (ltm->tm_mday * 24 * 60 * 60) + (ltm->tm_hour * 60 * 60) + (ltm->tm_min * 60);
	}
	
	if( SQL_ERROR == Sql_Query(mmysql_handle, "INSERT INTO `%s`(`nameid`,`start`,`end`,`amount`,`rentalTime`) VALUES ( '%u', FROM_UNIXTIME(%d), FROM_UNIXTIME(%d), '%d', '%d' )", sales_table, nameid, (uint32)from, (uint32)to, count, rent_time_temp) ){
		Sql_ShowDebug(mmysql_handle);
		return SALE_ADD_FAILED;
	}
	if( SQL_ERROR == Sql_Query(mmysql_handle, "SELECT `id` FROM `%s` WHERE `nameid` = '%u'", sales_table, nameid) ){
		Sql_ShowDebug(mmysql_handle);
		return SALE_ADD_FAILED;
	}
	while( SQL_SUCCESS == Sql_NextRow( mmysql_handle ) )
		Sql_GetData( mmysql_handle, 0, &data, nullptr ); id = atoi(data);

	Sql_FreeResult( mmysql_handle );

	RECREATE(sale_items.item, struct sale_item_data *, ++sale_items.count);
	CREATE(sale_items.item[sale_items.count - 1], struct sale_item_data, 1);
	struct sale_item_data* sale_item = sale_items.item[sale_items.count - 1];

	sale_item->id = id;
	sale_item->nameid = nameid;
	sale_item->start = from;
	sale_item->end = to;
	sale_item->amount = count;
	sale_item->timer_start = add_timer( gettick() + (uint32)(from - time(nullptr)) * 1000, sale_start_timer, 0, (intptr_t)sale_item );
	sale_item->timer_end = INVALID_TIMER;
	sale_item->rentalTime = static_cast<int32>(rent_time_temp);

	return SALE_ADD_SUCCESS;
}

bool sale_remove_item( t_itemid nameid ){
	struct sale_item_data* sale_item;
	int32 i;

	// Check if there is an entry for this item id
	sale_item = sale_find_item(nameid, false);

	if( sale_item == nullptr ){
		return false;
	}

	// Delete it from the database
	if( SQL_ERROR == Sql_Query(mmysql_handle, "DELETE FROM `sales_limited_acc` WHERE `sales_id` IN (SELECT `id` FROM `%s` WHERE `nameid` = '%u')", sales_table, nameid ) ){
		Sql_ShowDebug(mmysql_handle);
		return false;
	}
	if( SQL_ERROR == Sql_Query(mmysql_handle, "DELETE FROM `%s` WHERE `nameid` = '%u'", sales_table, nameid ) ){
		Sql_ShowDebug(mmysql_handle);
		return false;
	}

	if( sale_item->timer_start != INVALID_TIMER ){
		delete_timer(sale_item->timer_start, sale_start_timer);
		sale_item->timer_start = INVALID_TIMER;
	}

	// Check if the sale is currently running
	if( sale_item->timer_end != INVALID_TIMER ){
		delete_timer(sale_item->timer_end, sale_end_timer);
		sale_item->timer_end = INVALID_TIMER;

		// Notify all clients that the sale has ended
		map_foreachpc(clif_CashShopLimited_sub);
	}

	// Find the original pointer in the array
	ARR_FIND( 0, sale_items.count, i, sale_items.item[i] == sale_item );

	// Is there still any entry left?
	if( --sale_items.count > 0 ){
		// fill the hole by moving the rest
		for( ; i < sale_items.count; i++ ){
			memcpy( sale_items.item[i], sale_items.item[i + 1], sizeof(struct sale_item_data) );
		}

		aFree(sale_items.item[i]);

		RECREATE(sale_items.item, struct sale_item_data *, sale_items.count);
	}else{
		aFree(sale_items.item[0]);
		aFree(sale_items.item);
		sale_items.item = nullptr;
	}

	return true;
}

struct sale_item_data* sale_find_item( t_itemid nameid, bool onsale ){
	int32 i;
	struct sale_item_data* sale_item;
	time_t now = time(nullptr);

	ARR_FIND( 0, sale_items.count, i, sale_items.item[i]->nameid == nameid );

	// No item with the specified item id was found
	if( i == sale_items.count ){
		return nullptr;
	}

	sale_item = sale_items.item[i];

	// No need to check any further
	if( !onsale ){
		return sale_item;
	}

	// The sale is in the future
	if( sale_items.item[i]->start > now ){
		return nullptr;
	}

	// The sale was in the past
	if( sale_items.item[i]->end < now ){
		return nullptr;
	}

	// Return the sale item
	return sale_items.item[i];
}

void sale_notify_login( map_session_data* sd ){
	int32 i;

	for( i = 0; i < sale_items.count; i++ ){
		if( sale_items.item[i]->timer_end != INVALID_TIMER ){
			clif_CashShopLimited(sd);
		}
	}
}

void sale_load_pc( map_session_data* sd ){
	char* data;
	int32 id, amount;

	if( SQL_ERROR == Sql_Query(mmysql_handle, "SELECT `sales_id`, `amount` FROM `sales_limited_acc` WHERE `account_id` = '%d'", sd->status.account_id) ){
		Sql_ShowDebug(mmysql_handle);
		return;
	}
	while( SQL_SUCCESS == Sql_NextRow( mmysql_handle ) ){
		Sql_GetData( mmysql_handle, 0, &data, nullptr ); id = atoi(data);
		Sql_GetData( mmysql_handle, 1, &data, nullptr ); amount = atoi(data);
		sd->sales.push_back( std::make_pair(id,amount) );
	}
	Sql_FreeResult(mmysql_handle);
}
#endif

static void cashshop_read_db( void ){
	cash_shop_db.load();

#if PACKETVER_SUPPORTS_SALES
	int32 i;
	time_t now = time(nullptr);

	sale_read_db_sql();

	// Clean outdated sales
	if( SQL_ERROR == Sql_Query(mmysql_handle, "DELETE FROM `sales_limited_acc` WHERE `sales_id` IN (SELECT `id` FROM `%s` WHERE `end` < FROM_UNIXTIME(%d))", sales_table, (uint32)now ) ){
		Sql_ShowDebug(mmysql_handle);
	}
	if( SQL_ERROR == Sql_Query(mmysql_handle, "DELETE FROM `%s` WHERE `end` < FROM_UNIXTIME(%d)", sales_table, (uint32)now ) ){
		Sql_ShowDebug(mmysql_handle);
	}

	// Init next sale start, if there is any
	for( i = 0; i < sale_items.count; i++ ){
		struct sale_item_data* it = sale_items.item[i];

		if( it->start > now ){
			it->timer_start = add_timer( gettick() + (uint32)( it->start - time(nullptr) ) * 1000, sale_start_timer, 0, (intptr_t)it );
		}else{
			sale_start_timer( 0, gettick(), 0, (intptr_t)it );
		}
	}
#endif
}

/** Attempts to purchase a cashshop item from the list.
 * Checks if the transaction is valid and if the user has enough inventory space to receive the item.
 * If yes, take cashpoints and give items; else return clif_error.
 * @param sd Player that request to buy item(s)
 * @param kafrapoints
 * @param n Count of item list
 * @param item_list Array of item ID
 * @return true: success, false: fail
 */
bool cashshop_buylist( map_session_data* sd, uint32 kafrapoints, int32 n, const PACKET_CZ_SE_PC_BUY_CASHITEM_LIST_sub* item_list ){
	uint32 totalcash = 0;
	uint32 totalweight = 0;
	int32 i,new_;

	if( sd == nullptr || item_list == nullptr ){
		clif_cashshop_result( sd, 0, CASHSHOP_RESULT_ERROR_UNKNOWN );
		return false;
	}else if( sd->state.trading ){
		clif_cashshop_result( sd, 0, CASHSHOP_RESULT_ERROR_PC_STATE );
		return false;
	}

	new_ = 0;

	for( i = 0; i < n; ++i ){
		t_itemid nameid = item_list[i].itemId;
		uint32 quantity = item_list[i].amount;
		uint16 tab = item_list[i].tab;

		if( tab >= CASHSHOP_TAB_MAX ){
			clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_UNKNOWN );
			return false;
		}

		std::shared_ptr<s_cash_item> cash_item = cash_shop_db.findItemInTab( static_cast<e_cash_shop_tab>( tab ), nameid );

		if( cash_item == nullptr ){
			clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_UNKONWN_ITEM );
			return false;
		}


		std::shared_ptr<item_data> id = item_db.find(nameid);

		if( !id ){
			clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_UNKONWN_ITEM );
			return false;
		}

		if( quantity > 99 ){
			// Client blocks buying more than 99 items of the same type at the same time, this means someone forged a packet with a higher quantity
			clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_UNKNOWN );
			return false;
		}

		if( tab == CASHSHOP_TAB_SALE ){
#if PACKETVER_SUPPORTS_SALES
			struct sale_item_data* sale = sale_find_item( nameid, true );

			if( sale == nullptr ){
				// Client tried to buy an item from sale that was not even on sale
				clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_UNKNOWN );
				return false;
			}

			int32 temp_amount = sale->amount;
			for(auto &it : sd->sales){
				if(it.first == sale->id){
					temp_amount = it.second;
					break;
				}
			}

			if( temp_amount < quantity ){
				// Client tried to buy a higher quantity than is available for his account
				clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_UNKNOWN );
				// Maybe he did not get refreshed in time -> do it now
				clif_CashShopLimited(sd);
				return false;
			}
#else
			clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_UNKNOWN );
			return false;
#endif
		}

		switch( pc_checkadditem( sd, nameid, quantity ) ){
			case CHKADDITEM_EXIST:
				break;

			case CHKADDITEM_NEW:
				new_ += id->inventorySlotNeeded(quantity);
				break;

			case CHKADDITEM_OVERAMOUNT:
				clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_OVER_PRODUCT_TOTAL_CNT );
				return false;
		}

		totalcash += cash_item->price * quantity;
		totalweight += itemdb_weight( nameid ) * quantity;
	}

	if( ( totalweight + sd->weight ) > sd->max_weight ){
		clif_cashshop_result( sd, 0, CASHSHOP_RESULT_ERROR_INVENTORY_WEIGHT );
		return false;
	}else if( pc_inventoryblank( sd ) < new_ ){
		clif_cashshop_result( sd, 0, CASHSHOP_RESULT_ERROR_INVENTORY_ITEMCNT );
		return false;
	}

	if(pc_paycash( sd, totalcash, kafrapoints, LOG_TYPE_CASH ) <= 0){
		clif_cashshop_result( sd, 0, CASHSHOP_RESULT_ERROR_SHORTTAGE_CASH );
		return false;
	}

	for( i = 0; i < n; ++i ){
		t_itemid nameid = item_list[i].itemId;
		uint32 quantity = item_list[i].amount;
#if PACKETVER_SUPPORTS_SALES
		uint16 tab = item_list[i].tab;
#endif
		struct item_data *id = itemdb_search(nameid);

		if (!id)
			continue;

		unsigned short get_amt = quantity;

		if (id->flag.guid || !itemdb_isstackable2(id))
			get_amt = 1;

#if PACKETVER_SUPPORTS_SALES
		struct sale_item_data* sale = nullptr;

		if( tab == CASHSHOP_TAB_SALE ){
			sale = sale_find_item( nameid, true );

			if( sale == nullptr ){
				// Client tried to buy an item from sale that was not even on sale
				clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_UNKNOWN );
				return false;
			}

			int32 temp_amount = sale->amount;
			for(auto &it : sd->sales){
				if(it.first == sale->id){
					temp_amount = it.second;
					break;
				}
			}
			if( temp_amount < quantity ){
				// Client tried to buy a higher quantity than is available
				clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_UNKNOWN );
				// Maybe he did not get refreshed in time -> do it now
				clif_CashShopLimited(sd);
				return false;
			}
		}
#endif

		for (uint32 j = 0; j < quantity; j += get_amt) {
			if( !pet_create_egg( sd, nameid ) ){
				struct item item_tmp = { 0 };

				item_tmp.nameid = nameid;
				item_tmp.identify = 1;
				if(tab == CASHSHOP_TAB_SALE && sale->rentalTime > 0 && id->type != IT_HEALING && id->type != IT_CARD)
					item_tmp.expire_time = (unsigned int)(time(nullptr) + sale->rentalTime);

				switch( pc_additem( sd, &item_tmp, get_amt, LOG_TYPE_CASH ) ){
					case ADDITEM_OVERWEIGHT:
						clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_INVENTORY_WEIGHT );
						return false;
					case ADDITEM_OVERITEM:
						clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_INVENTORY_ITEMCNT );
						return false;
					case ADDITEM_OVERAMOUNT:
						clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_OVER_PRODUCT_TOTAL_CNT );
						return false;
					case ADDITEM_STACKLIMIT:
						clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_ERROR_RUNE_OVERCOUNT );
						return false;
				}
			}

			clif_cashshop_result( sd, nameid, CASHSHOP_RESULT_SUCCESS );

#if PACKETVER_SUPPORTS_SALES
			if( tab == CASHSHOP_TAB_SALE ){
				int32 new_amount = sale->amount-get_amt;
				new_amount = (new_amount == 0) ? -1 : new_amount;

				for(auto &it : sd->sales){
					if(it.first == sale->id){
						new_amount = it.second-get_amt;
						new_amount = (new_amount == 0) ? -1 : new_amount;
						it.second = new_amount;
						break;
					}
				}

				if( SQL_ERROR == Sql_Query( mmysql_handle, "INSERT INTO `sales_limited_acc` (`sales_id`,`account_id`,`amount`) VALUES ('%d', '%d', '%d') ON DUPLICATE KEY UPDATE amount = '%d'", sale->id, sd->status.account_id, new_amount, new_amount ) ){
					Sql_ShowDebug(mmysql_handle);
				}

				sd->sales.push_back( std::make_pair(sale->id,new_amount) );

				clif_CashShopLimited(sd);
			}
#endif
		}
	}

	return true;
}

/*
 * Reloads cashshop database by destroying it and reading it again.
 */
void cashshop_reloaddb( void ){
	do_final_cashshop();
	do_init_cashshop();
}

/*
 * Destroys cashshop class.
 * Closes all and cleanup.
 */
void do_final_cashshop( void ){
	cash_shop_db.clear();

#if PACKETVER_SUPPORTS_SALES
	if( sale_items.count > 0 ){
		for( int32 i = 0; i < sale_items.count; i++ ){
			struct sale_item_data* it = sale_items.item[i];

			if( it->timer_start != INVALID_TIMER ){
				delete_timer( it->timer_start, sale_start_timer );
				it->timer_start = INVALID_TIMER;
			}

			if( it->timer_end != INVALID_TIMER ){
				delete_timer( it->timer_end, sale_end_timer );
				it->timer_end = INVALID_TIMER;
			}

			aFree(it);
		}

		aFree(sale_items.item);

		sale_items.item = nullptr;
		sale_items.count = 0;
	}
#endif
}

/*
 * Initializes cashshop class.
 */
void do_init_cashshop( void ){
	cashshop_read_db();
}
