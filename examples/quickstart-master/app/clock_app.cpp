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
#include "co_core.h"
// #include "object/cia301.h"
#include "co_nmt_master.h"
#include "co_master.hpp"

#include "drv_can_socketcan.h"
#include "drv_timer_swcycle.h" /* Timer driver                */
#include "drv_nvm_mmap.h"      /* NVM driver                  */

#define _OPEN_THREADS
#include <pthread.h> //posix
#include <time.h>    //posix
#include <signal.h>  //posix

#ifdef __cplusplus
// #include <atomic>
#include <csignal>
using std::sig_atomic_t;
#else
#include <stdatomic.h>
#endif

#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <filesystem>

/******************************************************************************
 * PRIVATE VARIABLES
 ******************************************************************************/

/* Allocate a global CANopen node object */
static CO_NODE master_node;
Master_node *master_ptr;
static volatile sig_atomic_t keep_running = 1;

/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

PDO_data_map pdo_data_map;

/* timer callback function */
static void AppClock(void *p_arg)
{
    CO_NODE *node;

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
    }

    // printf("\n");
    print_od(node->Dict.Root, 0x1000, 0x2000);
    printf("---\n");
    print_data_map(pdo_data_map);
    // printf("status wd: 0x%x\n", *(uint16_t *)(pdo_data_map[2][0x604100]));
    // CO_ERR err = CONodeGetErr(&master_node);
    // if (err != CO_ERR_NONE)
    // {
    //     printf("[DEBUG] co node failed with code: %d\n", err);
    //     exit(EXIT_FAILURE);
    // }
    printf("app clock called, my state:%d\n", node->Nmt.Mode);
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

    uint16_t tick = 0;
    while (keep_running)
    {
        if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeup_time, NULL))
        {
            printf("rt thread exit\n");
            pthread_exit((void *)EXIT_FAILURE);
        }
        {
            // following is 1ms callback
            CONodeProcess(&master_node);
            COTmrService(&Node->Tmr);
            COTmrProcess(&master_node.Tmr);
            CONodeProcess(&master_node);
            {
                // user app logic following
                if (tick >= 500)
                {
                    printf("node 3 hb mode2: %d\n", master_ptr->get_slave(3).get_NMT_state());

                    tick = 0; // reset
                }
                tick++;
            }
            // sleep until next tick
        }
        wakeup_time = timespec_add(wakeup_time, (struct timespec){.tv_sec = 0, .tv_nsec = tv_nsec});
    }
    printf("\nrt thread exit success\n");
    return 0;
}

void handle_error_en(int err, const char *msg)
{
    errno = err;
    printf("[ERROR] %s\n", msg);
    exit(EXIT_FAILURE);
}

// void COPdoTransmit(CO_IF_FRM *frm){
//     printf("[DEBUG] tpdo will transmit\n");
// }

static void sig_handler(int _)
{
    (void)_;
    keep_running = 0;
}

/******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

Slave_model_configs slaveConfigs = {
    {
        2,
        {
            {0x1600, {{std::make_tuple(0x604000, 2), std::make_tuple(0x604200, 2)}}},
            {0x1A00, {{std::make_tuple(0x604100, 2), std::make_tuple(0x606C00, 4)}}},
            {0x1A01, {{std::make_tuple(0x60FD00, 4), std::make_tuple(0x606400, 4)}}},
            // {0x1A01, {}}, // disable only
            {0x1A02, {}},
            {0x1A03, {}},
        },
    },
    {
        3,
        {
            {0x1600, {{std::make_tuple(0x604000, 2), std::make_tuple(0x604200, 2)}}},
            {0x1A00, {{std::make_tuple(0x604100, 2), std::make_tuple(0x606C00, 4)}}},
            {0x1A01, {{std::make_tuple(0x60FD00, 4), std::make_tuple(0x606400, 4)}}},
            // {0x1A01, {}}, // disable only
            {0x1A02, {}},
            {0x1A03, {}},
        },
    },
};
#define APP_NODE_ID 1u                 /* CANopen node ID             */
#define APP_BAUDRATE 1000000u          /* CAN baudrate                */
#define APP_TMR_N (8 * CO_CSDO_N + 16) /* Number of software timers   */
#define APP_TICKS_PER_SEC 1000u        /* Timer clock frequency in Hz */

static CO_TMR_MEM TmrMem[APP_TMR_N];

