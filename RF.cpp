#include <Arduino.h>
#include <RH_ASK.h>
#include "RF.h"

namespace {
constexpr uint8_t TX_PIN = 12;
constexpr uint8_t UNUSED_RX_PIN = 11;
constexpr uint8_t UNUSED_PTT_PIN = 10;
constexpr uint16_t BIT_RATE = 2000;
constexpr uint32_t SEND_INTERVAL_MS = 500;

RH_ASK radio(BIT_RATE, UNUSED_RX_PIN, TX_PIN, UNUSED_PTT_PIN);
bool ready = false;
uint32_t lastSendMs = 0;

// AVR wire format: little-endian integers and IEEE 754 binary32 floats.
// Field order matches the USB CSV. RadioHead supplies framing and CRC.
static_assert(sizeof(float) == 4 && sizeof(TelemetrySample) == 40,
              "Unexpected telemetry packet layout");
}

bool beginRF() {
  ready = radio.init();
  return ready;
}

void sendRF(const TelemetrySample &sample) {
  const uint32_t now = millis();
  if (!ready || now - lastSendMs < SEND_INTERVAL_MS) {
    return;
  }

  // RH_ASK copies the packet before returning. Timer1 sends it in the background.
  if (radio.send(reinterpret_cast<const uint8_t *>(&sample), sizeof(sample))) {
    lastSendMs = now;
  }
}
