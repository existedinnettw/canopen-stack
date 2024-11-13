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

#ifndef CO_SIZE_ONLY_H_
#define CO_SIZE_ONLY_H_

#ifdef __cplusplus               /* for compatibility with C++ environments  */
extern "C" {
#endif

/******************************************************************************
* INCLUDES
******************************************************************************/

#include "co_integer32.h"
#include "co_integer16.h"
#include "co_integer8.h"

/******************************************************************************
* DEFINES
******************************************************************************/

#define CO_TREAL32 CO_TUNSIGNED32 //should be same

#define CO_SIZE_4 CO_TUNSIGNED32
#define CO_SIZE_2 CO_TUNSIGNED16
#define CO_SIZE_1 CO_TUNSIGNED8

/******************************************************************************
* PUBLIC CONSTANTS
******************************************************************************/

// no new type declare

/******************************************************************************
* PUBLIC FUNCTION
******************************************************************************/

CO_OBJ_TYPE* size_to_obj_type(uint8_t size);

#ifdef __cplusplus               /* for compatibility with C++ environments  */
}
#endif

#endif  /* #ifndef CO_SIZE_ONLY_H_ */
