#include "HumidityStore.h"

const char* HumidityStore::NVS_NAMESPACE = "humidity_store";
const char* HumidityStore::LATEST_KEY_TRACKER_NAME = "next_key";

bool HumidityStore::begin() {
  if (_is_initialized) {
    Serial.println("HumidityStore: Already initialized.");
    return true;
  }
  if (!_preferences.begin(NVS_NAMESPACE, false)) {
    Serial.println("HumidityStore: Failed to initialize NVS.");
    _is_initialized = false;
    return false;
  }
  _is_initialized = true;
  Serial.println("HumidityStore: NVS Initialized.");
  return true;
}

bool HumidityStore::write(const uint8_t data[]) {
  if (!_is_initialized) {
    Serial.println("HumidityStore: Not initialized, cannot write block.");
    return false;
  }

  uint16_t current_key_index = getNextKeyIndex();
  String nvs_key_str = String(current_key_index);

  // Stored block: [data (RH_STORE_BLOCK_SIZE)] + [crc (2 bytes)]
  size_t total_block_size = RH_STORE_BLOCK_SIZE + sizeof(uint16_t);
  uint16_t crc = CRC16(data, RH_STORE_BLOCK_SIZE);

  memcpy(_block_buffer, data, RH_STORE_BLOCK_SIZE);
  _block_buffer[RH_STORE_BLOCK_SIZE] = crc & 0xFF;
  _block_buffer[RH_STORE_BLOCK_SIZE + 1] = (crc >> 8) & 0xFF;

  size_t bytes_written_to_nvs = _preferences.putBytes(nvs_key_str.c_str(), _block_buffer, total_block_size);

  if (bytes_written_to_nvs != total_block_size) {
    Serial.printf("HumidityStore Error: Failed to write NVS block for key %s. Expected %u, wrote %u\n", nvs_key_str.c_str(), total_block_size, bytes_written_to_nvs);
    // Reset key tracker
    if (!_preferences.putUShort(LATEST_KEY_TRACKER_NAME, 0)) {
      Serial.println("HumidityStore Error: Failed to update next key tracker.");
    }
    return false;
  }

  uint16_t next_key_idx = (current_key_index + 1) % MAX_DATA_KEYS;
  if (!_preferences.putUShort(LATEST_KEY_TRACKER_NAME, next_key_idx)) {
    Serial.println("HumidityStore Error: Failed to update next key tracker.");
    return false;
  }

  Serial.printf("HumidityStore: Wrote block to key %s (CRC: 0x%04X).\n",
                nvs_key_str.c_str(), crc);
  return true;
}

int32_t HumidityStore::read(uint16_t key_index, uint8_t data[]) {

  if (!_is_initialized) {
    Serial.println("HumidityStore: Not initialized, cannot read block.");
    return -1;
  }

  String nvs_key_str = String(key_index);
  Serial.printf("HumidityStore Reading %s\n", nvs_key_str);

  size_t stored_block_size = _preferences.getBytesLength(nvs_key_str.c_str());
  if (stored_block_size == 0) {
    // Key doesn't exist or NVS error. Not an error for reading, just means no data at this key.
    return 0;
  }

  // Stored block must be at least the size of CRC + 1 byte
  if (stored_block_size <= sizeof(uint16_t)) {
    Serial.printf("HumidityStore Error: Unexpected block size for key %s (%u bytes). Corrupted?\n", nvs_key_str.c_str(), stored_block_size);
    return -2;
  }

  size_t bytes_read_from_nvs = _preferences.getBytes(nvs_key_str.c_str(), _block_buffer, stored_block_size);
  if (bytes_read_from_nvs != stored_block_size) {
    Serial.printf("HumidityStore Error: Failed to read NVS block for key %s. Expected %u, read %u\n", nvs_key_str.c_str(), stored_block_size, bytes_read_from_nvs);
    return -3;
  }

  // Reconstruct CRC (little-endian)
  uint16_t stored_crc = _block_buffer[stored_block_size - sizeof(uint16_t)] | (_block_buffer[stored_block_size - sizeof(uint16_t) + 1] << 8);

  uint16_t calculated_crc = CRC16(_block_buffer, stored_block_size - sizeof(uint16_t));

  if (calculated_crc != stored_crc) {
    Serial.printf("HumidityStore Error: CRC mismatch for key %s! Stored: 0x%04X, Calculated: 0x%04X. Data corrupted.\n",
                  nvs_key_str.c_str(), stored_crc, calculated_crc);
    return -4;
  }

  memcpy(data, _block_buffer, stored_block_size - sizeof(uint16_t));

  Serial.printf("HumidityStore: Read block from key %s (CRC: 0x%04X).\n",
                nvs_key_str.c_str(), stored_crc);
  return stored_block_size - sizeof(uint16_t);
}

