#include "co_nmt_master.h"
#include <cassert>
#include <chrono>
#include <co_master.hpp>
#include <stdexcept>
#include <thread>

Slave_node_model::Slave_node_model(uint8_t node_id, CO_NODE& master_node, Nmt_monitor_method monitor_method)
  : monitor_method(monitor_method)
  , master_node(master_node)
{
  if (node_id > 127 || node_id < 1) {
    throw std::invalid_argument("Node ID must be between 1 and 127");
  }
  this->node_id = node_id;
}

void
Slave_node_model::set_NMT_mode(CO_NMT_command_t cmd)
{
  CO_NMT_sendCommand(&master_node, cmd, node_id);

  // how to check? monitor the received CAN message by `COIfCanReceive` or heartbeat consumer OD entry?
  // or override CONmtHbConsChange

  switch (monitor_method) {
    case HEARTBEAT: {
      break;
    }
    case RXPDO_TIMEOUT: {
      // assume success
      this->_RXPDO_TIMEOUT_fake_nmt_mode = CO_NMT_command_t_to_CO_MODE(cmd);
      break;
    }
  }
};

CO_MODE
Slave_node_model::get_NMT_state()
{
  switch (monitor_method) {
    case HEARTBEAT: {
      // search, if HbCons in treemap, code will much cleaner
      // @todo cache by pointer?
      CO_HBCONS* hbc = master_node.Nmt.HbCons;
      if (hbc == nullptr) {
        // return
      }
      while (hbc != 0) {
        if (hbc->NodeId != node_id) {
          hbc = hbc->Next;
        } else {
          // hbc->NodeId == node_id
          return hbc->State;
          // may better use get_mode()
        }
      } // end of while
    }
    case RXPDO_TIMEOUT: {
      return this->_RXPDO_TIMEOUT_fake_nmt_mode;
    }
  } // end of switch

  return CO_STOP;
};

void
Slave_node_model::config_pdo_mapping(Slave_model_config config)
{
  uint32_t abort_code = 0;
  for (auto pdo_it = config.begin(); pdo_it != config.end(); ++pdo_it) {
    int pdoId = pdo_it->first;
    printf("[DEBUG] pdo_id: %x, ", pdoId);
    assert((pdoId >= 0x1600 && pdoId <= 0x17FF) || (pdoId >= 0x1A00 && pdoId <= 0x1BFF));
    if (pdoId >= 0x1A00) {
      // txpdo of slave

      // close txpdo first
      uint8_t num_mapped_PDO = 0;
      abort_code = this->b_send_sdo_request(pdoId << 8 + 0x00, 0, &num_mapped_PDO, sizeof(num_mapped_PDO));
      // config transfer type
      uint8_t txpdo_trans_type = 0x01;
      abort_code =
        this->b_send_sdo_request((0x1800 + pdoId - 0x1A00) << 8 + 0x02, 0, &txpdo_trans_type, sizeof(txpdo_trans_type));

      for (auto pdo_entry_it = pdo_it->second.begin(); pdo_entry_it != pdo_it->second.end(); ++pdo_entry_it) {
        int entryId = std::get<0>(*pdo_entry_it);
        uint8_t size_in_byte = std::get<1>(*pdo_entry_it);
        size_t nth_mapped_object = std::distance(pdo_it->second.begin(), pdo_entry_it) + 1;

        uint32_t app_obj_map = CO_LINK(entryId >> 8, entryId | 0xFF, size_in_byte * 8);
        abort_code = this->b_send_sdo_request((pdoId << 8) + nth_mapped_object, 0, &app_obj_map, sizeof(app_obj_map));
      }

      // re-enable
      num_mapped_PDO = pdo_it->second.size();
      abort_code = this->b_send_sdo_request(pdoId << 8 + 0x00, 0, &num_mapped_PDO, sizeof(num_mapped_PDO));

    } else {
      // rxpdo of slave

      // close rxpdo first
      uint8_t num_mapped_PDO = 0;
      abort_code = this->b_send_sdo_request(pdoId << 8 + 0x00, 0, &num_mapped_PDO, sizeof(num_mapped_PDO));
      // config transfer type
      uint8_t rxpdo_trans_type = 0x00; // FE
      abort_code =
        this->b_send_sdo_request((0x1400 + pdoId - 0x1600) << 8 + 0x02, 0, &rxpdo_trans_type, sizeof(rxpdo_trans_type));

      for (auto pdo_entry_it = pdo_it->second.begin(); pdo_entry_it != pdo_it->second.end(); ++pdo_entry_it) {
        int entryId = std::get<0>(*pdo_entry_it);
        uint8_t size_in_byte = std::get<1>(*pdo_entry_it);
        size_t nth_mapped_object = std::distance(pdo_it->second.begin(), pdo_entry_it) + 1;

        uint32_t app_obj_map = CO_LINK(entryId >> 8, entryId | 0xFF, size_in_byte * 8);
        abort_code = this->b_send_sdo_request((pdoId << 8) + nth_mapped_object, 0, &app_obj_map, sizeof(app_obj_map));
      }

      // re-enable
      num_mapped_PDO = pdo_it->second.size();
      abort_code = this->b_send_sdo_request(pdoId << 8 + 0x00, 0, &num_mapped_PDO, sizeof(num_mapped_PDO));
    }
  }
};

