#ifndef HUMIDITY_STORE_H
#define HUMIDITY_STORE_H


#include <math.h>  // For round
#include <Arduino.h>
#include <Preferences.h>

uint16_t CRC16(const uint8_t* data, uint16_t length);

class HumidityStore {
public:
  HumidityStore();
  ~HumidityStore();

  /**
     * @brief Initializes the NVS storage for humidity data. Call this once.
     * @return true if initialization was successful, false otherwise.
     */
  bool begin();

  /**
     * @brief Closes the NVS storage. Call this when done or before deep sleep.
     */
  void end();

  /**
     * @brief Writes an array of relative humidity values to NVS.
     *        Data is stored with a CRC32 checksum.
     *        Automatically handles key rollover.
     *
     * @param data Array of rH values to be written.
     * @return true if the data was successfully written, false otherwise.
     */
  bool write(const uint8_t data[]);

  /**
     * @brief Reads an array of relative humidity values from NVS.
     *        Verifies data integrity using the stored CRC32 checksum.
     *
     * @param key_index The index of the key to read (0 to MAX_DATA_KEYS - 1).
     * @param data Array that will store the read rH values.
     * @return true if the data was successfully read and CRC is valid, false otherwise.
     */
  bool read(uint16_t key_index, uint8_t data[]);

  uint8_t rh_encode(float rh_percent);

  float rh_decode(uint8_t rh_encoded);

  /**
     * @brief Gets the index of the next key that will be written.
     *        This handles the rollover.
     * @return The next key index (0 to MAX_DATA_KEYS - 1). Returns 0 if not initialized.
     */
  uint16_t getNextKeyIndex();

  /**
     * @brief Clears all data associated with this humidity storage system.
     *        Use with caution!
     * @return true if successful, false otherwise.
     */
  bool clearAll();

  /**
     * @brief Checks if the storage has been successfully initialized.
     * @return true if initialized, false otherwise.
     */
  bool isInitialized() const;

  // Public constant for max payload size
  static const size_t MAX_PAYLOAD_BYTES = 1440;

private:
  Preferences _preferences;
  bool _is_initialized;

  // Configuration constants
  static const char* NVS_NAMESPACE;
  static const uint16_t MAX_DATA_KEYS = 2000;
  static const uint16_t RH_MIN = 450;
  static const char* LATEST_KEY_TRACKER_NAME;

  uint32_t crc32_hash(const uint8_t* data, size_t length);
};

#endif  // HUMIDITY_STORAGE_H
