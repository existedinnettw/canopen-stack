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
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h> //import timeval
#include <linux/can.h>
#include <linux/can/raw.h>
#include <errno.h>
#include <time.h>

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

static void DrvCanInit(const CO_IF_CAN_DRV *super);
static void DrvCanEnable(const CO_IF_CAN_DRV *super, uint32_t baudrate);
static int16_t DrvCanSend(const CO_IF_CAN_DRV *super, CO_IF_FRM *frm);
static int16_t DrvCanNBSend(const CO_IF_CAN_DRV *super, CO_IF_FRM *frm);
static int16_t DrvCanRead(const CO_IF_CAN_DRV *super, CO_IF_FRM *frm);
static void DrvCanReset(const CO_IF_CAN_DRV *super);
static void DrvCanClose(const CO_IF_CAN_DRV *super);

/******************************************************************************
 * PUBLIC VARIABLE
 ******************************************************************************/

/******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/
void COLnxSktCanInit(CO_LNX_SKTCAN *self, const char *if_name)
{
    self->super.Init = DrvCanInit;
    self->super.Enable = DrvCanEnable;
    self->super.Read = DrvCanRead;
    self->super.Send = DrvCanSend;
    self->super.Send = DrvCanNBSend;
    self->super.Reset = DrvCanReset;
    self->super.Close = DrvCanClose;

    strncpy(self->CanInterfaceName, if_name, sizeof(self->CanInterfaceName) - 1);
    self->CanInterfaceName[sizeof(self->CanInterfaceName) - 1] = '\0';
}
/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

static void DrvCanInit(const CO_IF_CAN_DRV *super)
{
    printf("%s\n", __func__);

    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);
    self->CanSocket = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (self->CanSocket < 0)
    {
        perror("Creating CAN socket failed");
        exit(EXIT_FAILURE);
    }

    struct timeval tvRx = {0, 1}; // 1 microsecond timeout for receive
    if (setsockopt(self->CanSocket, SOL_SOCKET, SO_RCVTIMEO, &tvRx, sizeof(tvRx)) < 0)
    {
        perror("Setting receive timeout failed");
        close(self->CanSocket);
        exit(EXIT_FAILURE);
    }
    struct timeval tvTx = {0, 100000}; // 100ms
    if (setsockopt(self->CanSocket, SOL_SOCKET, SO_SNDTIMEO, &tvTx, sizeof(tvTx)) < 0)
    {
        perror("Setting send timeout failed");
        close(self->CanSocket);
        exit(EXIT_FAILURE);
    }

    int buffer_size = 4096; // 4kiB
    if (setsockopt(self->CanSocket, SOL_SOCKET, SO_SNDBUF, &buffer_size, sizeof(buffer_size)) < 0)
    {
        perror("setsockopt(SO_SNDBUF)");
    }

    // Set the socket to non-blocking mode for reading
    int flags = fcntl(self->CanSocket, F_GETFL, 0);
    if (flags < 0)
    {
        perror("fcntl(F_GETFL) failed");
        close(self->CanSocket);
        exit(EXIT_FAILURE);
    }
    if (fcntl(self->CanSocket, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        perror("fcntl(F_SETFL) failed");
        close(self->CanSocket);
        exit(EXIT_FAILURE);
    }

    // Set a receive filter so we only receive select CAN IDs
    // can set CAN_RAW_FILTER

    struct sockaddr_can addr = {0};
    addr.can_family = AF_CAN;
    addr.can_ifindex = if_nametoindex(self->CanInterfaceName);
    if (addr.can_ifindex == 0)
    {
        fprintf(stderr, "Invalid Parameter, CAN interface name unknown (%s)\n", self->CanInterfaceName);
        close(self->CanSocket);
        exit(EXIT_FAILURE);
    }

    if (bind(self->CanSocket, (struct sockaddr *)(&addr), sizeof(addr)) < 0)
    {
        perror("Binding CAN socket failed");
        close(self->CanSocket);
        exit(EXIT_FAILURE);
    }

    printf("CAN socket initialization success\n");
}

static void DrvCanEnable(const CO_IF_CAN_DRV *super, uint32_t baudrate)
{
    printf("%s\n", __func__);

    /* Currently CAN socket needs to be enabled beforehand via command line

    sudo modprobe vcan
    sudo ip link add dev can0 type vcan
    sudo ip link set up can0
    */
}

