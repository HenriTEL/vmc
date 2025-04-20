# ESP32 based VMC

## Getting Started

```bash
# Run the following from the outside of a rust project
rusup update # Update rust
cargo install espup cargo-espflash ldproxy cargo-generate
espup install # Install Espressif Rust ecosystem
```

## Developing

```bash
. $HOME/export-esp.sh # Set some envs
cargo espflash flash --monitor # Select the tty entry
```

## Board info

Product name: `adafruit-qt-py-esp32-pico`  
Chip version: `ESP32-Pico-V3-02`  
Arch: `Xtensa`  
Processor version: `ESP32`  

## How the project was initialized

```bash
cargo generate esp-rs/esp-idf-template cargo
```

## Useful links

Product page: <https://learn.adafruit.com/adafruit-qt-py-esp32-pico?view=all>  
Chip datasheet: <https://www.espressif.com/sites/default/files/documentation/esp32-pico_series_datasheet_en.pdf>  
