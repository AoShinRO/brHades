// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "path.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iterator>

#include <common/cbasetypes.hpp>
#include <common/db.hpp>
#include <common/malloc.hpp>
#include <common/nullpo.hpp>
#include <common/random.hpp>
#include <common/showmsg.hpp>

#include "battle.hpp"
#include "map.hpp"

/// Binary heap of path nodes
/// c++ A*

class BinaryHeap {
private:
	std::vector<path_node*> open_set;
	
	// Returns the capacity of the open_set
	size_t capacity() const {
		return open_set.capacity();
	}
	std::function<int(const path_node*, const path_node*)> node_min_to_comp = [](const path_node* i, const path_node* j) {
		return i->f_cost - j->f_cost;
	};

	// Helper function to sift down
	void siftDown(size_t startIdx, size_t idx) {
		while (idx > startIdx) {
			size_t parent = (idx - 1) / 2;
			if (node_min_to_comp(open_set[parent], open_set[idx]) <= 0) break;
			std::swap(open_set[parent], open_set[idx]);
			idx = parent;
		}
	}

	// Helper function to sift up
	void siftUp(size_t idx) {
		size_t leftChild = idx * 2 + 1;
		while (leftChild < open_set.size()) {
			size_t rightChild = idx * 2 + 2;
			size_t swapChild = (rightChild < open_set.size() && node_min_to_comp(open_set[leftChild], open_set[rightChild]) > 0) ? rightChild : leftChild;
			if (node_min_to_comp(open_set[idx], open_set[swapChild]) <= 0) break;
			std::swap(open_set[idx], open_set[swapChild]);
			idx = swapChild;
			leftChild = idx * 2 + 1;
		}
	}

public:
	int calc_index(uint16 x, uint16 y){
		return static_cast<int>(((x)+(y)*(MAX_WALKPATH)) & (MAX_WALKPATH*MAX_WALKPATH-1));
	}

	// Returns the length of the open_set
	size_t length() const {
		return open_set.size();
	}

	void ensure(size_t n) {
		if (capacity() < n) {
			open_set.reserve(n); // Reservar espaço adicional, como especificado por 'step'
		}
	}

	// Returns the top value of the open_set
	path_node* peek() const {
		return open_set.front();
	}
	
	// Inserts a value in the open_set and restores the open_set property
	void push(path_node& val) {
		open_set.push_back(&val);
		size_t i = open_set.size() - 1;
		siftDown(0, i);
	}
	
	// Removes the top value of the open_set and restores the open_set property
	void pop() {
		open_set.front() = open_set.back();
		open_set.pop_back();
		if (open_set.empty()) return;  // Se não restar nenhum elemento, não há mais nada a fazer
		siftUp(0);
	}
	
	// Updates the open_set after modifying an element
	void update(size_t idx) {
		siftDown(0, idx);
		siftUp(idx);
	}
	
	// Clears the open_set
	void clear() {
		if(open_set.empty()) return; // Se não tiver nenhum elemento, não há nada a fazer
		open_set.clear();
	}

	/// Pushes path_node to the binary node_open_set.
	/// Ensures there is enough space in array to store new element.
	void push_node(path_node* node) {
		ensure(open_set.size() + 1);
		push(*node); 
	}

	/// Updates path_node in the binary node_open_set.
	int update_node(path_node& node)
	{
		auto it = std::find(open_set.begin(), open_set.end(), &node);
		if (it == open_set.end()) {
			return 1;
		}
		size_t i = std::distance(open_set.begin(), it);
		update(i);
		return 0;
	}
};

/// @}

/// @name Distance related functions
/// @{

/// @param dx: Horizontal distance
/// @param dy: Vertical distance
/// @return Manhattan distance -> Radius.DIAMOND
static inline unsigned char manhattan_distance(char dx, char dy) {
	return static_cast<unsigned char>(std::abs(dx) + std::abs(dy));
}

/*
 * 123/128 is a good approximation for 0.96. The formula uses a combination of left shifts (<<) and subtractions to approximate this multiplication.
 * 51/128 is a good approximation for 0.4. The formula uses a similar process to approximate this multiplication using shifts and subtractions.
 * Chebyshev distance taken from http://web.archive.org/web/20071003001801/http://www.flipcode.com/articles/article_fastdistance.shtml
 */

// @param dx: Horizontal distance
// @param dy: Vertical distance
// @return Chebyshev range -> Radius.SQUARE
static inline unsigned char chebyshev_range(char dx, char dy) {
	return static_cast<unsigned char>(std::max(std::abs(dx), std::abs(dy)));
}

