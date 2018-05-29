
#include <stdint.h>
#include <string.h>

#include "sys-gnu-linux.h"

const uint8_t *unique_device_id (void)
{
  /*
   * STM32F103 has 96-bit unique device identifier.
   * This routine mimics that.
   */

  static const uint8_t id[] = { /* My RSA fingerprint */
    0x12, 0x41, 0x24, 0xBD, 0x3B, 0x48, 0x62, 0xAF,
    0x7A, 0x0A, 0x42, 0xF1, 0x00, 0xB4, 0x5E, 0xBD,
    0x4C, 0xA7, 0xBA, 0xBE
  };

  return id;
}