bool HumidityStore::clearAll() {
  if (!_is_initialized) {
    Serial.println("HumidityStore: Not initialized, cannot clear.");
    return false;
  }
  Serial.println("WARNING: Will remove all humidity data in 10 seconds!");
  delay(10000);
  bool success = true;
  for (uint16_t i = 0; i < MAX_DATA_KEYS; ++i) {
    String nvs_key_str = String(i);
    if (_preferences.isKey(nvs_key_str.c_str())) {
      if (!_preferences.remove(nvs_key_str.c_str())) {
        Serial.printf("HumidityStore: Failed to remove key %s\n", nvs_key_str.c_str());
        success = false;
      }
    }
  }
  if (_preferences.isKey(LATEST_KEY_TRACKER_NAME)) {
    if (!_preferences.remove(LATEST_KEY_TRACKER_NAME)) {
      Serial.printf("HumidityStore: Failed to remove tracker key %s\n", LATEST_KEY_TRACKER_NAME);
      success = false;
    }
  }
  // Reset tracker to 0
  _preferences.putUShort(LATEST_KEY_TRACKER_NAME, 0);

  if (success) Serial.println("HumidityStore: All data cleared.");
  else Serial.println("HumidityStore: Partial or failed clear operation.");
  return success;
}

bool HumidityStore::isInitialized() const {
  return _is_initialized;
}

uint8_t HumidityStore::rh_encode(float rh_percent) {
  const uint16_t RH_MAX = RH_MIN + UCHAR_MAX;
  uint16_t scaled_rh = static_cast<uint16_t>(round(rh_percent * 10.0));  // e.g., 55.3% -> 553

  // Clamp to practical range, 45.0% - 70.5% by default
  if (scaled_rh < RH_MIN) {
    scaled_rh = RH_MIN;
  } else if (scaled_rh > RH_MAX) {
    scaled_rh = RH_MAX;
  }

  return static_cast<uint8_t>(scaled_rh - RH_MIN);
}

float HumidityStore::rh_decode(uint8_t rh_encoded) {
  uint16_t scaled_rh = static_cast<uint16_t>(rh_encoded) + RH_MIN;
  return static_cast<float>(scaled_rh) / 10.0f;
}

uint16_t HumidityStore::getNextKeyIndex() {
  if (!_is_initialized) {
    Serial.println("HumidityStore: Not initialized, cannot get next key index.");
    return 0;
  }
  return _preferences.getUShort(LATEST_KEY_TRACKER_NAME, 0);
}

void HumidityStore::end() {
  if (_is_initialized) {
    _preferences.end();
    _is_initialized = false;
    Serial.println("HumidityStore: NVS Ended.");
  }
}

HumidityStore::HumidityStore()
  : _is_initialized(false) {
  // Constructor
}

HumidityStore::~HumidityStore() {
  if (_is_initialized) {
    end();
  }
}

// CRC 16 implementation copied from https://github.com/espressif/arduino-esp32/blob/6bfcd6d9a9a7ecd07d656a7a88673c93e7d4d537/libraries/SD/src/sd_diskio_crc.c
const uint16_t m_CRC16Table[256] = {
  0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 0x1231, 0x0210, 0x3273,
  0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7,
  0x44A4, 0x5485, 0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 0xB75B,
  0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 0xC9CC, 0xD9ED, 0xE98E, 0xF9AF,
  0x8948, 0x9969, 0xA90A, 0xB92B, 0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B,
  0xAB1A, 0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 0x7E97, 0x6EB6,
  0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78, 0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C,
  0xC12D, 0xF14E, 0xE16F, 0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067, 0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
  0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 0x34E2, 0x24C3, 0x14A0,
  0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676,
  0x4615, 0x5634, 0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 0xCB7D,
  0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 0xFD2E, 0xED0F, 0xDD6C, 0xCD4D,
  0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9,
  0x9FF8, 0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

uint16_t CRC16(const uint8_t* data, uint16_t length) {
  uint16_t crc = 0;
  for (uint16_t i = 0; i < length; i++) {
    crc = (crc << 8) ^ m_CRC16Table[((crc >> 8) ^ data[i]) & 0x00FF];
  }
  return crc;
}