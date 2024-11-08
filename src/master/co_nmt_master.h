
#ifndef CO_NMT_MASTER_H_
#define CO_NMT_MASTER_H_

#ifdef __cplusplus /* for compatibility with C++ environments  */
extern "C"
{
#endif

  /******************************************************************************
   * INCLUDES
   ******************************************************************************/

#include "co_err.h"
#include "co_if.h"
#include "co_nmt.h"
#include "co_obj.h"

  /******************************************************************************
   * PUBLIC INCLUDES
   ******************************************************************************/

  /******************************************************************************
   * PUBLIC TYPES
   ******************************************************************************/

  struct CO_NODE_T; /* Declaration of canopen node structure       */

  // typedef struct CO_NMT_MASTER_T;

  /**
   * @ref CONmtModeCode
   * @see canopennode CO_NMT_command_t
   */
  typedef enum
  {
    CO_NMT_NO_COMMAND = 0,              /**< 0, No command */
    CO_NMT_ENTER_OPERATIONAL = 1,       /**< 1, Start device */
    CO_NMT_ENTER_STOPPED = 2,           /**< 2, Stop device */
    CO_NMT_ENTER_PRE_OPERATIONAL = 128, /**< 128, Put device into pre-operational */
    CO_NMT_RESET_NODE = 129,            /**< 129, Reset device */
    CO_NMT_RESET_COMMUNICATION = 130    /**< 130, Reset CANopen communication on device */
  } CO_NMT_command_t;

  /******************************************************************************
   * PUBLIC FUNCTIONS
   ******************************************************************************/

  /**
   * @brief
   * convert NMT command (event) to specifiy NMT mode
   * @details
   * assume transition success
   */
  CO_MODE CO_NMT_command_t_to_CO_MODE(CO_NMT_command_t command);

  /**
   * @brief send NMT command to other node
   * @param node pointer to self node structure
   * @param command NMT command, event of state transition request.
   * @param nodeID target node id. range between [0x0,127]. 0x0 imply all node, others specifiy node id.
   */
  CO_ERR
  CO_NMT_sendCommand(struct CO_NODE_T* node, CO_NMT_command_t command, uint8_t nodeID);

  /******************************************************************************
   * PRIVATE FUNCTIONS
   ******************************************************************************/

  /******************************************************************************
   * CALLBACK FUNCTIONS
   ******************************************************************************/

#ifdef __cplusplus /* for compatibility with C++ environments  */
}
#endif

#endif /* ifndef CO_NMT_MASTER_H_ */
