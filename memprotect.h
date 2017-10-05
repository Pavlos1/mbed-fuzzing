#include <stdbool.h>

#include "controller.h"
#include "stdint.h"

#define MEM_MANAGE_ISR_ADDRESS 0x10
#define MPU_TYPE_ADDRESS 0xE000ED90
#define MPU_CTRL_ADDRESS 0xE000ED94
#define MPU_RBAR_ADDRESS 0xE000ED9C
#define MPU_RASR_ADDRESS 0xE000EDA0

#define GPIOE_ADDRESS 0x48001000

bool enable_memory_protection(ExecStatus * stat);
