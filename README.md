# Embedded Interrupt-Driven Game System (ARM MCU)

## Overview

This project implements a bare-metal embedded system on the NUC140 ARM Cortex-M microcontroller, featuring a game system with **interrupt-driven UART communication**, **SPI** and **ADC integration**, and **real-time display management**.

The system consists of a **Battleship-style game** where the user selects coordinates to "shoot" at ships. The game uses UART for communication, SPI for data transmission, and ADC to read analog voltages to trigger specific actions. The user interacts with the game using a keypad and views the game state via an LCD and 7-segment display.

## Key Features
- **UART Communication**: Receive map data from a PC using UART interrupts. Data is received and displayed on an LCD.
- **SPI & ADC Integration**: Continuously sample analog input through ADC and send the team name over SPI to an external device when a threshold is reached.
- **Finite State Machine (FSM)**: Manage the game states and transitions using an FSM, handling user input and game logic.
- **Real-Time Display**: Use **timer interrupts** for multiplexed display on a **7-segment display** and real-time updates on an LCD screen.
- **Keypad & External Interrupts**: Handle user input via a keypad for coordinate selection and use an external interrupt to start the game.

## Hardware Used
- **NUC140 ARM Cortex-M Microcontroller**
- **LCD Display** for showing game state and messages
- **7-segment Display** for showing shots fired
- **Keypad** for coordinate selection
- **LEDs** for visual indicators (hits/misses)
- **Buzzer** for game over indication
- **UART-to-USB Adapter** for communication with PC
- **Oscilloscope** for signal validation (SPI waveform)

## System Design

### 1. **Interrupt-Driven UART Communication**
- Configured UART for **bidirectional communication** between the microcontroller and a PC. 
- Used **UART interrupts** to receive and transmit data, ensuring real-time handling of inputs and outputs.

### 2. **SPI & ADC Integration**
- **ADC** is used to continuously sample an analog input (e.g., potentiometer). If the voltage is above 2V, the system sends the **team name** via **SPI** to an external device.
- Configured **SPI in master mode** with specific settings for clock polarity, edge, and LSB-first data transfer.

### 3. **Game Logic with Finite State Machine (FSM)**
- The game operates based on an FSM, which tracks the game's state (e.g., waiting for input, shooting, game over).
- The system **detects hits and misses**, displays them on the LCD, and flashes an LED when a ship is hit.
- **Game over** is triggered when all ships are sunk or the user exceeds the allowed number of shots.

### 4. **Real-Time Display & Keypad Input**
- Used **timer interrupts** to control the 7-segment display for real-time counters.
- **Keypad input** allows the user to select coordinates for the game, with real-time updates on the display.

## Installation

1. **Clone the repository:**

   ```bash
   git clone https://github.com/yourusername/embedded-game-system.git
   cd embedded-game-system
   ```

2. **Set up the environment for the NUC140:**
  - **Install the Keil MDK** (Microcontroller Development Kit) for ARM:  
    You can download the latest version of Keil MDK from [the official ARM Keil website](https://www.keil.com/download/).  
    Follow the installation instructions provided for your operating system.

  - **Install the Nuvoton NUC140 MCU toolchain**:  
    To develop for the NUC140, you will need the **Nuvoton NUC140 development tools**. These tools should be included in the Keil MDK when you install it, but make sure to check the **device support pack** for NUC140 in Keil MDK.

  - **Install USB-to-UART drivers**:  
    If you're using a USB-to-UART adapter to connect the NUC140 to your PC, ensure that the necessary drivers are installed for your operating system. This may include drivers for **CH340 USB-to-UART** chips or other common adapters.

3. Open the project file in Keil MDK:
  - Navigate to the folder where you cloned this repository.
  - Open the `*.uvprojx` project file in Keil MDK. This is the project configuration file for the NUC140 platform.
  - If prompted, select the appropriate **Toolchain** for the NUC140 and ensure that you have the correct **device settings** for the NUC140 series.

- **Configure the project settings**:
  - Verify that the **NUC140** microcontroller is selected in the project settings.
  - Ensure that the **compiler and linker settings** match the NUC140 development requirements.
  - Set the **target device** in Keil MDK to NUC140 to match your hardware configuration.

4. **Build the project:**
  - After configuring the project, click on the **Build** button in Keil MDK to compile the code. This will generate the output binary file (`.bin`) that can be flashed onto the NUC140.
  - If there are any errors, resolve them by checking the configuration and the included libraries.

5. **Flash the NUC140 microcontroller:**
  - Once the project has successfully built, you can flash the `.bin` file to your NUC140 MCU.
  - Connect the NUC140 to your PC via a **JTAG/SWD** programmer or **UART-to-USB adapter**.
  - Use the Keil **Flash** tool to upload the binary to the device.

6. **Hardware setup:**
  - Connect the NUC140 to the LCD, keypad, 7-segment display, and UART-to-USB adapter.
  - Use **terminal software** (like **PuTTY** or **Tera Term**) to communicate with the NUC140 board via UART. Configure the terminal with the correct **baud rate** (115200 bps), **data format**, and **stop bits** to match your system’s settings.
  - After successfully uploading the code, open the terminal to interact with the system and load the game map data.

7. **Testing and running the system:**
  - After flashing the firmware to the NUC140, use the connected keypad and LCD to interact with the game.
  - Press the **SW_INT** button (GPIO15) to start the game, and select coordinates via the keypad to shoot at the ships.

## Usage

1. **Map Loading**:  
   - Send the game map via UART from your PC. This will populate the game field on the LCD.
   - The system will confirm with "Map Loaded Successfully".

2. **Game Start**:  
   - Press the **SW_INT** button (GPIO15) to start the game.
   - Select coordinates using the keypad to shoot. The LCD will display hits and misses, and the 7-segment display will show the number of shots fired.

3. **Game Over**:  
   - The game ends when all ships are sunk or if the user exceeds 16 shots. The system will display "Game Over" and the buzzer will sound.

4. **Replay**:  
   - Press the **SW_INT** button again to restart the game.

## Validation

- **Oscilloscope** was used to verify SPI waveform integrity and ADC readings.
- **Unit testing** was performed to ensure that UART interrupts, SPI communication, and ADC sampling were working correctly.

## 📜 License
This project is for educational purposes under the RMIT **EEET2481 - Embedded System Design and Implementation** course.
Not intended for commercial distribution.
