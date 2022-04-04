# Bootloader_ATMega644PA

This bootloader only works with the "Lordyphon" Ribbon Controller prototype and its onboard 23LC1024 SRAM.
As a safety concept, the firmware data will be checked for validity and buffered on the SRAM before being flashed onto the controller.

1. During power-up the RECORD - Button must be pressed to enter the boot-section of the uC otherwise Lordyphon starts main application.

2. In boot-section, the LordyLink Updater Application ( see Repository ) starts firmware transfer via handshakes, 
   INTEL-HEX data sections are written to SRAM (if checksums are correct)
   and burned into the application section of the uC.

3. User is prompted to restart the controller after successful burn.
