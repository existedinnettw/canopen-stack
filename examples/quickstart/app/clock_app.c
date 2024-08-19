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

/* get external node specification */
#include "clock_spec.h"

#include "drv_can_socketcan.h"

#define _OPEN_THREADS
#include <pthread.h>   //posix
#include <time.h>      //posix
#include <signal.h>    //posix
#include <stdatomic.h> //C11
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>

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
    CO_NODE  *node;
    CO_OBJ   *od_sec;
    CO_OBJ   *od_min;
    CO_OBJ   *od_hr;
    uint8_t   second;
    uint8_t   minute;
    uint32_t  hour;

    /* For flexible usage (not needed, but nice to show), we use the argument
     * as reference to the CANopen node object. If no node is given, we ignore
     * the function call by returning immediatelly.
     */
    node = (CO_NODE *)p_arg;
    if (node == 0) {
        return;
    }

    /* Main functionality: when we are in operational mode, we get the current
     * clock values out of the object dictionary, increment the seconds and
     * update all clock values in the object dictionary.
     */
    if (CONmtGetMode(&node->Nmt) == CO_OPERATIONAL) {

        od_sec = CODictFind(&node->Dict, CO_DEV(0x2100, 3));
        od_min = CODictFind(&node->Dict, CO_DEV(0x2100, 2));
        od_hr  = CODictFind(&node->Dict, CO_DEV(0x2100, 1));

        COObjRdValue(od_sec, node, (void *)&second, sizeof(second));
        COObjRdValue(od_min, node, (void *)&minute, sizeof(minute));
        COObjRdValue(od_hr , node, (void *)&hour  , sizeof(hour)  );

        second++;
        if (second >= 60) {
            second = 0;
            minute++;
        }
        if (minute >= 60) {
            minute = 0;
            hour++;
        }

        COObjWrValue(od_hr , node, (void *)&hour  , sizeof(hour)  );
        COObjWrValue(od_min, node, (void *)&minute, sizeof(minute));
        COObjWrValue(od_sec, node, (void *)&second, sizeof(second));
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
  if ((time1.tv_nsec + time2.tv_nsec) >= NSEC_PER_SEC) {
    result.tv_sec = time1.tv_sec + time2.tv_sec + 1;
    result.tv_nsec = time1.tv_nsec + time2.tv_nsec - NSEC_PER_SEC;
  } else {
    result.tv_sec = time1.tv_sec + time2.tv_sec;
    result.tv_nsec = time1.tv_nsec + time2.tv_nsec;
  }
  return result;
}

/**
 * @brief
 * simulation of realtime callback.
 */
void* rt_cb(void *args)
{
    CO_NODE *Node = (CO_NODE *)args;
    struct timespec wakeup_time;
    struct timespec rem = {0};

    // APP_TICKS_PER_SEC should match following period config.
    __syscall_slong_t tv_nsec=1e6; // 1 ms
    clock_gettime(CLOCK_MONOTONIC, &wakeup_time);
    wakeup_time = timespec_add(wakeup_time, (struct timespec){ .tv_sec=0, .tv_nsec=tv_nsec });

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
        wakeup_time = timespec_add(wakeup_time, (struct timespec){ .tv_sec=0, .tv_nsec=tv_nsec });
    }
    printf("rt thread exit success\n");
    pthread_exit((void *)EXIT_SUCCESS);
}

void handle_error_en(int err, const char *msg) {
    errno = err;
    perror(msg);
    exit(EXIT_FAILURE);
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
    if (CONodeGetErr(&Clk) != CO_ERR_NONE)
    {
        return EXIT_FAILURE;
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
    CONmtSetMode(&Clk.Nmt, CO_OPERATIONAL);

    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param sched;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &sched);
    int ret = pthread_create(&thread, &attr, rt_cb, (void *)&Clk);
    if (ret != 0) {
        handle_error_en(ret, "pthread_create");
    }

    void *status;
    pthread_join(thread, &status);
    return (int)(intptr_t)status;
}
