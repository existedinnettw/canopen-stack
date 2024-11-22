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

#include "drv_nvm_mmap.h"
#include <fcntl.h>    //open
#include <sys/mman.h> //mmap
#include <unistd.h>   //lseek

#include <assert.h>
#include <stdio.h>

/******************************************************************************
 * PRIVATE DEFINES
 ******************************************************************************/
#define container_of(ptr, type, member) ({                          \
        const __typeof__( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)((char *)__mptr - offsetof(type,member)); })

/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

static void DrvNvmInit(const CO_IF_NVM_DRV *super);
static uint32_t DrvNvmRead(const CO_IF_NVM_DRV *super, uint32_t start, uint8_t *buffer, uint32_t size);
static uint32_t DrvNvmWrite(const CO_IF_NVM_DRV *super, uint32_t start, uint8_t *buffer, uint32_t size);

/******************************************************************************
 * PUBLIC FUNCTIONS
 ******************************************************************************/

/**
 * @see https://stackoverflow.com/a/67499703
 */
void CONvmMmapInit(CO_NVM_MMAP *self, const char *file_path)
{
    self->super.Init = DrvNvmInit;
    self->super.Read = DrvNvmRead;
    self->super.Write = DrvNvmWrite;

    assert(file_path != NULL);
    self->file_path = (char *)file_path;
    printf("[DEBUG] file to open: %s\n", file_path);
    if (self->nvm_sim_size == 0)
    {
        self->nvm_sim_size = 128;
    }
    int memFileFd = open(file_path, O_RDWR | O_CREAT, (mode_t)0600);

    // go to the end of the desired size
    lseek(memFileFd, self->nvm_sim_size, SEEK_SET);

    // write termination character to expand it
    write(memFileFd, "", 1);

    self->NvmMemory = (uint8_t *)mmap(0, self->nvm_sim_size, PROT_READ | PROT_WRITE, MAP_SHARED, memFileFd, 0);
    assert(self->NvmMemory != MAP_FAILED);

    // we can safely close the FD now after the mem was mapped
    close(memFileFd);
}

/******************************************************************************
 * PRIVATE FUNCTIONS
 ******************************************************************************/

static void DrvNvmInit(const CO_IF_NVM_DRV *super)
{
    uint32_t idx;
    CO_NVM_MMAP *self = container_of(super, CO_NVM_MMAP, super);
    uint8_t *NvmMemory = self->NvmMemory;
    uint32_t nvm_sim_size = self->nvm_sim_size;

    for (idx = 0; idx < nvm_sim_size; idx++)
    {
        NvmMemory[idx] = 0xffu;
    }
    msync(NvmMemory, nvm_sim_size, MS_SYNC);
}

static uint32_t DrvNvmRead(const CO_IF_NVM_DRV *super, uint32_t start, uint8_t *buffer, uint32_t size)
{
    uint32_t idx = 0;
    uint32_t pos;
    CO_NVM_MMAP *self = container_of(super, CO_NVM_MMAP, super);
    uint8_t *NvmMemory = self->NvmMemory;
    uint32_t nvm_sim_size = self->nvm_sim_size;

    idx = 0;
    pos = start;
    while ((pos < nvm_sim_size) && (idx < size))
    {
        buffer[idx] = NvmMemory[pos];
        idx++;
        pos++;
    }

    return (idx);
}

static uint32_t DrvNvmWrite(const CO_IF_NVM_DRV *super, uint32_t start, uint8_t *buffer, uint32_t size)
{
    uint32_t idx = 0;
    uint32_t pos;
    CO_NVM_MMAP *self = container_of(super, CO_NVM_MMAP, super);
    uint8_t *NvmMemory = self->NvmMemory;
    uint32_t nvm_sim_size = self->nvm_sim_size;

    idx = 0;
    pos = start;
    while ((pos < nvm_sim_size) && (idx < size))
    {
        NvmMemory[pos] = buffer[idx];
        idx++;
        pos++;
    }
    msync(&NvmMemory[start], size, MS_SYNC);

    return (idx);
}