uint32_t
Slave_node_model::b_send_sdo_request(uint32_t idx, bool trans_type, void* val_buf, size_t size)
{
  CO_ERR err = CO_ERR_NONE;
  while ((err = this->nb_send_sdo_request(idx, trans_type, val_buf, size)) != CO_ERR_NONE) {
    /* code */
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  while (_csdo->State != CO_CSDO_STATE_IDLE) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  // @see COCSdoTransferFinalize, at this situation, abort code already reset
  // if (this->_last_csdo_abort_code != 0) {
  //   throw std::runtime_error("SDO request failed");
  // }
  return this->_last_csdo_abort_code;
};


static void
TS_AppCSdoCallback(CO_CSDO* csdo, uint16_t index, uint8_t sub, uint32_t code, void* parg)
{
  // checkout Table 22: SDO abort codes, CO_SDO_ERR_OBJ
  // The stack not yet implement response handle yet
  // can refer to `COSdoGetObject` and `igh::ecrt_sdo_request_state, ec_sdo_request_t`
  // check COCSdoTransferFinalize for calling detail

  printf("[DEBUG] sdo received response at index: 0x%04x, sub: 0x%02x, code: 0x%08x, state: ", index, sub, code);
  Slave_node_model* self = (Slave_node_model*)parg;

  self->_last_csdo_abort_code = code;
  switch (code) {
    case 0:
      printf("successfully\n");
      // store the value
      break;
    // decode based on abort code
    case CO_SDO_ERR_TIMEOUT:
      printf("SDO protocol timed out\n");
      break;
    case CO_SDO_ERR_OBJ:
      printf("Object doesn't exist in target dictionary\n");
      break;
    case CO_SDO_ERR_WR:
      printf("Attempt to write a read only object\n");
      break;
    case CO_SDO_ERR_OBJ_MAP_N:
      printf("Number and length exceed PDO\n");
      break;
    case CO_SDO_ERR_DATA_LOCAL:
      printf("Data cannot be transferred or stored to the application because of local control\n");
      break;
    default:
      printf("general abort sdo request\n");
      break;
  }
}

/**
 * @param idx SDO index, e.g. 0x100501 imply primary index 0x1005 and sub idx 05
 * @see ecrt_slave_config_create_sdo_request
 * @see ecrt_sdo_request_write
 * @todo write and read
 */
CO_ERR
Slave_node_model::nb_send_sdo_request(uint32_t idx, bool trans_type, void* val_buf, size_t size)
{
  if (_csdo == nullptr) {
    throw std::runtime_error("[ERROR] SDO client is missing, you may need to bind to master first\n");
  }
  uint32_t timeout = 500;
  CO_ERR err = CO_ERR_NONE;

  // if (csdo->State != CO_CSDO_STATE_IDLE) {
  //   }

  // sdo client send

  // client sdo with transmission state, callback used for update state.

  // if there is edian problem?
  if (trans_type == 0) {
    err = COCSdoRequestDownload(_csdo, idx << 8, (uint8_t*)val_buf, size, TS_AppCSdoCallback, this, timeout);
  } else {
    err = COCSdoRequestUpload(_csdo, idx << 8, (uint8_t*)val_buf, size, TS_AppCSdoCallback, this, timeout);
  }
  return err;
};


//====================================================
Master_node::Master_node(CO_NODE& master_node)
  : master_node(master_node)
{
  // od should constructed and inited

  // find support max slave num by sdo client in OD
  while (CODictFind(&master_node.Dict, CO_DEV(0x1280 + this->max_slave_num, 0))) {
    this->max_slave_num++;
    if (this->max_slave_num > CO_CSDO_N) {
      throw std::runtime_error("Exceed support num of slave nodes of underline canopen-stack.");
    }
  };
  this->slave_node_models.reserve(this->max_slave_num);
};

void
Master_node::bind_slave(Slave_node_model& slave_model)
{
  this->slave_node_models.push_back(slave_model);
  size_t pos = this->slave_node_models.size() - 1;
  slave_model._csdo = COCSdoFind(&master_node, pos);
  if (slave_model._csdo == nullptr) {
    throw std::runtime_error("[ERROR] SDO client is missing\n");
  }
}

void
Master_node::start_config()
{
  // sync message timing should keep stable after 1st sync, or slave will enter ERROR state for PDO
  // so disable sync producer first, unless both PREOP and OP can be gurantee stable
  // Choosed no sync message during master PREOP.

  uint32_t own_0x1005_val = 0x80; // @see CO_SYNC_COBID_ON
  CODictWrBuffer(&master_node.Dict, CO_DEV(0x1005, 0), (uint8_t*)&own_0x1005_val, sizeof(own_0x1005_val));

  CONodeStart(&master_node);
  this->config_commu_thread = std::thread([this]() {
    while (true) {
      // printf("[DEBUG] config coomu callback called\n");
      this->process();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      if (this->logic_state == CO_MASTER_ACTIVE) {
        break;
      }
    }
  });
}

void
Master_node::set_slaves_NMT_mode(CO_NMT_command_t cmd)
{
  CO_NMT_sendCommand(&master_node, cmd, master_node.NodeId);
  for (auto& slave : this->slave_node_models) {
    slave.set_NMT_mode(cmd);
  }
}

/**
 * @todo timeout?
 */
void
Master_node::start_config_slaves()
{
  printf("[DEBUG] set PREOP\n");
  this->set_slaves_NMT_mode(CO_NMT_ENTER_PRE_OPERATIONAL);

  // wait all preop
  while (true) {
    CO_MODE state;
    bool all_preop = true;
    for (auto& slave : this->slave_node_models) {
      state = slave.get_NMT_state();
      if (state != CO_PREOP) {
        all_preop = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        break;
      }
    }

    if (all_preop) {
      break;
    }
  }
}

void
Master_node::config_pdo_mappings(Slave_model_configs configs)
{
  this->self_master_pdo_mapping();
  for (auto& slave : this->slave_node_models) {
    auto config = configs[slave.node_id];
    slave.config_pdo_mapping(config);
  }
}

void
Master_node::activate()
{
  this->logic_state = CO_MASTER_ACTIVE;
  this->config_commu_thread.join();

  // enable sync producer
  uint32_t own_0x1005_val = CO_SYNC_COBID_ON | 0x80;
  CODictWrBuffer(&master_node.Dict, CO_DEV(0x1005, 0), (uint8_t*)&own_0x1005_val, sizeof(own_0x1005_val));

  this->set_slaves_NMT_mode(CO_NMT_ENTER_OPERATIONAL);
  // check if all reach OP at ones own logic later
}

void
Master_node::process()
{
  // printf("[DEBUG] process called\n");
  CONodeProcess(&master_node);
  COTmrService(&master_node.Tmr);
  COTmrProcess(&master_node.Tmr);
  CONodeProcess(&master_node);
}

void
Master_node::self_master_pdo_mapping()
{
}