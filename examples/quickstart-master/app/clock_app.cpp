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

/******************************************************************************
 * INCLUDES
 ******************************************************************************/
#define _DEFAULT_SOURCE

/* get external node specification */
#include "clock_spec.hpp"
#include "co_core.h"
// #include "object/cia301.h"
#include "co_nmt_master.h"

#include "drv_can_socketcan.h"

#define _OPEN_THREADS
#include <pthread.h> //posix
#include <time.h>    //posix
#include <signal.h>  //posix
#ifdef __cplusplus
#include <atomic>
using namespace std;
#else
#include <stdatomic.h>
#endif
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * PRIVATE VARIABLES
 ******************************************************************************/

/* Allocate a global CANopen node object */
static CO_NODE Clk;
static volatile sig_atomic_t keep_running = 1;

/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

/* timer callback function */
static void AppClock(void *p_arg)
{
    CO_NODE *node;
    CO_OBJ *od_sec;
    CO_OBJ *od_min;
    CO_OBJ *od_hr;
    uint8_t second;
    uint8_t minute;
    uint32_t hour;

    /* For flexible usage (not needed, but nice to show), we use the argument
     * as reference to the CANopen node object. If no node is given, we ignore
     * the function call by returning immediatelly.
     */
    node = (CO_NODE *)p_arg;
    if (node == 0)
    {
        return;
    }

    /* Main functionality: when we are in operational mode, we get the current
     * clock values out of the object dictionary, increment the seconds and
     * update all clock values in the object dictionary.
     */
    if (CONmtGetMode(&node->Nmt) == CO_OPERATIONAL)
    {

        od_sec = CODictFind(&node->Dict, CO_DEV(0x2100, 3));
        od_min = CODictFind(&node->Dict, CO_DEV(0x2100, 2));
        od_hr = CODictFind(&node->Dict, CO_DEV(0x2100, 1));

        COObjRdValue(od_sec, node, (void *)&second, sizeof(second));
        COObjRdValue(od_min, node, (void *)&minute, sizeof(minute));
        COObjRdValue(od_hr, node, (void *)&hour, sizeof(hour));

        second++;
        if (second >= 60)
        {
            second = 0;
            minute++;
        }
        if (minute >= 60)
        {
            minute = 0;
            hour++;
        }

        COObjWrValue(od_hr, node, (void *)&hour, sizeof(hour));
        COObjWrValue(od_min, node, (void *)&minute, sizeof(minute));
        COObjWrValue(od_sec, node, (void *)&second, sizeof(second));

        uint16_t status_wd = 0;
        COObjRdValue(CODictFind(&node->Dict, CO_DEV(0x2200, 0)), node, (void *)&status_wd, sizeof(status_wd));
        printf("status wd: 0x%04x\n", status_wd);
        printf("my node mode:%d \n", node->Nmt.Mode);
    }
    printf("app clock called\n");
}

static void sig_handler(int _)
{
    (void)_;
    keep_running = 0;
}

struct timespec
timespec_add(struct timespec time1, struct timespec time2)
{
    struct timespec result;
    const uint32_t NSEC_PER_SEC = 1e9;
    if ((time1.tv_nsec + time2.tv_nsec) >= NSEC_PER_SEC)
    {
        result.tv_sec = time1.tv_sec + time2.tv_sec + 1;
        result.tv_nsec = time1.tv_nsec + time2.tv_nsec - NSEC_PER_SEC;
    }
    else
    {
        result.tv_sec = time1.tv_sec + time2.tv_sec;
        result.tv_nsec = time1.tv_nsec + time2.tv_nsec;
    }
    return result;
}

/**
 * @brief
 * simulation of realtime callback.
 */
void *rt_cb(void *args)
{
    CO_NODE *Node = (CO_NODE *)args;
    struct timespec wakeup_time;
    struct timespec rem = {0};

    // APP_TICKS_PER_SEC should match following period config.
    __syscall_slong_t tv_nsec = 1e6; // 1 ms
    clock_gettime(CLOCK_MONOTONIC, &wakeup_time);
    wakeup_time = timespec_add(wakeup_time, (struct timespec){.tv_sec = 0, .tv_nsec = tv_nsec});

    while (keep_running)
    {
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeup_time, NULL))
        {
            printf("rt thread exit\n");
            pthread_exit((void *)EXIT_FAILURE);
        }
        COTmrService(&Node->Tmr);
        COTmrProcess(&Clk.Tmr);
        CONodeProcess(&Clk);
        wakeup_time = timespec_add(wakeup_time, (struct timespec){.tv_sec = 0, .tv_nsec = tv_nsec});
    }
    printf("rt thread exit success\n");
    pthread_exit((void *)EXIT_SUCCESS);
}

