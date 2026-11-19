#ifndef RADIO_BUS_H
#define RADIO_BUS_H

#include <SPI.h>

/* nRF24 与 CC1101 共用同一条 SPI3(HSPI) 总线：
 *   SCK / MISO / MOSI 三个脚相同，各自有独立的 CSN 片选。
 * SPIClass 的引脚映射是一次性绑定的，所以两条驱动必须拿到同一个实例，
 * 否则第二次 begin() 会重新复位 SPI3 外设，把正在通信的那一路打断。
 * 每次寄存器访问各自包 beginTransaction/endTransaction，靠同一把 lock 串行化。 */
SPIClass& radioBus();
void radioBusEnsureStarted();

#endif  // RADIO_BUS_H