static uint8_t SdoSrvMem[CO_SSDO_N * CO_SDO_BUF_BYTE];
enum EMCY_CODES
{
    APP_ERR_ID_SOMETHING = 0,
    APP_ERR_ID_HOT,

    APP_ERR_ID_NUM /* number of EMCY error codes in application */
};
static CO_EMCY_TBL AppEmcyTbl[APP_ERR_ID_NUM] = {
    {CO_EMCY_REG_GENERAL, CO_EMCY_CODE_GEN_ERR},      /* APP_ERR_CODE_SOMETHING */
    {CO_EMCY_REG_TEMP, CO_EMCY_CODE_TEMP_AMBIENT_ERR} /* APP_ERR_CODE_HAPPENS   */
};

/* main entry function */
int main(void)
{
    /* Initialize your hardware layer */
    /* BSPInit(); */
    signal(SIGINT, sig_handler);

    std::vector<CO_OBJ_T> od = create_default_od();
    Imd_PDO_data_map imd_pdo_data_map = config_od_through_configs(od, slaveConfigs);
    pdo_data_map = get_PDO_data_map(od, imd_pdo_data_map);
    od.push_back(CO_OBJ_DICT_ENDMARK);

    CO_LNX_SKTCAN Linux_Socketcan_CanDriver;
    COLnxSktCanInit(&Linux_Socketcan_CanDriver, "can0");

    CO_NVM_MMAP mmap_nvm_driver{.nvm_sim_size = 128};
    std::filesystem::path nvm_fs_path = std::filesystem::current_path() / "canopen_nvm.bin";
    CONvmMmapInit(&mmap_nvm_driver, nvm_fs_path.c_str());

    static struct CO_IF_DRV_T AppDriver = {
        &Linux_Socketcan_CanDriver.super,
        &SwCycleTimerDriver,
        &mmap_nvm_driver.super};

    struct CO_NODE_SPEC_T AppSpec = {
        APP_NODE_ID,                      /* default Node-Id                */
        APP_BAUDRATE,                     /* default Baudrate               */
        od.data(),                        /* pointer to object dictionary   */
        static_cast<uint16_t>(od.size()), /* object dictionary max length   */
        &AppEmcyTbl[0],                   /* EMCY code & register bit table */
        &TmrMem[0],                       /* pointer to timer memory blocks */
        APP_TMR_N,                        /* number of timer memory blocks  */
        APP_TICKS_PER_SEC,                /* timer clock frequency in Hz    */
        &AppDriver,                       /* select drivers for application */
        &SdoSrvMem[0]                     /* SDO Transfer Buffer Memory     */
    };

    CONodeInit(&master_node, &AppSpec); // to INIT mode
    CO_ERR err = CONodeGetErr(&master_node);
    if (err != CO_ERR_NONE)
    {
        printf("[DEBUG] co node init failed with code: %d\n", err);
        exit(EXIT_FAILURE);
    }

    // tmr since have bug, if loading large in pdo
    COTmrCreate(&master_node.Tmr, 0, COTmrGetTicks(&master_node.Tmr, 1000, CO_TMR_UNIT_1MS), AppClock, &master_node);

    print_od(master_node.Dict.Root);

    Master_node master = Master_node(master_node);
    master_ptr = &master;
    master.set_slaves_NMT_mode(CO_NMT_RESET_NODE);
    master.start_config();

    auto slave_mot1 = Slave_node_model(0x02, master_node, BYPASS);
    auto slave_mot2 = Slave_node_model(0x03, master_node, HEARTBEAT); // HEARTBEAT

    master.bind_slave(slave_mot1); // todo: may need mapping
    master.bind_slave(slave_mot2);
    master.start_config_slaves();

    master.config_nmt_monitors();
    master.config_pdo_mappings(slaveConfigs);
    // exit(EXIT_SUCCESS);

    master.activate(1000);

    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param sched;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &sched);
    int ret = pthread_create(&thread, &attr, rt_cb, (void *)&master_node);
    if (ret != 0)
    {
        printf("[ERROR] pthread_create error, you may need to offer sudo privilege.\n");
        handle_error_en(ret, "pthread_create");
    }

    // print_od(master_node.Dict.Root);

    void *status;
    pthread_join(thread, &status);

    master.start_config_slaves();
    master.start_config();
    master.release();
    printf("program end\n");

    return (int)(intptr_t)status;
}
