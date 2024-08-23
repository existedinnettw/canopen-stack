
#include "co_nmt_master.h"

#include "co_core.h"

#include "stdio.h"

/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/
CO_NMT_command_t CO_MODE_to_CO_NMT_command_t(CO_MODE internalCommand)
{
    switch (internalCommand)
    {
    case CO_INIT:
        return CO_NMT_RESET_NODE;
    case CO_PREOP:
        return CO_NMT_ENTER_PRE_OPERATIONAL;
    case CO_OPERATIONAL:
        return CO_NMT_ENTER_OPERATIONAL;
    case CO_STOP:
        return CO_NMT_ENTER_STOPPED;
    default:
        return CO_NMT_NO_COMMAND;
    }
}

CO_MODE CO_NMT_command_t_to_CO_MODE(CO_NMT_command_t command)
{
    switch (command)
    {
    case CO_NMT_RESET_NODE:
    case CO_NMT_RESET_COMMUNICATION:
        return CO_INIT;
    case CO_NMT_ENTER_PRE_OPERATIONAL:
        return CO_PREOP;
    case CO_NMT_ENTER_OPERATIONAL:
        return CO_OPERATIONAL;
    case CO_NMT_ENTER_STOPPED:
        return CO_STOP;
    default:
        return CO_NMT_NO_COMMAND;
    }
}

/******************************************************************************
 * PUBLIC API FUNCTIONS
 ******************************************************************************/

/**
 * @see https://github.com/CANopenNode/CANopenNode/blob/58012aae34928d823abed5a2556bdab91ed3f4a2/301/CO_NMT_Heartbeat.c#L274
 * @ref CONmtBootup
 */
CO_ERR
CO_NMT_sendCommand(CO_NODE *node, CO_NMT_command_t command, uint8_t nodeID)
{
    /* verify arguments */
    if (node == NULL)
    {
        return CO_ERR_BAD_ARG;
    }
    if (command == CO_NMT_NO_COMMAND)
    {
        return CO_ERR_BAD_ARG;
    }

    /* Apply NMT command also to this node, if set so. */
    if ((nodeID == 0U) || (nodeID == node->NodeId))
    {
        CONmtSetMode(&node->Nmt, CO_NMT_command_t_to_CO_MODE(command));
        if (nodeID == node->NodeId)
        {
            return CO_ERR_NONE;
        }
    }

    /* Send NMT master message. */
    CO_IF_FRM frm;
    /* build NMT protocol frame  */
    CO_SET_ID(&frm, 0x000);
    /* 'start remote node' */
    CO_SET_BYTE(&frm, (uint8_t)command, 0);
    printf("nodeID:0x%x\n", nodeID);
    CO_SET_BYTE(&frm, nodeID, 1);
    CO_SET_DLC(&frm, 2);
    for (int i = 0; i < 8; i++)
    {
        printf("0x%x\n", frm.Data[i]);
    }

    (void)COIfCanSend(&(node->If), &frm); /* send NMT command  */
    return CO_ERR_NONE;
}