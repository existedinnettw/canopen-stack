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
   */
  void b_send_sdo_request();

  void nb_send_sdo_request();

private:
  // remote slave OD store in underline memory
  uint8_t node_id = 127;      // node id shoul in [1, 127]
  CO_MODE nmt_mode = CO_INIT; // expected remote node state
  Nmt_monitor_method monitor_method = HEARTBEAT;
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
  ~Master_node() {};

  void start_config();

  void wait_all_preop();

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

private:
  std::vector<Slave_node_model> slave_node_model_list;
  Co_master_state logic_state = CO_MASTER_IDLE;
  CO_NODE& master_node;

  std::thread config_commu_thread;

  void self_master_pdo_mapping();
};