void handle_error_en(int err, const char *msg)
{
    errno = err;
    printf("[ERROR] %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

void TS_AppCSdoCallback(CO_CSDO *csdo, uint16_t index, uint8_t sub, uint32_t code)
{
    // checkout Table 22: SDO abort codes, CO_SDO_ERR_OBJ
    // The stack not yet implement response handle yet
    // can refer to `COSdoGetObject` and `igh::ecrt_sdo_request_state, ec_sdo_request_t`
    // check COCSdoTransferFinalize for calling detail

    printf("[DEBUG] sdo received response\n");
    printf("[DEBUG] index: 0x%04x, sub: 0x%02x, code: 0x%08x\n", index, sub, code);

    switch (code)
    {
    case 0:
        printf("[DEBUG] SDO response successy\n");
        // store the value
        break;
    // decode based on abort code
    case CO_SDO_ERR_OBJ:
        printf("[WARN] Object doesn't exist in target dictionary\n");
        break;
    case CO_SDO_ERR_WR:
        printf("[WARN] Attempt to write a read only object\n");
        break;
    case CO_SDO_ERR_OBJ_MAP_N:
        printf("[WARN] Number and length exceed PDO\n");
        break;
    case CO_SDO_ERR_DATA_LOCAL:
        printf("[WARN] Data cannot be transferred or stored to the application because of local control\n");
        break;
    default:
        printf("[WARN] general abort sdo request\n");
        break;
    }
}

void COPdoTransmit(CO_IF_FRM *frm){
    printf("[DEBUG] tpdo will transmit\n");
}

/******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

/* main entry function */
int main(void)
{
    uint32_t ticks;

    /* Initialize your hardware layer */
    /* BSPInit(); */
    signal(SIGINT, sig_handler);

    /* Initialize the CANopen stack. Stop execution if an
     * error is detected.
     */
    CO_LNX_SKTCAN Linux_Socketcan_CanDriver;
    COLnxSktCanInit(&Linux_Socketcan_CanDriver, "can0");
    AppSpec.Drv->Can = &Linux_Socketcan_CanDriver.super;

    CONodeInit(&Clk, &AppSpec);
    CO_ERR err = CONodeGetErr(&Clk);
    if (err != CO_ERR_NONE)
    {
        printf("[DEBUG] co node init failed with code: %d\n", err);
        exit(EXIT_FAILURE);
    }

    /* Use CANopen software timer to create a cyclic function
     * call to the callback function 'AppClock()' with a period
     * of 1s (equal: 1000ms).
     */
    ticks = COTmrGetTicks(&Clk.Tmr, 1000, CO_TMR_UNIT_1MS);
    COTmrCreate(&Clk.Tmr, 0, ticks, AppClock, &Clk);

    /* Start the CANopen node and set it automatically to
     * NMT mode: 'OPERATIONAL'.
     */
    CONodeStart(&Clk);

    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param sched;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &sched);
    int ret = pthread_create(&thread, &attr, rt_cb, (void *)&Clk);
    if (ret != 0)
    {
        handle_error_en(ret, "pthread_create");
    }

    // config other nodes
    CO_CSDO *csdo;
    csdo = COCSdoFind(&Clk, 0x0);
    if (csdo == 0)
    {
        printf("SDO client #0 is missing or busy\n");
    }
    else
    {
        printf("Your sdo spec setting correct. SDO client #0 is usable\n");
    }
    // printf("[DEBUG] sending sdo request to NodeId: %u\n", csdo->NodeId); //access nodeId is wrong

    uint32_t timeout = 5000;
    err = CO_ERR_NONE;

    // read sync message by 0x1005
    uint32_t other_node_0x1005_val = CO_SYNC_COBID_ON | 0x80;
    // while ((err = COCSdoRequestUpload(csdo,
    //                                   CO_DEV(0x1005, 0),
    //                                   (uint8_t *)&other_node_0x1005_val, sizeof(other_node_0x1005_val),
    //                                   TS_AppCSdoCallback,
    //                                   timeout)) != CO_ERR_NONE)
    // {
    // }

    // set 0x1005, request send sync message
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1005, 0x00),
                                        (uint8_t *)&other_node_0x1005_val, sizeof(other_node_0x1005_val),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        // error handle
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    };
    printf("[INFO] sdo send 0x1005 success\n");

    // set node heart beat 0x1017 by sdo
    uint16_t val = 1000; // 1sec

    // write Producer heartbeat time
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1017, 0x00),
                                        (uint8_t *)&val, sizeof(val),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        // error handle
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    };
    printf("[INFO] sdo send 0x1017 success\n");
    // how to get response?

    // config pdo by sdo
    printf("[DEBUG] set PREOP\n");
    CO_NMT_sendCommand(&Clk, CO_NMT_ENTER_PRE_OPERATIONAL, 0x02);

    // --- config pdo mapping
    uint8_t txpdo_trans_type = 0x01;
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1800, 0x02),
                                        (uint8_t *)&txpdo_trans_type, sizeof(txpdo_trans_type),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        /* code */
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    }
    printf("[INFO] sdo send 0x180002 success\n");

    // close pdo mapping before setting
    uint8_t num_mapped_TPDO = 0;
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1A00, 0x00),
                                        (uint8_t *)&num_mapped_TPDO, sizeof(num_mapped_TPDO),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        /* code */
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    }
    printf("[INFO] sdo send 0x1A0000 success\n");

    uint32_t app_obj_map = CO_LINK(0x6041, 0x00, 16);
    // app_obj_map = CO_LINK(0x606C, 0x00, 32);
    // app_obj_map = CO_LINK(0x60FD, 0x00, 32);
    // app_obj_map = CO_LINK(0x6064, 0x00, 32);
    printf("[DEBUG] prepare for 0x1A0001 map: 0x%08x\n", app_obj_map);
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1A00, 0x01),
                                        (uint8_t *)&app_obj_map, sizeof(app_obj_map),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        /* code */
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    }
    printf("[INFO] sdo send 0x1A0001 success\n");

    // restart to pdo mapping
    num_mapped_TPDO = 1; // 1 mapped TPDO
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1A00, 0x00),
                                        (uint8_t *)&num_mapped_TPDO, sizeof(num_mapped_TPDO),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        /* code */
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    }
    printf("[INFO] sdo send 0x1A0000 success\n");

    //
    uint8_t rxpdo_trans_type = 0x00; // FE
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1400, 0x02),
                                        (uint8_t *)&rxpdo_trans_type, sizeof(rxpdo_trans_type),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        /* code */
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    }
    printf("[INFO] sdo send 0x140002 success\n");
    // close pdo mapping before setting
    uint8_t num_mapped_RPDO = 0;
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1600, 0x00),
                                        (uint8_t *)&num_mapped_RPDO, sizeof(num_mapped_RPDO),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        /* code */
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    }
    printf("[INFO] sdo send 0x160000 success\n");

    app_obj_map = CO_LINK(0x6040, 0x00, 16);
    // app_obj_map = CO_LINK(0x6042, 0x00, 16);
    // app_obj_map = CO_LINK(0x607A, 0x00, 32);
    // app_obj_map = CO_LINK(0x6081, 0x00, 32);
    printf("[DEBUG] prepare for 0x160001 map: 0x%08x\n", app_obj_map);
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1600, 0x01),
                                        (uint8_t *)&app_obj_map, sizeof(app_obj_map),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        /* code */
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    }
    printf("[INFO] sdo send 0x160001 success\n");

    // restart to pdo mapping
    num_mapped_RPDO = 1; // 1 mapped TPDO
    while ((err = COCSdoRequestDownload(csdo,
                                        CO_DEV(0x1600, 0x00),
                                        (uint8_t *)&num_mapped_RPDO, sizeof(num_mapped_RPDO),
                                        TS_AppCSdoCallback,
                                        timeout)) != CO_ERR_NONE)
    {
        /* code */
        switch (err)
        {
        case CO_ERR_SDO_BUSY:
            printf("[INFO] busy\n");
            break;
        default:
            printf("[ERROR] sdo client download error: %d\n", (int)err);
            exit(EXIT_FAILURE);
        }
        struct timespec ts1 = {
            .tv_sec = 0,
            .tv_nsec = static_cast<__syscall_slong_t>(20e6)};
        nanosleep(&ts1, NULL);
    }
    printf("[INFO] sdo send 0x160000 success\n");
    //

    // ==================================================local pdo map set============================================================

    // NMT OP
    CO_NMT_sendCommand(&Clk, CO_NMT_ENTER_OPERATIONAL, 0x02);

    CONmtSetMode(&Clk.Nmt, CO_OPERATIONAL);

    void *status;
    pthread_join(thread, &status);
    return (int)(intptr_t)status;
}
