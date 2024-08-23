
#ifndef CO_NMT_MASTER_H_
#define CO_NMT_MASTER_H_

#ifdef __cplusplus /* for compatibility with C++ environments  */
extern "C"
{
#endif

    /******************************************************************************
     * INCLUDES
     ******************************************************************************/

#include "co_obj.h"
#include "co_if.h"
#include "co_err.h"

    /******************************************************************************
     * PuBLIC INCLUDES
     ******************************************************************************/

#define CO_NMT_ALLOWED 0x01  /*!< indication of NMT transfers allowed    */
#define CO_BOOT_ALLOWED 0x02 /*!< indication of BOOT transfers allowed   */
#define CO_EMCY_ALLOWED 0x04 /*!< indication of EMCY transfers allowed   */
#define CO_TIME_ALLOWED 0x08 /*!< indication of TIME transfers allowed   */
#define CO_SYNC_ALLOWED 0x10 /*!< indication of SYNC transfers allowed   */
#define CO_SDO_ALLOWED 0x20  /*!< indication of SDO transfers allowed    */
#define CO_PDO_ALLOWED 0x40  /*!< indication of PDO transfers allowed    */

    /******************************************************************************
     * PUBLIC TYPES
     ******************************************************************************/

    struct CO_NODE_T; /* Declaration of canopen node structure       */

    // typedef struct CO_NMT_MASTER_T;

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
     * @param nodeID [0x0,0x80], 0x0 imply all node, others specifiy node id.
     */
    CO_ERR
    CO_NMT_sendCommand(struct CO_NODE_T *node, CO_NMT_command_t command, uint8_t nodeID);

    /******************************************************************************
     * PRIVATE FUNCTIONS
     ******************************************************************************/

    /******************************************************************************
     * CALLBACK FUNCTIONS
     ******************************************************************************/

#ifdef __cplusplus /* for compatibility with C++ environments  */
}
#endif

#endif /* #ifndef CO_NMT_MASTER_H_ */
