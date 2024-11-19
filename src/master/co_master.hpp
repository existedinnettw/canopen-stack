#pragma once

#include "co_core.h"
#include "co_nmt_master.h"

#include <map>
#include <od_util.hpp>
#include <thread>
#include <vector>

// #include "pdo_map.hpp"

/**
 * may use `COIfCanReceive` to add master logic.
 */

enum Co_master_state
{
  CO_MASTER_IDLE,   // can config all NMT slave node, and they stayed in PREOP
  CO_MASTER_ACTIVE, // all slave switched to OP state
  CO_MASTER_EXITING,
  CO_MASTER_EXIT
};

enum Nmt_monitor_method
{
  HEARTBEAT,
  BYPASS,
  // NODE_GUARD, // never support due to RTR need
  // RXPDO_TIMEOUT, //just a workaround for node unsupport heartbeat, not yet implement
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
   * config hb producer with SDO
   */
  void config_nmt_monitor();

  /**
   * @brief
   * config pdo mapping of slave through SDO client
   */
  void config_pdo_mapping(const Slave_model_config& config);

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

  // remote slave OD store in underline memory
  uint8_t node_id = 127; // node id shoul in [1, 127]

  friend class Master_node;
  CO_CSDO* _csdo = nullptr; // private, friend, master_node only
private:
  Nmt_monitor_method monitor_method = HEARTBEAT;
  CO_MODE _BYPASS_fake_nmt_mode = CO_INIT; // for BYPASS only
  CO_HBCONS* _hbc;                         // for HEARTBEAT method only

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
  Master_node(CO_NODE& master_node);
  /**
   *@see rule of three
   */
  Master_node(const Master_node& other) // II. copy constructor
    : Master_node(other.master_node) {};
  ~Master_node();

  /**
   * @attention
   * input slave_model should on stack memory or container to handle its own memory lifecycle.
   */
  void bind_slave(Slave_node_model& slave_model);

  Slave_node_model& get_slave(uint8_t node_id);

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
   * @details
   * enable slaves hb producer(0x1017), enable master hb consumer
   * And check all reach PREOP
   */
  void config_nmt_monitors();

  /**
   * @brief
   * config all pdo mapping of slaves through SDO client
   * @details
   * currently, all pdo of slaves will be configed in synchronous
   */
  void config_pdo_mappings(Slave_model_configs& configs);

  /**
   * @brief
   * turn all slave into OP state.
   * @details
   * user have to handle its own realtime callback after this function
   */
  void activate(uint32_t cycle_time_us = 10 * 1000);

  /**
   * @details
   * need to call at each realtime
   */
  void process();

  void release();

  /**
   * @brief
   * set both master and slaves NMT state
   */
  void set_slaves_NMT_mode(CO_NMT_command_t cmd);

private:
  std::map<uint8_t, Slave_node_model*> slave_node_models; // node_id-->slave model
  Co_master_state logic_state = CO_MASTER_IDLE;
  CO_NODE& master_node;
  uint8_t max_slave_num = 0;

  std::thread config_commu_thread;
};