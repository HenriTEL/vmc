# ESP32 based VMC

## Getting Started

```bash
rusup update # Update rust
cargo install espup ldproxy
espup install # Install Espressif Rust ecosystem
cargo install cargo-espflash
```

## Developing

```bash
source $HOME/export-esp.sh # Set some envs
```

## Board info

Product name: `adafruit-qt-py-esp32-pico`  
Chip version: `ESP32-Pico-V3-02`  
Arch: `Xtensa`  
Processor version: `ESP32`  

## How the project was initialize

```bash
cargo install cargo-generate
cargo generate esp-rs/esp-idf-template cargo
```

## Useful links

Product page: <https://learn.adafruit.com/adafruit-qt-py-esp32-pico?view=all>  
Chip datasheet: <https://www.espressif.com/sites/default/files/documentation/esp32-pico_series_datasheet_en.pdf>  