// @param dx: Horizontal distance
// @param dy: Vertical distance
// @return Chebyshev distance -> Radius.SQUARE
static inline unsigned char chebyshev_distance(char dx, char dy) {
	return static_cast<unsigned char>((123.0 / 128.0 * chebyshev_range(dx, dy)) + (51.0 / 128.0 * std::min(std::abs(dx), std::abs(dy))));
}

/*
 * Euclidean distance taken from https://cplusplus.com/forum/beginner/178293/
*/

// @param dx: Horizontal distance
// @param dy: Vertical distance
// @return Euclidean range -> Radius.CIRCLE 
static inline double euclidean_range(char dx, char dy) {
	return std::pow(std::abs(dx), 2) + std::pow(std::abs(dy), 2);
}

// @param dx: Horizontal distance
// @param dy: Vertical distance
// @return Euclidean distance -> Radius.CIRCLE
static inline double euclidean_distance(char dx, char dy) {
	return std::sqrt(euclidean_range(dx,dy));
}

/// @}

void do_init_path(){
}//

void do_final_path(){
}//


/*==========================================
 * Find the closest reachable cell, 'count' cells away from (x0,y0) in direction (dx,dy).
 * Income after the coordinates of the blow
 *------------------------------------------*/
int path_blownpos(int16 m,int16 x0,int16 y0,int16 dx,int16 dy,int count)
{
	struct map_data *mapdata = map_getmapdata(m);

	if( !mapdata->cell )
		return -1;

	if( count>25 ){ //Cap to prevent too much processing...?
		ShowWarning("path_blownpos: count too many %d !\n",count);
		count=25;
	}
	if( dx > 1 || dx < -1 || dy > 1 || dy < -1 ){
		ShowError("path_blownpos: illegal dx=%d or dy=%d !\n",dx,dy);
		dx=(dx>0)?1:((dx<0)?-1:0);
		dy=(dy>0)?1:((dy<0)?-1:0);
	}

	while( count > 0 && (dx != 0 || dy != 0) )
	{
		if( !map_getcellp(mapdata,x0+dx,y0+dy,CELL_CHKPASS) )
		{
			if (battle_config.path_blown_halt)
				break;
			else
			{// attempt partial movement
				int fx = ( dx != 0 && map_getcellp(mapdata,x0+dx,y0,CELL_CHKPASS) );
				int fy = ( dy != 0 && map_getcellp(mapdata,x0,y0+dy,CELL_CHKPASS) );
				if( fx && fy )
				{
					if(rnd_chance(50, 100))
						dx=0;
					else
						dy=0;
				}
				if( !fx )
					dx=0;
				if( !fy )
					dy=0;
			}
		}

		x0 += dx;
		y0 += dy;
		count--;
	}

	return (x0<<16)|y0; //TODO: use 'struct point' here instead?
}

/*==========================================
 * is ranged attack from (x0,y0) to (x1,y1) possible?
 *------------------------------------------*/
bool path_search_long(struct shootpath_data *spd,int16 m,int16 x0,int16 y0,int16 x1,int16 y1,cell_chk cell)
{
	int dx, dy;
	int wx = 0, wy = 0;
	int weight;
	struct map_data *mapdata = map_getmapdata(m);
	struct shootpath_data s_spd;

	if( spd == nullptr )
		spd = &s_spd; // use dummy output variable

	if (!mapdata->cell)
		return false;

	dx = (x1 - x0);
	if (dx < 0) {
		std::swap(x0, x1);
		std::swap(y0, y1);
		dx = -dx;
	}
	dy = (y1 - y0);

	spd->rx = spd->ry = 0;
	spd->len = 1;
	spd->x[0] = x0;
	spd->y[0] = y0;

	if (dx > abs(dy)) {
		weight = dx;
		spd->ry = 1;
	} else {
		weight = abs(y1 - y0);
		spd->rx = 1;
	}

	while (x0 != x1 || y0 != y1)
	{
		wx += dx;
		wy += dy;
		if (wx >= weight) {
			wx -= weight;
			x0++;
		}
		if (wy >= weight) {
			wy -= weight;
			y0++;
		} else if (wy < 0) {
			wy += weight;
			y0--;
		}
		if( spd->len<MAX_WALKPATH )
		{
			spd->x[spd->len] = x0;
			spd->y[spd->len] = y0;
			spd->len++;
		}
		if ((x0 != x1 || y0 != y1) && map_getcellp(mapdata,x0,y0,cell))
			return false;
	}

	return true;
}

