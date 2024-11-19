#include "od_util.hpp"
#include "co_size_only.h"
#include <algorithm> //std::find
#include <cassert>
#include <iostream>

/**
 */
std::vector<CO_OBJ>
create_default_od()
{
  std::vector<CO_OBJ> ret_od;
  ret_od = {
    { CO_KEY(0x1000, 0, CO_OBJ_D___R_),
      CO_TUNSIGNED32,
      (CO_DATA)(0x00000000) },                                            // device type, 0-->not standard profile
    { CO_KEY(0x1001, 0, CO_OBJ_D___R_), CO_TUNSIGNED32, (CO_DATA)(0x0) }, // error reg
    { CO_KEY(0x1005, 0, CO_OBJ_D___RW), CO_TSYNC_ID, (CO_DATA)(0x80) },   // sync producer enable
    { CO_KEY(0x1006, 0, CO_OBJ_D___RW), CO_TSYNC_CYCLE, (CO_DATA)(100 * 1000) }, // sync cycle time in us
    { CO_KEY(0x1014, 0, CO_OBJ_D___R_), CO_TEMCY_ID, (CO_DATA)(0x00000080L) },
    { CO_KEY(0x1017, 0, CO_OBJ_D___RW), CO_THB_PROD, (CO_DATA)(0) }, // heart beat
    // Identity Object
    { CO_KEY(0x1018, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA)(4) },
    { CO_KEY(0x1018, 1, CO_OBJ_D___R_), CO_TUNSIGNED32, (CO_DATA)(0) },
    { CO_KEY(0x1018, 2, CO_OBJ_D___R_), CO_TUNSIGNED32, (CO_DATA)(0) },
    { CO_KEY(0x1018, 3, CO_OBJ_D___R_), CO_TUNSIGNED32, (CO_DATA)(0) },
    { CO_KEY(0x1018, 4, CO_OBJ_D___R_), CO_TUNSIGNED32, (CO_DATA)(0) },
    // SDO server parameter
    { CO_KEY(0x1200, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA)(2) },
    { CO_KEY(0x1200, 1, CO_OBJ_DN__R_), CO_TUNSIGNED32, (CO_DATA)(CO_COBID_SDO_REQUEST()) },
    { CO_KEY(0x1200, 2, CO_OBJ_DN__R_), CO_TUNSIGNED32, (CO_DATA)(CO_COBID_SDO_RESPONSE()) },
    // SDO client parameter, create based on slave num later
    // rxpdo parameter, later
    // self rxpdo, later
    // txpdo parameter, later
    // self txpdo, later
    // user entries, later
  };

  // std::sort(ret_od.begin(), ret_od.end(), [](CO_OBJ a, CO_OBJ b) { return a.Key < b.Key; });

  return ret_od;
}

/**
 * @todo delete CO_HBCONS memory
 * @todo CO_TPDO_NUM for 0x160000 or 0x1A0000
 * @todo CO_TPDO_MAP for 0x160001 or 0x1A0001
 * @todo CO_TPDO_TYPE for 0x140002 or 0x180002
 * @todo CO_RPDO_EVENT implement, CO_TPDO_EVENT isn't work with RPDO
 * @see https://canopen-stack.org/v4.4/usage/configuration/?query=deadline#transmit-pdo-communication
 */
