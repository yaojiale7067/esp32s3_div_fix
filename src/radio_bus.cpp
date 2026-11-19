#include <Arduino.h>
#include <SPI.h>

#include "radio_bus.h"
#include "shared.h"

/* SPI3(HSPI) 上的公共总线：nRF24(CS=15) + CC1101(CS=5)。 */
static SPIClass s_bus(HSPI);
static bool s_started = false;

void radioBusEnsureStarted() {
  if (s_started) {
    return;
  }
  // ss 传 -1：片选由两个驱动各自用普通 GPIO 控制，不让外设接管。
  s_bus.begin(RADIO_SPI_SCK, RADIO_SPI_MISO, RADIO_SPI_MOSI, -1);
  s_started = true;
  delay(20);
}

SPIClass& radioBus() {
  radioBusEnsureStarted();
  return s_bus;
}