/// @name A* pathfinding related functions
/// @{

// Estimates the cost from (x0,y0) to (x1,y1).
// The walkpath uses a Diamond distance instead of the square one.
// @param x0: Cell X
// @param y0: Cell y
// @param x1: Target cell X
// @param y1: Target cell y
// @return movecost X manhattan distance (This is inadmissible (overestimating) heuristic used by game client)
static inline unsigned short heuristic(uint16 x0,uint16 y0,uint16 x1,uint16 y1) {
	return MOVE_COST * manhattan_distance((x1)-(x0), (y1)-(y0));
}

/// Path_node processing in A* pathfinding.
/// Adds new node to heap and updates/re-adds old ones if necessary.
static unsigned char add_path(BinaryHeap& heap, std::vector<path_node>& tp, uint16 x, uint16 y, unsigned short g_cost, path_node& parent, unsigned short h_cost)
{
	int i = heap.calc_index(x, y);

	if (tp[i].x == x && tp[i].y == y) { // We processed this node before
		if (g_cost < tp[i].g_cost) { // New path to this node is better than old one
			// Update costs and parent
			tp[i].g_cost = g_cost;
			tp[i].parent = &parent;
			tp[i].f_cost = g_cost + h_cost;
			if (tp[i].flag == SET_CLOSED) {
				heap.push_node(&tp[i]); // Put it in open set again
			}
			else if (heap.update_node(tp[i])) {
				ShowError("heap_update_node: node not found\n");
				return 1;
			}
			tp[i].flag = SET_OPEN;
		}
		return 0;
	}

	if (tp[i].x || tp[i].y){ // Index is already taken; see `tp` array FIXME for details
		ShowDebug("heap_update_node:Index is already taken \n"); // isso parou de aparecer quando comecei a usar um binaryheap por walkpath, acho  que as colisoes aconteciam por isso
		return 1;
	}

	// New node
	tp[i].x = x;
	tp[i].y = y;
	tp[i].g_cost = g_cost;
	tp[i].parent = &parent;
	tp[i].f_cost = g_cost + h_cost;
	tp[i].flag = SET_OPEN;
	heap.push_node(&tp[i]);
	return 0;
}
///@}

/// @name A* pathfinding
/// @{