Imd_PDO_data_map
config_od_through_configs(std::vector<CO_OBJ>& od, const Slave_model_configs& configs)
{
  Imd_PDO_data_map ret_imd_pdo_data_map;

  /**
   * 0x1016 heartbeat consumer
   * @see void test_init_multiple(void), TS_HBCons_MultiConsumer
   */
  od.push_back({ CO_KEY(0x1016, 0, CO_OBJ_D___RW), CO_THB_CONS, (CO_DATA)(configs.size()) });
  for (auto it = configs.begin(); it != configs.end(); ++it) {
    size_t nth_node = std::distance(configs.begin(), it) + 1;
    uint8_t nodeId = it->first;
    // not support direct memory
    CO_HBCONS* hbc_data_ptr = new CO_HBCONS{
      .Time = 50, // 50ms
      .NodeId = nodeId,
    };
    // create hb consumer for each slave,
    // Monitoring of the heartbeat producer shall start after the reception of the first heartbeat.
    // Therefore non trigger is acceptable
    od.push_back({ CO_KEY(0x1016, nth_node, CO_OBJ_____RW),
                   CO_THB_CONS,
                   (CO_DATA)(hbc_data_ptr) }); // 0ms --> not enable yet, 50ms is default hb time
  }

  // SDO client parameter
  for (auto it = configs.begin(); it != configs.end(); ++it) {
    size_t nth_node = std::distance(configs.begin(), it);
    int nodeId = it->first;

    od.push_back({ CO_KEY(0x1280 + nth_node, 0, CO_OBJ_D___R_), CO_TUNSIGNED8, (CO_DATA)(3) });
    // client -> server (tx)
    od.push_back({ CO_KEY(0x1280 + nth_node, 1, CO_OBJ_D___RW), CO_TSDO_ID, (CO_DATA)(0x600) }); // request cobid
    // rx
    od.push_back({ CO_KEY(0x1280 + nth_node, 2, CO_OBJ_D___RW), CO_TSDO_ID, (CO_DATA)(0x580) }); // response cobid
    od.push_back({ CO_KEY(0x1280 + nth_node, 3, CO_OBJ_D___RW), CO_TUNSIGNED8, (CO_DATA)(nodeId) });
  }

  size_t nth_rpdo_map = 0;
  size_t nth_tpdo_map = 0;
  for (auto node_it = configs.begin(); node_it != configs.end(); ++node_it) {
    size_t nth_node = std::distance(configs.begin(), node_it);
    int nodeId = node_it->first;
    uint8_t nth_mapped_memory = 1;

    for (auto pdo_it = node_it->second.begin(); pdo_it != node_it->second.end(); ++pdo_it) {
      // need to map based on iterate
      uint16_t pdoId = pdo_it->first;
      printf("[DEBUG] pdo_id: %x, ", pdoId);
      assert((pdoId >= 0x1600 && pdoId <= 0x17FF) || (pdoId >= 0x1A00 && pdoId <= 0x1BFF));

      if (pdoId >= 0x1A00) {
        // txpdo of slave --> rxpdo of master
        printf("is_rpdo");
        if (pdo_it->second.size() == 0) {
          break;
        }

        // RPDO communication parameter
        od.push_back({ CO_KEY(0x1400 + nth_rpdo_map, 0, CO_OBJ_D___R_),
                       CO_TUNSIGNED8,
                       (CO_DATA)(02) }); // highest sub-index supported
        od.push_back({ CO_KEY(0x1400 + nth_rpdo_map, 1, CO_OBJ_D___RW),
                       CO_TPDO_ID,
                       (CO_DATA)(CO_COBID_RPDO_STD(1u, CO_COBID_TPDO_BASE + ((pdoId - 0x1A00) * CO_COBID_RPDO_INC)) +
                                 nodeId) }); // COB-ID used by RPDO
        od.push_back({ CO_KEY(0x1400 + nth_rpdo_map, 2, CO_OBJ_D___RW),
                       CO_TUNSIGNED8,
                       (CO_DATA)(0x00) }); // transmission type, 0x00-->synchronous

        // canopen-stack not yet implement
        // od.push_back({ CO_KEY(0x1400 + nth_rpdo_map, 3, CO_OBJ_D___RW),
        //                CO_TUNSIGNED16,
        //                (CO_DATA)(0x00) }); // inhibit time, 0-->disable
        // od.push_back(
        //   { CO_KEY(0x1400 + nth_rpdo_map, 4, CO_OBJ_D___RW), CO_TUNSIGNED8, (CO_DATA)(0x00) }); // compatibility
        //   entry
        // od.push_back({ CO_KEY(0x1400 + nth_rpdo_map, 5, CO_OBJ_D___RW),
        //                CO_TUNSIGNED16,
        //                (CO_DATA)(50) }); // event-timer, ms, 0->disable
        // sync start value


        od.push_back({ CO_KEY(0x1600 + nth_rpdo_map, 0, CO_OBJ_D___R_),
                       CO_TUNSIGNED8,
                       (CO_DATA)(pdo_it->second.size()) }); // index of object
        uint8_t tot_byte = 0;
        // iterate vector
        for (auto pdo_entry_it = pdo_it->second.begin(); pdo_entry_it != pdo_it->second.end(); ++pdo_entry_it) {
          uint32_t entryId = std::get<0>(*pdo_entry_it);
          // printf("[DEBUG] entryID:0x%x\n", entryId);
          assert(entryId >= 0x200000 && entryId <= 0xFFFFFF);
          uint8_t size_in_byte = std::get<1>(*pdo_entry_it);
          tot_byte += size_in_byte;
          assert(tot_byte <= 8 && "CANopen in CANbus CC require DLC <= 8");
          size_t nth_mapped_object = std::distance(pdo_it->second.begin(), pdo_entry_it) + 1;
          // mapping
          od.push_back(
            { CO_KEY(0x1600 + nth_rpdo_map, nth_mapped_object, CO_OBJ_D___R_),
              CO_TUNSIGNED32,
              (CO_DATA)(CO_LINK(0x2000 + nodeId, nth_mapped_memory, size_in_byte * 8)) }); // 1st mapped object


          od.push_back({ CO_KEY(0x2000 + nodeId, nth_mapped_memory, CO_OBJ_D___RW),
                         size_to_obj_type(size_in_byte),
                         (CO_DATA)(0) });
          ret_imd_pdo_data_map[std::make_tuple(nodeId, entryId)] = CO_DEV(0x2000 + nodeId, nth_mapped_memory) >> 8;
          nth_mapped_memory++;
          /**
           * @todo create relevant object at 0x20xx for mapping
           */
        } // end of entry iterate

        nth_rpdo_map++;
      } // end of rpdo iterate

      else {
        // rxpdo of slave --> txpdo of master
        printf("is_tpdo");
        if (pdo_it->second.size() == 0) {
          break;
        }

        // RPDO communication parameter
        od.push_back({ CO_KEY(0x1800 + nth_tpdo_map, 0, CO_OBJ_D___R_),
                       CO_TUNSIGNED8,
                       (CO_DATA)(2) }); // highest sub-index supported
        od.push_back(
          { CO_KEY(0x1800 + nth_tpdo_map, 1, CO_OBJ_D___RW),
            CO_TPDO_ID,
            (CO_DATA)(CO_COBID_TPDO_STD(1u, CO_COBID_RPDO_BASE + ((pdoId - 0x1600) * CO_COBID_TPDO_INC)) + nodeId) });
        od.push_back({ CO_KEY(0x1800 + nth_tpdo_map, 2, CO_OBJ_D___RW),
                       CO_TUNSIGNED8,
                       (CO_DATA)(0x01) }); // transmission type, trigger by sync
                                           // inhibit time
                                           // reserved
                                           // event-timer
                                           // SYNC start value


        od.push_back({ CO_KEY(0x1A00 + nth_tpdo_map, 0, CO_OBJ_D___R_),
                       CO_TUNSIGNED8,
                       (CO_DATA)(pdo_it->second.size()) }); // index of object

        uint8_t tot_byte = 0;
        // iterate vector
        for (auto pdo_entry_it = pdo_it->second.begin(); pdo_entry_it != pdo_it->second.end(); ++pdo_entry_it) {
          uint32_t entryId = std::get<0>(*pdo_entry_it);
          // printf("[DEBUG] entryID:0x%x\n", entryId);
          assert(entryId >= 0x200000 && entryId <= 0xFFFFFF);
          uint8_t size_in_byte = std::get<1>(*pdo_entry_it);
          tot_byte += size_in_byte;
          assert(tot_byte <= 8 && "CANopen in CANbus CC require DLC <= 8");
          size_t nth_mapped_object = std::distance(pdo_it->second.begin(), pdo_entry_it) + 1;
          // mapping
          od.push_back(
            { CO_KEY(0x1A00 + nth_tpdo_map, nth_mapped_object, CO_OBJ_D___R_),
              CO_TUNSIGNED32,
              (CO_DATA)(CO_LINK(0x2000 + nodeId, nth_mapped_memory, size_in_byte * 8)) }); // 1st mapped object

          od.push_back({ CO_KEY(0x2000 + nodeId, nth_mapped_memory, CO_OBJ_D___RW),
                         size_to_obj_type(size_in_byte),
                         (CO_DATA)(0) });
          ret_imd_pdo_data_map[std::make_tuple(nodeId, entryId)] = CO_DEV(0x2000 + nodeId, nth_mapped_memory) >> 8;
          nth_mapped_memory++;
          /**
           * @todo create relevant object at 0x20xx for mapping
           */
        } // end of entry iterate

        nth_tpdo_map++;
      } // end of tpdo iterate


      printf("\n");
    } // end of pdo iterate
    od.push_back({ CO_KEY(0x2000 + nodeId, 0x00, CO_OBJ_D___RW),
                   CO_TUNSIGNED8,
                   (CO_DATA)(nth_mapped_memory - 1) }); // subidx highest idx
  } // end of node iterate

  // sort for binary search
  std::sort(od.begin(), od.end(), [](CO_OBJ a, CO_OBJ b) { return a.Key < b.Key; });
  return ret_imd_pdo_data_map;
};

