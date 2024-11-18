// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef NAVI_H
#define NAVI_H

#ifdef MAP_GENERATOR

#include <config/core.hpp>

struct navi_pos {
	int32 m;
	int32 x;
	int32 y;
};

struct npc_data;

struct navi_npc {
	struct npc_data * npc;
	int32 id;
	struct navi_pos pos;

};

struct navi_link {
	struct npc_data * npc;
	int32 id;
	struct navi_pos pos;
	struct navi_pos warp_dest; // only set for warps
	bool hidden; // hidden by script
	std::string name; // custom name
};

struct navi_walkpath_data {
	uint8 path_len, path_pos;
	uint8 path[MAX_WALKPATH_NAVI];
};

void navi_create_lists();
#endif // ifdef MAP_GENERATOR
#endif
