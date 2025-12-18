# Embedded Media Center on LPC1768

## Short Description
An embedded multimedia application built on the NXP LPC1768 (ARM Cortex-M3) that integrates a graphical menu, photo gallery, USB-based MP3 player with hardware volume control, and a real-time Flappy Bird game, demonstrating interrupt-driven and non-blocking embedded system design.

---

## Project Overview
This project implements a **Media Center** on the Keil MCB1700 development board using the **NXP LPC1768 ARM Cortex-M3 microcontroller**. The system demonstrates how a resource-constrained embedded platform can handle **graphics rendering, audio streaming, real-time input, and game logic** within a modular and interrupt-driven architecture.

Users interact with the system through a **GLCD display**, **joystick**, and **potentiometer**, navigating between multiple multimedia features from a central menu.

### Key Features
- Graphical menu system on GLCD
- Photo gallery using RGB565 bitmap images
- USB MP3 audio streaming from a host PC
- Hardware volume control using potentiometer (ADC)
- Real-time Flappy Bird–style game
- Modular, non-blocking, interrupt-driven design

---

## Hardware & Tools

### Hardware
- NXP LPC1768 (ARM Cortex-M3)
- Keil MCB1700 development board
- Graphical LCD (GLCD)
- Joystick (UP / DOWN / LEFT / RIGHT / SELECT)
- Potentiometer (ADC input)
- Speaker / headphones (DAC output)
- USB connection to PC

### Software & Tools
- Keil µVision IDE
- CMSIS (LPC17xx)
- Keil GLCD and KBD libraries
- Keil USB Audio Class framework
- C programming language

---

## System Architecture
The application is structured as a **finite state machine (FSM)** with four main states:
1. Main Menu  
2. Photo Gallery  
3. USB MP3 Player  
4. Flappy Bird Game  

Only one subsystem is active at a time, ensuring predictable behavior and clean resource management.

### Finite State Machine
<img width="1816" height="720" alt="image" src="https://github.com/user-attachments/assets/d0601cc9-186b-4503-a0bd-0fa3cb6e58d9" />


### System Block Diagram
<img width="1784" height="1057" alt="image" src="https://github.com/user-attachments/assets/91c1c652-9c09-4327-88e9-ed8f171d3be2" />


---

## Modules

### 1. Menu System
The menu acts as the **central controller** of the Media Center.
- Joystick UP/DOWN moves the highlight
- SELECT launches a module
- Selective screen redraws improve performance and reduce flicker

<img width="884" height="955" alt="image" src="https://github.com/user-attachments/assets/5815bff6-0fe3-4cc3-8fbe-4e09c3e563b1" />


---

### 2. Photo Gallery
Displays images stored as **16-bit RGB565 C arrays**.
- LEFT / RIGHT scroll through images
- Wrap-around navigation
- SELECT returns to menu

<img width="875" height="594" alt="image" src="https://github.com/user-attachments/assets/a0c66a5e-67b5-4045-ba42-845df996fa26" />

---

### 3. USB MP3 Player
Streams audio from a host PC using the **USB Audio Class**.
- Timer0 interrupt drives DAC output
- Potentiometer controls volume in real time
- Non-blocking UI updates during playback

<img width="875" height="719" alt="image" src="https://github.com/user-attachments/assets/17164d0c-63aa-4d07-97a8-2dba5799efb5" />


---

### 4. Flappy Bird Game
A lightweight real-time game demonstrating animation and collision detection.
- Gravity and flap physics
- SELECT to flap
- Pipes regenerate with random gaps
- Game Over and restart logic

<img width="882" height="662" alt="image" src="https://github.com/user-attachments/assets/6fb5e67b-c4bc-4278-a3ff-f693e88e75a9" />
) 
<img width="884" height="788" alt="image" src="https://github.com/user-attachments/assets/cde4d3eb-b6e8-4ebf-850f-1ce3e586dc1c" />

---

## Software Design Principles
- Modular design (each feature in its own C file)
- Interrupt-driven audio playback
- Non-blocking execution for responsiveness
- Selective GLCD redraws for performance
- Clear state transitions via FSM

---

## How to Build & Run
1. Open the project in **Keil µVision**
2. Select the **LPC1768** target
3. Build the project
4. Flash to the **Keil MCB1700** board
5. Connect USB to PC for MP3 playback
6. Use joystick and potentiometer to interact

---

## Results
- Stable long-duration operation
- Smooth graphics and animation
- Clean audio playback without glitches
- Reliable module switching

The system successfully integrates graphics, audio, input devices, and real-time processing on a single embedded platform.

---

## Author
**Arman Grewal**  
COE718 – Embedded Systems Design  
Toronto Metropolitan University  

---

## Images
Create an `images/` folder in your repository and add screenshots or photos using the following filenames:

- `fsm.png`
- `system_block_diagram.png`
- `menu.png`
- `gallery.png`
- `mp3_player.png`
- `flappy_gameplay.png`
- `flappy_gameover.png`
