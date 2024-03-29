/*! @file
  @brief
  Object, Proc, Nil, True and False class.

  <pre>
  Copyright (C) 2015-2023 Kyushu Institute of Technology.
  Copyright (C) 2015-2023 Shimane IT Open-Innovation Center.

  This file is distributed under BSD 3-Clause License.

  </pre>
*/

#ifndef MRBC_SRC_OBJECT_H_
#define MRBC_SRC_OBJECT_H_

/***** Feature test switches ************************************************/
/***** System headers *******************************************************/
/***** Local headers ********************************************************/
/***** Constant values ******************************************************/
/***** Macros ***************************************************************/
/***** Typedefs *************************************************************/
/***** Global variables *****************************************************/
/***** Function prototypes **************************************************/
#ifdef __cplusplus
extern "C" {
#endif

struct VM;
struct RObject;

void mrbc_instance_call_initialize(struct VM *vm, struct RObject v[], int argc);


/***** Inline functions *****************************************************/


#ifdef __cplusplus
}
#endif
#endif
