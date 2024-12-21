# Project Name

BPC_SPC_Project

## Table of Contents

- [Project Name](#project-name)
    - [Table of Contents](#table-of-contents)
    - [Overview](#overview)
    - [Features](#features)
    - [Installation](#installation)
    - [Usage](#usage)
    - [Libraries](#libraries)

## Overview

This project is part of a school project, including the setting of the PC volume.
The second part of this project can be found here: https://github.com/MartinStieber/SPC_2024_project_embed.
This project demonstrates the use of serial communication, signal handling, and volume control on a Linux-based system. It includes features such as UART initialization, signal handling, and volume adjustment.

## Features

- Serial communication with UART
- Signal handling for cleanup on exit
- Volume control based on received data
- Multi-threading for handling audio mixer

## Installation

1. Clone the repository:
    ```sh
    git clone https://github.com/MartinStieber/BPC_SPC_Project.git
    ```
2. Navigate to the project directory:
    ```sh
    cd BPC_SPC_Project
    ```
3. Build the project:
    ```sh
    make
    ```

## Usage

1. Connect your hardware device.
2. Run the program:
    ```sh
    ./BPC_SPC_Project
    ```
3. Follow the on-screen instructions to ensure proper communication and volume control.

## Libraries

This project uses the following libraries:

- `pthread` - For multi-threading support.
- `signal` - For handling various signals for cleanup on exit.
- `termios` - For UART communication.
- Custom `queue` library for buffer management.