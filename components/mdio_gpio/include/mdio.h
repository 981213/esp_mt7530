#ifndef _MDIO_H_
#define _MDIO_H_
#include "freertos/FreeRTOS.h"

#define MDIO_READ 2
#define MDIO_WRITE 1

#define MDIO_C45 (1 << 15)
#define MDIO_C45_ADDR (MDIO_C45 | 0)
#define MDIO_C45_READ (MDIO_C45 | 3)
#define MDIO_C45_WRITE (MDIO_C45 | 1)
#define MII_ADDR_C45 (1 << 30)

#define BIT(nr)			(1UL << (nr))
#define BIT_ULL(nr)		(1ULL << (nr))

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
void set_mdc(int val);
void set_mdio_data(int val);
void set_mdio_dir(int dir);
int get_mdio_data(void);

void mdio_gpio_init(void);
int mdio_read(int phy, int reg);
int mdio_write(int phy, int reg, u16 val);
#endif