// A* (A-star) pathfinding
// We always use A* for finding walkpaths because it is what game client uses.
// Easy pathfinding cuts corners of non-walkable cells, but client always walks around it.
static bool aegis_pathfinding(walkpath_data& wpd, int16 m, uint16 x0, uint16 y0, uint16 x1, uint16 y1, cell_chk cell){

	unsigned short i;
	uint16 x, y;
	char dx = 0, dy = 0;
	struct map_data *mapdata = map_getmapdata(m);

	struct path_node* current, *it;
	int16 xs = mapdata->xs - 1;
	int16 ys = mapdata->ys - 1;
	unsigned char len = 0;
	char j; // need to be signed
	unsigned char e = 0; // error flag
	unsigned short g_cost;

	// Saves allowed directions for the current cell. Diagonal directions
	// are only allowed if both directions around it are allowed. This is
	// to prevent cutting corner of nearby wall.
	// For example, you can only go NW from the current cell, if you can
	// go N *and* you can go W. Otherwise you need to walk around the
	// (corner of the) non-walkable cell.
	char allowed_dirs;

	// FIXME: This array is too small to ensure all paths shorter than MAX_WALKPATH
	// can be found without node collision: calc_index(node1) = calc_index(node2).
	// Figure out more proper size or another way to keep track of known nodes.
	// (fixed dont using same binaryheap? maybe we never know) [AoShinHo]
	std::vector<path_node> tp(static_cast<int>(std::pow(MAX_WALKPATH,2)));

	static BinaryHeap heap;

	// Start node
	i = heap.calc_index(x0, y0);
	tp[i].parent = nullptr;
	tp[i].x      = x0;
	tp[i].y      = y0;
	tp[i].g_cost = 0;
	tp[i].f_cost = heuristic(x0, y0, x1, y1);
	tp[i].flag   = SET_OPEN;

	heap.push_node(&tp[i]); // Put start node to 'open' set

	for(;;) {

		allowed_dirs = 0;

		if (heap.length() == 0) {
			return false;
		}

		current = heap.peek(); // Look for the lowest f_cost node in the 'open' set
		heap.pop(); // Remove it from 'open' set

		x      = current->x;
		y      = current->y;
		g_cost = current->g_cost;

		current->flag = SET_CLOSED; // Add current node to 'closed' set

		if (x == x1 && y == y1) {
			break;
		}

		if (y < ys && !map_getcellp(mapdata, x, y+1, cell)) allowed_dirs |= PATH_DIR_NORTH;
		if (y >  0 && !map_getcellp(mapdata, x, y-1, cell)) allowed_dirs |= PATH_DIR_SOUTH;
		if (x < xs && !map_getcellp(mapdata, x+1, y, cell)) allowed_dirs |= PATH_DIR_EAST;
		if (x >  0 && !map_getcellp(mapdata, x-1, y, cell)) allowed_dirs |= PATH_DIR_WEST;

#define chk_dir(d) ((allowed_dirs & (d)) == (d))
		// Process neighbors of current node
		if (chk_dir(PATH_DIR_SOUTH|PATH_DIR_EAST) && !map_getcellp(mapdata, x+1, y-1, cell))
			e += add_path(heap, tp, x+1, y-1, g_cost + MOVE_DIAGONAL_COST, *current, heuristic(x+1, y-1, x1, y1)); // (x+1, y-1) 5
		if (chk_dir(PATH_DIR_EAST))
			e += add_path(heap, tp, x+1, y, g_cost + MOVE_COST, *current, heuristic(x+1, y, x1, y1)); // (x+1, y) 6
		if (chk_dir(PATH_DIR_NORTH|PATH_DIR_EAST) && !map_getcellp(mapdata, x+1, y+1, cell))
			e += add_path(heap, tp, x+1, y+1, g_cost + MOVE_DIAGONAL_COST, *current, heuristic(x+1, y+1, x1, y1)); // (x+1, y+1) 7
		if (chk_dir(PATH_DIR_NORTH))
			e += add_path(heap, tp, x, y+1, g_cost + MOVE_COST, *current, heuristic(x, y+1, x1, y1)); // (x, y+1) 0
		if (chk_dir(PATH_DIR_NORTH|PATH_DIR_WEST) && !map_getcellp(mapdata, x-1, y+1, cell))
			e += add_path(heap, tp, x-1, y+1, g_cost + MOVE_DIAGONAL_COST, *current, heuristic(x-1, y+1, x1, y1)); // (x-1, y+1) 1
		if (chk_dir(PATH_DIR_WEST))
			e += add_path(heap, tp, x-1, y, g_cost + MOVE_COST, *current, heuristic(x-1, y, x1, y1)); // (x-1, y) 2
		if (chk_dir(PATH_DIR_SOUTH|PATH_DIR_WEST) && !map_getcellp(mapdata, x-1, y-1, cell))
			e += add_path(heap, tp, x-1, y-1, g_cost + MOVE_DIAGONAL_COST, *current, heuristic(x-1, y-1, x1, y1)); // (x-1, y-1) 3
		if (chk_dir(PATH_DIR_SOUTH))
			e += add_path(heap, tp, x, y-1, g_cost + MOVE_COST, *current, heuristic(x, y-1, x1, y1)); // (x, y-1) 4
#undef chk_dir
		if (e) {
			return false;
		}
	}

	for (it = current; it->parent != nullptr; it = it->parent, len++);

	if (len > sizeof(wpd.path))
		return false;

	// Recreate path
	wpd.path_len = len;
	wpd.path_pos = 0;

	it = current;
	j = len;

	if(it->parent == nullptr) // moving 0 range?
		return false;

	while (j-- >= 0 && it->parent != nullptr){ 
	    dx = it->x - it->parent->x;
	    dy = it->y - it->parent->y;
	    wpd.path[j] = walk_choices[-dy + 1][dx + 1];
	    it = it->parent;
	};

	return true;
}
///@} A* end

