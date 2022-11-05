#include "ble/BLE.h"
#include "ble/gap/Gap.h"
#include "ble/services/HeartRateService.h"
#include "mbed-trace/mbed_trace.h"
#include "pretty_printer.h"
#include "stm32l475e_iot01_magneto.h"
#include <cstdint>
#include <cstdio>
#include <events/mbed_events.h>
#include "ButtonService.h"
#include "MagnetoService.h"

using namespace std::literals::chrono_literals;

const static char DEVICE_NAME[] = "IOT32";

static events::EventQueue event_queue(16 * EVENTS_EVENT_SIZE);

class BLEDemo : ble::Gap::EventHandler {
public:
  BLEDemo(BLE &ble, events::EventQueue &event_queue)
      : _ble(ble), _event_queue(event_queue),
        _heartrate_uuid(GattService::UUID_HEART_RATE_SERVICE),
        _heartrate_value(100),
        _heartrate_service(ble, _heartrate_value,
                           HeartRateService::LOCATION_FINGER),
        _button(BLE_BUTTON_PIN_NAME, BLE_BUTTON_PIN_PULL),
        _button_service(new ButtonService(_ble, false)), _button_uuid(ButtonService::BUTTON_SERVICE_UUID),
        _magneto_service(new MagnetoService(_ble, _magneto_value)),
        _magneto_uuid(MagnetoService::MAGNETO_SERVICE_UUID),
        _adv_data_builder(_adv_buffer) {
    BSP_MAGNETO_Init();
  }

  void start() {
    _ble.init(this, &BLEDemo::on_init_complete);

    _event_queue.dispatch_forever();
  }

private:
  /** Callback triggered when the ble initialization process has finished */
  void on_init_complete(BLE::InitializationCompleteCallbackContext *params) {
    if (params->error != BLE_ERROR_NONE) {
      printf("Ble initialization failed.");
      return;
    }

    print_mac_address();

    /* this allows us to receive events like onConnectionComplete() */
    _ble.gap().setEventHandler(this);

    /* heart rate value updated every second */
    _event_queue.call_every(1000ms, [this] {
      update_sensor_value();
      update_magneto_data();
    });

    _button.fall(Callback<void()>(this, &BLEDemo::button_pressed));
    _button.rise(Callback<void()>(this, &BLEDemo::button_released));

    start_advertising();
  }

  void start_advertising() {
    /* Create advertising parameters and payload */

    ble::AdvertisingParameters adv_parameters(
        ble::advertising_type_t::CONNECTABLE_UNDIRECTED,
        ble::adv_interval_t(ble::millisecond_t(100)));

    _adv_data_builder.setFlags();
    _adv_data_builder.setAppearance(
        ble::adv_data_appearance_t::GENERIC_HEART_RATE_SENSOR);
    _adv_data_builder.setLocalServiceList({&_heartrate_uuid, 1});
    _adv_data_builder.setName(DEVICE_NAME);

    /* Setup advertising */

    ble_error_t error = _ble.gap().setAdvertisingParameters(
        ble::LEGACY_ADVERTISING_HANDLE, adv_parameters);

    if (error) {
      printf("_ble.gap().setAdvertisingParameters() failed\r\n");
      return;
    }

    error = _ble.gap().setAdvertisingPayload(
        ble::LEGACY_ADVERTISING_HANDLE, _adv_data_builder.getAdvertisingData());

    if (error) {
      printf("_ble.gap().setAdvertisingPayload() failed\r\n");
      return;
    }

    /* Start advertising */

    error = _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

    if (error) {
      printf("_ble.gap().startAdvertising() failed\r\n");
      return;
    }

    printf("start advertising, please connect\r\n");
  }

  void update_sensor_value() {
    /* you can read in the real value but here we just simulate a value */
    _heartrate_value++;

    /*  60 <= bpm value < 110 */
    if (_heartrate_value == 110) {
      _heartrate_value = 60;
    }

    _heartrate_service.updateHeartRate(_heartrate_value);
  }

  void update_magneto_data() {
    BSP_MAGNETO_GetXYZ(_magneto_value);
    _magneto_service->updateMagnetoValue(_magneto_value);
  }

  void button_pressed(void) {
    _event_queue.call(Callback<void(bool)>(_button_service,
                                           &ButtonService::updateButtonState),
                      true);
  }

  void button_released(void) {
    _event_queue.call(Callback<void(bool)>(_button_service,
                                           &ButtonService::updateButtonState),
                      false);
  }

  /* these implement ble::Gap::EventHandler */
private:
  /* when we connect we stop advertising, restart advertising so others can
   * connect */
  virtual void onConnectionComplete(const ble::ConnectionCompleteEvent &event) {
    if (event.getStatus() == ble_error_t::BLE_ERROR_NONE) {
      printf("Client connected, you may now subscribe to updates\r\n");
    }
  }

  /* when we connect we stop advertising, restart advertising so others can
   * connect */
  virtual void
  onDisconnectionComplete(const ble::DisconnectionCompleteEvent &event) {
    printf("Client disconnected, restarting advertising\r\n");

    ble_error_t error =
        _ble.gap().startAdvertising(ble::LEGACY_ADVERTISING_HANDLE);

    if (error) {
      printf("_ble.gap().startAdvertising() failed\r\n");
      return;
    }
  }

private:
  BLE &_ble;
  events::EventQueue &_event_queue;

  UUID _heartrate_uuid;
  uint16_t _heartrate_value;
  HeartRateService _heartrate_service;

  InterruptIn _button;
  ButtonService *_button_service;
  UUID _button_uuid;

  MagnetoService *_magneto_service;
  UUID _magneto_uuid;
  int16_t _magneto_value[3] = {0};

  uint8_t _adv_buffer[ble::LEGACY_ADVERTISING_MAX_SIZE];
  ble::AdvertisingDataBuilder _adv_data_builder;
};

/* Schedule processing of events from the BLE middleware in the event queue. */
void schedule_ble_events(BLE::OnEventsToProcessCallbackContext *context) {
  event_queue.call(Callback<void()>(&context->ble, &BLE::processEvents));
}

int main() {
  mbed_trace_init();

  BLE &ble = BLE::Instance();
  ble.onEventsToProcess(schedule_ble_events);

  BLEDemo demo(ble, event_queue);
  demo.start();

  return 0;
}
