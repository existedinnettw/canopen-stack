#include "co_size_only.h"
#include "assert.h"

CO_OBJ_TYPE*
size_to_obj_type(uint8_t size)
{
  switch (size) {
    case 1:
      return (CO_OBJ_TYPE*)CO_SIZE_1;
      break;
    case 2:
      return (CO_OBJ_TYPE*)CO_SIZE_2;
      break;
    case 4:
      return (CO_OBJ_TYPE*)CO_SIZE_4;
      break;
    default:
      assert(false);
  }
}