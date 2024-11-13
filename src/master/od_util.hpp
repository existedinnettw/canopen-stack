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

void
verify_slave_pdo_configs(const Slave_model_configs& configs);

// malloc OD memory based on slave PDO configs

std::vector<CO_OBJ_T>
create_default_od();

void
config_od_through_configs(std::vector<CO_OBJ_T>& od, const Slave_model_configs& configs);