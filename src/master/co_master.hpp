#pragma once

#include "co_core.h"

#include <map>
#include <thread>
#include <vector>

/**
 * may use `COIfCanReceive` to add master logic.
 */

enum Co_master_state
{
  CO_MASTER_IDLE,  // can config all NMT slave node, and they stayed in PREOP
  CO_MASTER_ACTIVE // all slave switched to OP state
};

enum Nmt_monitor_method
{
  HEARTBEAT,
  RXPDO_TIMEOUT, // just a workaround for node unsupport heartbeat
                 // NODE_GUARD //never support due to RTR need
};

/**
 * @details
 * Slave here isn't mean NMT slave, but software slave in master/slave abstraction.
 * Aware that most of cia301 spec use producer/consumer rather than in master/slave.
 * `Model` imply that it isn't actually 1:1 mapping to real slave node, but a software
 * handle.
 */
class Slave_node_model
{
public:
  Slave_node_model(uint8_t node_id, CO_NODE& master_node, Nmt_monitor_method monitor_method);
  ~Slave_node_model() {};

  /**
   *
   */
  void set_NMT_mode(CO_NMT_command_t cmd);

  CO_MODE get_NMT_state();

  /**
   * config pdo mapping through SDO client
   */
  void config_pdo_mapping();

  /**
   * @details
   * blocking mode?
   * @param trans_type 0 imply download, 1 imply upload
   * @return abort code, 0 imply success, else imply error
   */
  uint32_t b_send_sdo_request(uint32_t idx, bool trans_type, void* val_buf, size_t size);

  /**
   * @param trans_type 0 imply download, 1 imply upload
   * @param[in, out] val_buf as input if trans type is download. as output if trans type is upload
   */
  CO_ERR nb_send_sdo_request(uint32_t idx, bool trans_type, void* val_buf, size_t size);

  /**
   * @private
   */
  uint32_t _last_csdo_abort_code;

private:
  // remote slave OD store in underline memory
  uint8_t node_id = 127; // node id shoul in [1, 127]

  Nmt_monitor_method monitor_method = HEARTBEAT;
  CO_MODE _RXPDO_TIMEOUT_fake_nmt_mode = CO_INIT;

  //   CO_IF_CAN_DRV can_driver;                 // or use mater node?
  CO_NODE& master_node;
  std::map<uint32_t, uint32_t>
    pdo_mapping; // (remote_pdo_index (at this->node_id)) -> (local_pdo_index (of master node))
};

/**
 * @details
 * Master here isn't mean NMT slave, but software master in master/slave abstraction.
 */
class Master_node
{
public:
  /**
   * @details
   * will create basic OD
   */
  Master_node(CO_NODE& master_node)
    : master_node(master_node) {
      // od construct
    };
  Master_node(const Master_node&)
    : master_node(master_node) {};
  ~Master_node() {};

  /**
   * @brief
   * swtich master to PREOP
   */
  void start_config();

  /**
   * @brief
   * let slave reach PREOP state.
   */
  void start_config_slaves();

  /**
   *
   */
  void config_pdos();

  /**
   * @brief
   * turn all slave into OP state.
   * @details
   * user have to handle its own realtime callback after this function
   */
  void activate();

  /**
   * @details
   * need to call at each realtime
   */
  void process();

  // add node model through push_back directly
  std::vector<Slave_node_model> slave_node_models;

private:
  Co_master_state logic_state = CO_MASTER_IDLE;
  CO_NODE& master_node;

  std::thread config_commu_thread;

  void self_master_pdo_mapping();
  void set_slaves_NMT_mode(CO_NMT_command_t cmd);
};