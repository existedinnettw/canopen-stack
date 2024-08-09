/******************************************************************************
   Copyright 2020 Embedded Office GmbH & Co. KG

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
******************************************************************************/

#ifndef DRV_SOCKETCAN_LINUX_H_
#define DRV_SOCKETCAN_LINUX_H_

#ifdef __cplusplus /* for compatibility with C++ environments  */
extern "C"
{
#endif

    /******************************************************************************
     * INCLUDES
     ******************************************************************************/

#include "co_if.h"

    /******************************************************************************
     * PUBLIC SYMBOLS
     ******************************************************************************/

    /**
     * @brief
     * implement linux socketcan for CANopen interface.
     * @details
     * ```c
     * CO_LNX_SKTCAN Linux_Socketcan_CanDriver;
     * COLnxSktCanInit(&Linux_Socketcan_CanDriver)
     * ```
     */
    typedef struct CO_LNX_SKTCAN_T
    {
        /**
         * @public
         */
        CO_IF_CAN_DRV super; // super
        /**
         * @private
         */
        char CanInterfaceName[32];
        int CanSocket;
    } CO_LNX_SKTCAN;

    /**
     * ctor of CO_LNX_SKTCAN
     */
    void COLnxSktCanInit(CO_LNX_SKTCAN *self, const char *if_name);

#ifdef __cplusplus /* for compatibility with C++ environments  */
}
#endif

#endif
