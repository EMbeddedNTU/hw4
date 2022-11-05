/* mbed Microcontroller Library
 * Copyright (c) 2006-2013 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __BLE_MAGNETO_SERVICE_H__
#define __BLE_MAGNETO_SERVICE_H__

#include <cstdint>
#include <stdint.h>
class MagnetoService {
public:
  const static uint16_t MAGNETO_SERVICE_UUID = 0xA002;
  const static uint16_t MAGNETO_X_CHARACTERISTIC_UUID = 0xA003;
  const static uint16_t MAGNETO_Y_CHARACTERISTIC_UUID = 0xA004;
  const static uint16_t MAGNETO_Z_CHARACTERISTIC_UUID = 0xA005;

  MagnetoService(BLE &_ble, int16_t magnetoValueInitial[])
      : ble(_ble),
        magnetoX(MAGNETO_X_CHARACTERISTIC_UUID, &magnetoValueInitial[0],
                 GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
        magnetoY(MAGNETO_Y_CHARACTERISTIC_UUID, &magnetoValueInitial[1],
                 GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY),
        magnetoZ(MAGNETO_Z_CHARACTERISTIC_UUID, &magnetoValueInitial[2],
                 GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY) {
    GattCharacteristic *charTable[] = {&magnetoX, &magnetoY, &magnetoZ};
    GattService magnetoService(MagnetoService::MAGNETO_SERVICE_UUID, charTable,
                               sizeof(charTable) /
                                   sizeof(GattCharacteristic *));
    ble.gattServer().addService(magnetoService);
  }

  void updateMagnetoValue(int16_t newValue[]) {
    ble.gattServer().write(magnetoX.getValueHandle(), (uint8_t *)&newValue[0],
                           sizeof(int16_t));
    ble.gattServer().write(magnetoY.getValueHandle(), (uint8_t *)&newValue[1],
                           sizeof(int16_t));
    ble.gattServer().write(magnetoZ.getValueHandle(), (uint8_t *)&newValue[2],
                           sizeof(int16_t));
  }

private:
  BLE &ble;
  ReadOnlyGattCharacteristic<int16_t> magnetoX;
  ReadOnlyGattCharacteristic<int16_t> magnetoY;
  ReadOnlyGattCharacteristic<int16_t> magnetoZ;
};

#endif /* #ifndef __BLE_MAGNETO_SERVICE_H__ */
