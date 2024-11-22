#ifndef CO_NVM_MMAP_H_
#define CO_NVM_MMAP_H_

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
     * @note
     * this implement is not well tested yet
     * @todo unittest
     */
    typedef struct CO_NVM_MMAP_T CO_NVM_MMAP;
    struct CO_NVM_MMAP_T
    {
        CO_IF_NVM_DRV super; // super
        char *file_path;

        /**
         * @private
         */
        uint32_t nvm_sim_size; // byte
        uint8_t *NvmMemory;
    };

    /**
     * @param[in] file_path should be
     */
    void CONvmMmapInit(CO_NVM_MMAP *self, const char *file_path);

#ifdef __cplusplus /* for compatibility with C++ environments  */
}
#endif

#endif
