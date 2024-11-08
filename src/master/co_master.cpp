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

  // wait until command success?
  // call CONodeProcess directly?
  // CONodeProcess()

  // how to check? monitor the received CAN message by `COIfCanReceive` or heartbeat consumer OD entry?
  // or override CONmtHbConsChange

  switch (monitor_method) {
    case HEARTBEAT: {
      // search, if HbCons in treemap, code will much cleaner
      // move to init?
      CO_HBCONS* hbc = master_node.Nmt.HbCons;
      if (hbc == nullptr) {
        // return
      }
      while (hbc != 0) {
        if (hbc->NodeId != node_id) {
          hbc = hbc->Next;
        } else {
          // hbc->NodeId == node_id
          CO_MODE current_state = hbc->State;
          // may better use get_mode()
        }
      } // end of while
      break;
    }
    case RXPDO_TIMEOUT: {
      // assume success after delay
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      this->nmt_mode = CO_NMT_command_t_to_CO_MODE(cmd);
      break;
    }
  }
};

CO_MODE
Slave_node_model::get_NMT_state(){};

void
Slave_node_model::config_pdo_mapping() {};

void
Slave_node_model::b_send_sdo_request() {};

void
Slave_node_model::nb_send_sdo_request() {};


//====================================================

void
Master_node::start_config()
{
  CONodeStart(&master_node);
  this->config_commu_thread = std::thread([this]() {
    while (true) {
      this->process();
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (this->logic_state == CO_MASTER_ACTIVE) {
        break;
      }
    }
  });
}

void
Master_node::wait_all_preop()
{
  while (true) {
    CO_MODE state;
    bool all_preop = true;
    for (auto& slave : this->slave_node_model_list) {
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
  for (auto& slave : this->slave_node_model_list) {
    slave.config_pdo_mapping();
  }
}

void
Master_node::activate()
{
  CO_NMT_sendCommand(&master_node, CO_NMT_ENTER_OPERATIONAL, 0); // all node

  this->logic_state = CO_MASTER_ACTIVE;
  this->config_commu_thread.join();

  // check if all reach OP at ones own logic later
}

void
Master_node::process()
{
  CONodeProcess(&master_node);
  COTmrService(&master_node.Tmr);
  COTmrProcess(&master_node.Tmr);
  CONodeProcess(&master_node);
}

void
Master_node::self_master_pdo_mapping()
{
}