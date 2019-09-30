# note-stm32g0

This is an example of how to use the Notecard and the [note-c][note-c] library
with the native STM32 SDK.

The board that I used for testing is the [Nucleo-G031K8][board] kit with STM32G031 MCU.  This board
was selected because of its minimalism and the fact that it has an integrated debug probe.

As a proof-of-concept, it implements the same functions as the [note-arduino][note-arduino] library's JSON
example.

One other notable feature of this example is that it implements a working example of the
STM32's extremely low-power STOP1 mode.  When running the example, the STM32 draws so little power that
it is highly applicable, by using the code here, for battery-powered applications. An extension of
this example allows one to hook a wire to one of the STM32's GPIO pins, simulating a button press
that will also wake up the STM32 to update its sensor readings.

## Hardware Setup
Before you begin using the software, wire your Nucleo development board to the Notecarrier containing
the Notecard.  You may wire it for Serial, I2C, or both.  In order to do so, you'll need some standard female-to-female
header wires to jumper between the boards.  For reference, the Nucleo's user manual is [here][reference-manual]
- Connect the Notecarrier's GND pin to any of the Nucleo's GND pins
- For Serial
  - Connect the Notecarrier's RX to the Nucleo's D1/TX pin (USART1_TX PB6)
  - Connect the Notecarrier's TX to the Nucleo's D0/RX pin (USART1_RX PB7)
- For I2C
  - Connect the Notecarrier's SDA pin to the Nucleo's D4 pin (I2C1_SDA PA10)
  - Connect the Notecarrier's SCL pin to the Nucleo's D5 pin (I2C1_SCL PA9)
- In order to simulate a "button" for testing
  - Connect a wire to the Nucleo's D12 pin (GPIO PB4).  You will simulate a button press by temporarily
    touching it to any Nucleo GND pin.
- Connect both the Notecarrier and Nucleo to power by using their USB connectors to connect them to your development machine.

## Installation of the STMicroelectronics Integrated Development Environment

In order to do development with this example, you will need to download and install STMicroelectronics's free IDE
called [STM32CubeIDE][ide].  During installation it will ask you to specify a directory to contain your projects.
Remember this directory path, because that's where you'll download this example.

## Installation of this example

Clone this [note-stm32g0][note-stm32g0] repo into the projects folder that you selected during IDE
installation, as follows.  You'll note that the latest copy of the [note-c][note-c] C library is already
loaded by default, as a subdirectory of the [note-stm32g0][note-stm32g0] directory.

```
STM32CubeIDE projects folder
├ note-stm32g0
  ├ note-c  
```

In STM32CubeIDE, open the [note-stm32g0][note-stm32g0] project.  Make sure that you edit the "my" definitions
at the top of example.c so that this example will send data to your notehub.io project, and so that it uses
serial or I2C as you wish.  By using the standard Debug build configuration, you should be able to build and run the project.

This example has been tested with both UART and with I2C, and it has been verified that the project's STOP1 mode
works very reliably.  Note that in event.h there is a conditional (defaulted ON) that uses the green LED on the Nucleo
to indicate whether or not the STM32 is in STOP1 mode.  If it's OFF, it is stopped.  (Note that it will never enter
STOP1 mode while you are in the debugger, else the debugger would halt.)

[note-stm32g0]: https://github.com/blues/note-stm32g0
[note-c]: https://github.com/blues/note-c
[note-arduino]: https://github.com/blues/note-arduino
[board]: https://www.st.com/en/evaluation-tools/nucleo-g031k8.html
[reference-manual]: https://www.st.com/resource/en/user_manual/dm00622380.pdf
[ide]: https://www.st.com/en/development-tools/stm32cubeide.html
