/******************************************************************************
   @author Sandro Gort

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

/******************************************************************************
 * INCLUDES
 ******************************************************************************/

#include "drv_can_socketcan.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h> //import timeval
#include <linux/can.h>
#include <linux/can/raw.h>

/******************************************************************************
 * PRIVATE TYPE DEFINITION
 ******************************************************************************/

/******************************************************************************
 * PRIVATE DEFINES
 ******************************************************************************/
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)((char *)__mptr - offsetof(type,member)); })

/******************************************************************************
 * PRIVATE VARIABLES
 ******************************************************************************/

/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

static void DrvCanInit(CO_IF_CAN_DRV *super);
static void DrvCanEnable(CO_IF_CAN_DRV *super, uint32_t baudrate);
static int16_t DrvCanSend(CO_IF_CAN_DRV *super, CO_IF_FRM *frm);
static int16_t DrvCanRead(CO_IF_CAN_DRV *super, CO_IF_FRM *frm);
static void DrvCanReset(CO_IF_CAN_DRV *super);
static void DrvCanClose(CO_IF_CAN_DRV *super);

/******************************************************************************
 * PUBLIC VARIABLE
 ******************************************************************************/

/******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/
void COLnxSktCanInit(CO_LNX_SKTCAN *self, char *if_name)
{
    self->super.Init = DrvCanInit;
    self->super.Enable = DrvCanEnable;
    self->super.Read = DrvCanRead;
    self->super.Send = DrvCanSend;
    self->super.Reset = DrvCanReset;
    self->super.Close = DrvCanClose;

    strncpy(self->CanInterfaceName, if_name, sizeof(self->CanInterfaceName) - 1);
    self->CanInterfaceName[sizeof(self->CanInterfaceName) - 1] = '\0';
}
/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

static void DrvCanInit(CO_IF_CAN_DRV *super)
{
    printf("%s\n", __func__);

    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);
    self->CanSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (self->CanSocket < 0)
    {
        printf("Creating CAN socket failed\n");
        while (1)
        {
        }
    }

    struct sockaddr_can addr = {0};
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex(self->CanInterfaceName);
    if (addr.can_ifindex == 0)
    {
        printf("Invalid Parameter, can interface name unknown (%s)\n", self->CanInterfaceName);
        while (1)
        {
        }
    }

    int fd = bind(self->CanSocket, (struct sockaddr *)(&addr), sizeof(addr));
    if (fd < 0)
    {
        printf("Open CAN socket %s failed\n", self->CanInterfaceName);
        while (1)
        {
        }
    }

    struct timeval tvRx = {0, 1};
    setsockopt(self->CanSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)(&tvRx), sizeof(tvRx));
    struct timeval tvTx = {0, 100000};
    setsockopt(self->CanSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)(&tvTx), sizeof(tvTx));

    printf("CAN socket initialization success\n");
}

static void DrvCanEnable(CO_IF_CAN_DRV *super, uint32_t baudrate)
{
    printf("%s\n", __func__);

    /* Currently CAN socket needs to be enabled beforehand via command line

    sudo modprobe vcan
    sudo ip link add dev can0 type vcan
    sudo ip link set up can0
    */
}

static int16_t DrvCanSend(CO_IF_CAN_DRV *super, CO_IF_FRM *frm)
{
    struct can_frame frame = {0};
    frame.can_id = frm->Identifier;
    frame.can_dlc = frm->DLC;
    for (uint8_t i = 0; i < 8; ++i)
    {
        frame.data[i] = frm->Data[i];
    }
    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);
    int16_t bytesWritten = write(self->CanSocket, &frame, sizeof(frame));
    if (bytesWritten < 0)
    {
        printf("DrvCanSend failed\n");
        return 0;
    }
    return sizeof(CO_IF_FRM);
}

static int16_t DrvCanRead(CO_IF_CAN_DRV *super, CO_IF_FRM *frm)
{
    struct can_frame frame = {0};
    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);

    auto bytesReceived = read(self->CanSocket, &frame, sizeof(frame));
    if (bytesReceived <= 0)
    {
        // std::cout << "DrvCanRead failed" << std::endl;
        return 0;
    }
    frm->Identifier = frame.can_id;
    frm->DLC = frame.can_dlc;
    for (uint8_t i = 0; i < 8; ++i)
    {
        frm->Data[i] = frame.data[i];
    }
    return sizeof(CO_IF_FRM);
}

static void DrvCanReset(CO_IF_CAN_DRV *super)
{
    printf("%s\n", __func__);

    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);

    DrvCanClose(super);
    DrvCanInit(super);
}

static void DrvCanClose(CO_IF_CAN_DRV *super)
{
    printf("%s\n", __func__);

    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);

    close(self->CanSocket);
}
