#include "co_nmt_master.h"
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
Slave_node_model::config_pdo_mapping() {

};

uint32_t
Slave_node_model::b_send_sdo_request(uint32_t idx, bool trans_type, void* val_buf, size_t size)
{
  CO_ERR err = CO_ERR_NONE;
  while ((err = this->nb_send_sdo_request(idx, trans_type, val_buf, size)) != CO_ERR_NONE) {
    /* code */
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  CO_CSDO* csdo = COCSdoFind(&master_node, 0x0);
  while (csdo->State != CO_CSDO_STATE_IDLE) {
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

  printf("[DEBUG] sdo received response at index: 0x%04x, sub: 0x%02x, code: 0x%08x\n", index, sub, code);
  Slave_node_model* self = (Slave_node_model*)parg;

  self->_last_csdo_abort_code = code;
  switch (code) {
    case 0:
      printf("[DEBUG] SDO response successfully\n");
      // store the value
      break;
    // decode based on abort code
    case CO_SDO_ERR_TIMEOUT:
      printf("[WARN] SDO protocol timed out\n");
      break;
    case CO_SDO_ERR_OBJ:
      printf("[WARN] Object doesn't exist in target dictionary\n");
      break;
    case CO_SDO_ERR_WR:
      printf("[WARN] Attempt to write a read only object\n");
      break;
    case CO_SDO_ERR_OBJ_MAP_N:
      printf("[WARN] Number and length exceed PDO\n");
      break;
    case CO_SDO_ERR_DATA_LOCAL:
      printf("[WARN] Data cannot be transferred or stored to the application because of local control\n");
      break;
    default:
      printf("[WARN] general abort sdo request\n");
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
  CO_CSDO* csdo;
  csdo = COCSdoFind(&master_node, 0x0); // need to modify based on config
  if (csdo == 0) {
    printf("[ERROR] SDO client #0 is missing or busy\n");
  }
  uint32_t timeout = 500;
  CO_ERR err = CO_ERR_NONE;

  // if (csdo->State != CO_CSDO_STATE_IDLE) {
  //   }

  // sdo client send

  // client sdo with transmission state, callback used for update state.

  // if there is edian problem?
  if (trans_type == 0) {
    err = COCSdoRequestDownload(csdo, idx << 8, (uint8_t*)val_buf, size, TS_AppCSdoCallback, this, timeout);
  } else {
    err = COCSdoRequestUpload(csdo, idx << 8, (uint8_t*)val_buf, size, TS_AppCSdoCallback, this, timeout);
  }
  return err;
};


//====================================================

void
Master_node::start_config()
{
  CONodeStart(&master_node);
  this->config_commu_thread = std::thread([this]() {
    while (true) {
      // printf("[DEBUG] config coomu callback called\n");
      this->process();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
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
Master_node::config_pdos()
{
  this->self_master_pdo_mapping();
  for (auto& slave : this->slave_node_models) {
    slave.config_pdo_mapping();
  }
}

void
Master_node::activate()
{
  this->set_slaves_NMT_mode(CO_NMT_ENTER_OPERATIONAL);

  this->logic_state = CO_MASTER_ACTIVE;
  this->config_commu_thread.join();

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