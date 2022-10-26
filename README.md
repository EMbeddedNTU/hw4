# HW4: Peripheral in mbed-os (stm32) and Central in Python (RPi)

This codebase contains 2 parts that can be found in the central peripheral and the SocketServer respectively.

The peripheral folder contains the code and config files for the STM32 IoT node. The program can be built using Mbed Studio. The functionality for the peripheral is sending data to the RPi (central) via bluetooth.

The central folder contains the code for RPi. It scans for the peripheral named `IOT32` and prints out the data recieved from the peripheral.

## Team member:
b09901052 劉承亞
b09901058 邱奕翔

## How to run 

### Run the socket client
    
1, Open Mbed Studio

2, Import the program from here: https://github.com/ARMmbed/mbed-os-example-ble/

3, Copy the `mbed_app.json` in this repository to `./ble_gattserver_addservice/mbed_app.json`

4, Copy the `main.cpp` in this repository to `./ble_gattserver_addservice/source/main.cpp`

5, Copy the `ButtonService.h` directory into `./ble_gattserver_addservice/mbed-os-ble-utils/ButtonService.h`
        

### Setup RPi as central 

1, Clone this repository on RPi.

2, Go into the central directory
```
cd ./central
```

3, Then you can run the code by simply typing 
```
python ble_central.py
``` 

NOTE: You might need to use `sudo` for the last command.


## Features
- The central prints out the recieved data whenever there is a value changed.
- When the button state hasn't been updated (the peripheral hasn't notified the central), the central only prints out the heart rate data.

<img src=docs/figure_result.png height="400"/>