PDO_data_map
get_PDO_data_map(std::vector<CO_OBJ>& od, const Imd_PDO_data_map& imd_pdo_data_map)
{
  PDO_data_map ret_pdo_data_map;
  for (auto node_pdo_it = imd_pdo_data_map.begin(); node_pdo_it != imd_pdo_data_map.end(); ++node_pdo_it) {
    uint8_t node_id = std::get<0>(node_pdo_it->first);
    uint32_t pdo_idx = std::get<1>(node_pdo_it->first);
    uint32_t local_od_idx = node_pdo_it->second;
    std::vector<CO_OBJ>::iterator it = std::lower_bound(
      od.begin(), od.end(), local_od_idx, [](const CO_OBJ& obj, uint32_t idx) { return (obj.Key >> 8) < idx; });
    ret_pdo_data_map[std::make_tuple(node_id, pdo_idx)] = (void*)(&it->Data);
  } // end of node iterate
  return ret_pdo_data_map;
}

void
print_od(CO_OBJ_T* OD, uint16_t after_p_idx, uint16_t before_p_idx)
{
  for (uint32_t i = 0; i < 0xFFFFFF; i++)
  // CO_OBJ_T *OD = od.data();
  // for (int i = 0; i < od.size(); i++)
  {
    uint16_t p_idx = OD[i].Key >> 16;
    if (p_idx == 0 || p_idx >= before_p_idx) {
      // touch CO_OBJ_DICT_ENDMARK
      break;
    }
    if (p_idx < after_p_idx)
      continue;
    uint8_t sub_idx = (OD[i].Key >> 8) & 0xFF;
    auto type = OD[i].Type;

    // printf("key:0x%x \n", OD[i].Key);
    printf("p_idx:0x%x sub_idx:0x%x data:0x%lx \n", p_idx, sub_idx, OD[i].Data);
  }
}

void
print_data_map(const PDO_data_map& pdo_data_map)
{
  for (auto& [node_idx_tup, pdo_data] : pdo_data_map) {
    // printf("pdo_idx:0x%x data:%p \n", pdo_idx, pdo_data);
    auto node_id = std::get<0>(node_idx_tup);
    auto pdo_idx = std::get<1>(node_idx_tup);
    printf("node_id:%d, pdo_idx:0x%x, data:0x%08x, data_ptr:%p \n", node_id, pdo_idx, *(uint32_t*)pdo_data, pdo_data);
  }
}
