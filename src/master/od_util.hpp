#pragma once

#include "co_core.h"
#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

/**
 * e.g.
 * {
    {
      121,
      {
        { 0x1600, { { std::make_tuple(0x604000, 2), std::make_tuple(0x607A00, 4) } } },
        { 0x1A00, { { std::make_tuple(0x604100, 2) } } },
      },
    },
  };
 *
 * -----------------------------------------------------------------------------------------
 * | node_id | pdo_idx | {(slave_od_idx, size_in_byte), (slave_od_idx, size_in_byte), ...} |
 */
typedef std::map<uint8_t, std::map<uint32_t, std::vector<std::tuple<uint32_t, uint8_t>>>> Slave_model_configs;
typedef std::map<uint32_t, std::vector<std::tuple<uint32_t, uint8_t>>> Slave_model_config;

/**
 * uint16_t* data_ptr = map[0x2,0x604000];
 * (node_id, pdo_idx)--> data_ptr
 */
typedef std::map<std::tuple<uint8_t, uint32_t>, void*> PDO_data_map;

/**
 * @brief
 * intermediate pdo data map
 * @details
 * (node_id, pdo_idx)-->(local_OD_index)
 * (2, 0x6041) --> (0x200203)
 */
typedef std::map<std::tuple<uint8_t, uint32_t>, uint32_t> Imd_PDO_data_map;

void
verify_slave_pdo_configs(const Slave_model_configs& configs);

// malloc OD memory based on slave PDO configs

/**
 * @attention
 * append `CO_OBJ_DICT_ENDMARK` to vector to indicate the end of OD
 */
std::vector<CO_OBJ>
create_default_od();

Imd_PDO_data_map
config_od_through_configs(std::vector<CO_OBJ>& od, const Slave_model_configs& configs);

/**
 * @brief
 * get `PDO_data_map`
 * @details
 * input od must be sorted
 */
PDO_data_map
get_PDO_data_map(std::vector<CO_OBJ>& od, const Imd_PDO_data_map& imd_pdo_data_map);


void
print_od(CO_OBJ_T* OD, uint16_t after_p_idx = 0x0);
void
print_data_map(const PDO_data_map& pdo_data_map);