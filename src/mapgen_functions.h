#pragma once

#include <functional>
#include <map>
#include <string>
#include <utility>

#include "coordinates.h"
#include "type_id.h"

class map;
class mapgendata;
class mission;
struct mapgen_parameters;
struct point;
struct tripoint;

using mapgen_id = std::string;
using mapgen_update_func = std::function<void( const tripoint_abs_omt &map_pos3, mission *miss )>;
class JsonObject;

/**
 * Calculates the coordinates of a rotated point.
 * Should match the `mapgen_*` rotation.
 */
tripoint rotate_point( const tripoint &p, int rotations );

int terrain_type_to_nesw_array( oter_id terrain_type, bool array[4] );

using building_gen_pointer = void ( * )( mapgendata & );
building_gen_pointer get_mapgen_cfunction( const std::string &ident );
ter_id grass_or_dirt();
ter_id clay_or_sand();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null( mapgendata &dat );
void mapgen_test( mapgendata &dat );
void mapgen_crater( mapgendata &dat );
void mapgen_field( mapgendata &dat );
void mapgen_forest( mapgendata &dat );
void mapgen_forest_trail_straight( mapgendata &dat );
void mapgen_forest_trail_curved( mapgendata &dat );
void mapgen_forest_trail_tee( mapgendata &dat );
void mapgen_forest_trail_four_way( mapgendata &dat );
void mapgen_hive( mapgendata &dat );
void mapgen_river_center( mapgendata &dat );
void mapgen_road( mapgendata &dat );
//void mapgen_bridge( mapgendata &dat );
void mapgen_railroad( mapgendata &dat );
void mapgen_railroad_bridge( mapgendata &dat );
void mapgen_highway( mapgendata &dat );
void mapgen_river_curved_not( mapgendata &dat );
void mapgen_river_straight( mapgendata &dat );
void mapgen_river_curved( mapgendata &dat );
void mapgen_river_shore( mapgendata &dat );
void mapgen_parking_lot( mapgendata &dat );
void mapgen_cave( mapgendata &dat );
void mapgen_cave_rat( mapgendata &dat );
void mapgen_cavern( mapgendata &dat );
void mapgen_rock( mapgendata &dat );
void mapgen_rock_partial( mapgendata &dat );
void mapgen_open_air( mapgendata &dat );
void mapgen_rift( mapgendata &dat );
void mapgen_hellmouth( mapgendata &dat );
void mapgen_subway( mapgendata &dat );
void mapgen_sewer( mapgendata &dat );
void mapgen_tutorial( mapgendata &dat );
void mapgen_lake_shore( mapgendata &dat );

// Temporary wrappers
void mremove_trap( map *m, point );
void mtrap_set( map *m, point, trap_id type );
void madd_field( map *m, point, field_type_id type, int intensity );

mapgen_update_func add_mapgen_update_func( const JsonObject &jo, bool &defer );
bool run_mapgen_update_func( const std::string &update_mapgen_id, const tripoint_abs_omt &omt_pos,
                             mission *miss = nullptr, bool cancel_on_collision = true );
bool run_mapgen_update_func( const std::string &update_mapgen_id, mapgendata &dat,
                             bool cancel_on_collision = true );
bool run_mapgen_func( const std::string &mapgen_id, mapgendata &dat );
std::pair<std::map<ter_id, int>, std::map<furn_id, int>> get_changed_ids_from_update(
            const std::string &update_mapgen_id );
mapgen_parameters get_map_special_params( const std::string &mapgen_id );

void resolve_regional_terrain_and_furniture( const mapgendata &dat );

namespace mapgen
{

bool has_update_id( const mapgen_id &id );

} // namespace mapgen