/// @name easy path search
/// @{
static bool easy_pathfind(walkpath_data& wpd, int16 m, uint16 x0, uint16 y0, uint16 x1, uint16 y1, cell_chk cell){

	struct map_data *mapdata = map_getmapdata(m);
	unsigned char i;
	uint16 x, y;
	char dx = 0, dy = 0;

	// Try finding direct path to target
	// Direct path goes diagonally first, then in straight line.

	// calculate (sgn(x1-x0), sgn(y1-y0))
	dx = ((dx = x1-x0)) ? ((dx<0) ? -1 : 1) : 0;
	dy = ((dy = y1-y0)) ? ((dy<0) ? -1 : 1) : 0;

	x = x0; // Current position = starting cell
	y = y0;
	i = 0;
	while( i < ARRAYLENGTH(wpd.path) ) {
		wpd.path[i] = walk_choices[-dy + 1][dx + 1];
		i++;

		x += dx; // Advance current position
		y += dy;

		if( x == x1 ) dx = 0; // destination x reached, no longer move along x-axis
		if( y == y1 ) dy = 0; // destination y reached, no longer move along y-axis

		if( dx == 0 && dy == 0 )
			break; // success
		if( map_getcellp(mapdata,x,y,cell) )
			break; // obstacle = failure
	}

	if( x == x1 && y == y1 ) { // easy path successful.
		wpd.path_len = i;
		wpd.path_pos = 0;
		return true;
	}

	return false; // easy path unsuccessful
}
///@} easy path search end

/*==========================================
 * path search (x0,y0)->(x1,y1)
 * wpd: path info will be written here
 * flag: &1 = easy path search only
 * flag: &2 = call path_search_long instead
 * cell: type of obstruction to check for
 *------------------------------------------*/
bool path_search(struct walkpath_data *wpd, int16 m, uint16 x0, uint16 y0, uint16 x1, uint16 y1, int flag, cell_chk cell) {

	struct map_data *mapdata = map_getmapdata(m);
	struct walkpath_data s_wpd;

	if (flag&2)
		return path_search_long(nullptr, m, x0, y0, x1, y1, cell);

	if (wpd == nullptr)
		wpd = &s_wpd; // use dummy output variable

	if (!mapdata->cell)
		return false;

	//Do not check starting cell as that would get you stuck.
	if (x0 < 0 || x0 >= mapdata->xs || y0 < 0 || y0 >= mapdata->ys /*|| map_getcellp(mapdata,x0,y0,cell)*/)
		return false;

	// Check destination cell
	if (x1 < 0 || x1 >= mapdata->xs || y1 < 0 || y1 >= mapdata->ys || map_getcellp(mapdata,x1,y1,cell))
		return false;

	if (flag&1) 
		return easy_pathfind(*wpd, m, x0, y0, x1, y1, cell);
	else // !(flag&1)
		return aegis_pathfinding(*wpd, m, x0, y0, x1, y1, cell);

}

bool check_distance(char dx, char dy, unsigned char distance)
{
#ifdef CIRCULAR_AREA
	//In this case, we just do a square comparison. Add 1 tile grace for diagonal range checks.
	return (euclidean_range(dx, dy) <= std::pow(distance, 2) + (dx && dy ? 1.0 : 0.0));
#else
	return (chebyshev_range(dx, dy) <= distance);
#endif
}

unsigned char distance(char dx, char dy)
{
#ifdef CIRCULAR_AREA
	return std::max(chebyshev_distance(dx, dy), (unsigned char)1);
#else
	return chebyshev_range(dx, dy);
#endif
}

/**
 * The client uses a circular distance instead of the square one. The circular distance
 * is only used by units sending their attack commands via the client (not monsters).
 * @param dx: Horizontal distance
 * @param dy: Vertical distance
 * @param distance: Distance to check against
 * @return Within distance(1); Not within distance(0);
 */
bool check_distance_client(char dx, char dy, unsigned char distance){
	return (distance_client(dx,dy) <= distance);
}

/**
 * The client uses a circular distance instead of the square one. The circular distance
 * is only used by units sending their attack commands via the client (not monsters).
 * @param dx: Horizontal distance
 * @param dy: Vertical distance
 * @return Circular distance
 */
unsigned char distance_client(char dx, char dy) {
	// -0.1 factor used by client
	//This affects even horizontal/vertical lines so they are one cell longer than expected
	return static_cast<unsigned char>(std::max(euclidean_distance(dx,dy) - 0.1 ,0.0));
}

bool direction_diagonal( enum directions direction ){
	return direction == DIR_NORTHWEST || direction == DIR_SOUTHWEST || direction == DIR_SOUTHEAST || direction == DIR_NORTHEAST;
}

bool direction_opposite( enum directions direction ){
	if( direction == DIR_CENTER || direction == DIR_MAX ){
		return direction;
	}else{
		return static_cast<enum directions>( ( direction + DIR_MAX / 2 ) % DIR_MAX );
	}
}
