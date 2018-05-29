#include <stdint.h>
#include <string.h>

#include <rtthread.h>

#include "random.h"

#ifdef RT_USING_FINSH
#include <finsh.h>

#define RANDOM_BYTES_LENGTH 32

void generate_random()
{
  uint8_t random_bytes[RANDOM_BYTES_LENGTH];
  uint8_t i;

  rt_kprintf("-------------------------------\n");
  for(i=0;i<(RANDOM_BYTES_LENGTH/8);i++)
  {
    random_get_salt (&random_bytes[0]);
    rt_kprintf("%08x ",*((uint32_t *)&random_bytes[0]));
    rt_kprintf("%08x ",*((uint32_t *)&random_bytes[4]));
  }
  rt_kprintf("\n");
  rt_kprintf("-------------------------------\n");

}

MSH_CMD_EXPORT(generate_random, generate random);

#endif