double seconds_since(clock_t start)
{
    return (double)(clock() - start) / CLOCKS_PER_SEC;
}

static int16_t DrvCanSend(const CO_IF_CAN_DRV *super, CO_IF_FRM *frm)
{
    struct timespec start, end;
    struct can_frame frame = {0};
    frame.can_id = frm->Identifier;
    frame.can_dlc = frm->DLC;
    for (uint8_t i = 0; i < 8; ++i)
    {
        frame.data[i] = frm->Data[i];
    }
    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);

#ifndef NDEBUG
    // Get high-resolution time
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif
    // Perform write operation
    ssize_t bytesWritten = write(self->CanSocket, &frame, sizeof(frame));
    if (bytesWritten < 0)
    {
        perror("DrvCanSend failed");
        return errno;
    }
#ifndef NDEBUG
    // Measure elapsed time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("time req: %.9f us\n", elapsed * 1e6); // Convert to microseconds
#endif
    return sizeof(CO_IF_FRM);
}

/**
 * Send CAN frame in non-blocking mode
 * @details
 * If open a large tx buffer which ensure sufficient for 1 rt cycle, non-blocking write may always success.
 * Curret expm show no significant different with blocking version.
 */
static int16_t DrvCanNBSend(const CO_IF_CAN_DRV *super, CO_IF_FRM *frm)
{
    struct timespec start, end;
    struct can_frame frame = {0};
    frame.can_id = frm->Identifier;
    frame.can_dlc = frm->DLC;
    for (uint8_t i = 0; i < 8; ++i)
    {
        frame.data[i] = frm->Data[i];
    }

    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);
#ifndef NDEBUG
    // Get high-resolution time
    clock_gettime(CLOCK_MONOTONIC, &start);
#endif
    // Prepare to send the CAN frame
    ssize_t bytesSent = sendto(self->CanSocket, &frame, sizeof(frame), MSG_DONTWAIT, NULL, 0);
    if (bytesSent < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // Handle the case where the send would block
            printf("DrvCanNBSend: No space in buffer, data missing. Please enlarge tx buffer size\n");
            return -1; // Indicate that the send operation would block
        }
        perror("sendto");
        return errno;
    }
#ifndef NDEBUG
    // Measure elapsed time
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("time req: %.9f us\n", elapsed * 1e6); // Convert to microseconds
#endif
    return sizeof(CO_IF_FRM);
}

static int16_t DrvCanRead(const CO_IF_CAN_DRV *super, CO_IF_FRM *frm)
{
    struct can_frame frame = {0};
    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);

    // Attempt to read data from the socket
    ssize_t bytesReceived = read(self->CanSocket, &frame, sizeof(frame));
    if (bytesReceived < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // No data available, return 0 or handle as needed
            return 0;
        }
        // Handle other errors
        perror("read");
        return -1; // Return -1 to indicate an error
    }

    if (bytesReceived == 0)
    {
        // Connection closed or no data
        return 0; // Return 0 to indicate no data
    }

    // Process received data
    frm->Identifier = frame.can_id;
    frm->DLC = frame.can_dlc;
    for (uint8_t i = 0; i < 8; ++i)
    {
        frm->Data[i] = frame.data[i];
    }

    return sizeof(CO_IF_FRM); // Return size of the frame
}

static void DrvCanReset(const CO_IF_CAN_DRV *super)
{
    printf("%s\n", __func__);

    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);

    DrvCanClose(super);
    DrvCanInit(super);
}

static void DrvCanClose(const CO_IF_CAN_DRV *super)
{
    printf("%s\n", __func__);

    CO_LNX_SKTCAN *self = container_of(super, CO_LNX_SKTCAN, super);

    close(self->CanSocket);
}
