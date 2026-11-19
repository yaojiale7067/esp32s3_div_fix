#ifndef SUBGHZ_H
#define SUBGHZ_H

/* CC1101 Sub-GHz 三个功能（原 ESP32-DIV 的 SubGHz 菜单）：
 *   Replay Attack —— 录制 433/315MHz 遥控码并存档回放
 *   SubGHz Jammer —— 定频/扫频载波
 *   Saved Profile —— 已保存遥控码的管理与发射
 * 硬件：CC1101 与 nRF24 共用 SPI3（SCK16/MISO17/MOSI18），CSN=5，GDO0=6。 */
namespace replayat {
void ReplayAttackSetup();
void ReplayAttackLoop();
}  // namespace replayat

namespace subjammer {
void subjammerSetup();
void subjammerLoop();
}  // namespace subjammer

namespace SavedProfile {
void saveSetup();
void saveLoop();
}  // namespace SavedProfile

#endif  // SUBGHZ_H
