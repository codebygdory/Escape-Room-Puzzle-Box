# Multi-Stage Sequential Escape Room Puzzle Box

An advanced embedded security and authentication matrix built on the ATmega328P architecture featuring a three-tiered physical puzzle sequence gating a hardware-locked vault mechanism.

## Technical Highlights

- **Sequential Multi-Tiered State Machine:** Implements a strict, deterministic software state machine that isolates and handles authentication stages sequentially, preventing bypass attacks or out-of-order execution.
- **Dynamic Timing & Pattern Validation:** Evaluates real-time sensor inputs against pre-defined temporal windows for the motion-sequence memory and light-pattern matching arrays.
- **Hardware-Isolated Actuation:** Separates logic evaluation from the mechanical physical layer, pulsing precise pulse-width modulation (PWM) control signals to gate a servo-driven deadbolt assembly upon final code validation.
- **Debounced Input Filtering:** Implements software-level time-averaging and debouncing loops on passcode entries to ensure tactile physical interactions remain precise and false-trigger free.

## System Architecture & Components

| Component | Function / Purpose | Interface / Protocol |
| :--- | :--- | :--- |
| **Arduino Uno** | Central puzzle logic processor tracking sequence authentication arrays | N/A |
| **Servo Motor** | Drives the physical deadbolt mechanical lock upon final verification | Digital PWM |
| **RGB LED** | Emits multi-colored light-pattern codes and provides local user status feedback | Multi-Channel Digital Out |
| **Sensors & Inputs** | Array of hardware inputs (Buttons, Photoresistors, or IMU depending on build) | Mixed Analog & Digital GPIO |

## How to Replicate and Test

1. Clone this repository to your local computer.
2. Open the primary `.ino` sketch file in your development tool of choice.
3. Wire the physical input components and the lockout servo to your designated I/O layout pins.
4. Flash the code to your Arduino Uno.
5. Cycle through the sequential inputs (Motion -> Light -> Passcode) to test the servo trigger logic.
