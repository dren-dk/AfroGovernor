#include "VirtualSerial.h"
#include "LUFA/Drivers/Peripheral/TWI.h"
#include <stdio.h>


/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint           =
					{
						.Address          = CDC_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = CDC_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},
			},
	};

/** Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be
 *  used like any regular character stream in the C APIs.
 */
static FILE USBSerialStream;

const int16_t STALL_RPM = 5;
const int16_t TARGET_RPM = 17;
const int16_t MAX_THROTTLE = (1<<13) -1;

const int16_t P = 100;

/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
int main(void) {
  SetupHardware();
  
  /* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
  CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);
  
  LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
  GlobalInterruptEnable();
  
  fputs("Started", &USBSerialStream);

  TWI_Init(TWI_BIT_PRESCALE_1, TWI_BITLENGTH_FROM_FREQ(TWI_BIT_PRESCALE_4, 10000));

  uint8_t internalReadAddress = 0x00;
  uint8_t readPacket[8];
  uint8_t writePacket[2];
  int16_t throttle=0;
  uint8_t started = 0;
//  struct MotorStatus motorStatus;
  uint16_t loop = 0;
  unsigned char stalled = 0;

  DDRB |= _BV(PB5);
  
  while (1) {
    /* Must throw away unused bytes from the host, or it will lock up while waiting for the device */
    CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
    
    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();

    writePacket[0] = (throttle >> 6) & 0xff;
    writePacket[1] = (throttle << 2) & 0xff;
          
    if (!TWI_WritePacket(0x53, 100, 
      &internalReadAddress, sizeof(internalReadAddress),
      &writePacket, sizeof(writePacket))) {
      //fprintf(&USBSerialStream, "OK ");
      if (throttle > 0) {
	if (throttle < 0) {
	  throttle++;
	}
      } else {
        started = 1;
      }
    } else {
      fprintf(&USBSerialStream, "Error ");
    }

    if (loop++ > 1000) {

      loop = 0;
      PORTB |= _BV(PB5);

/*
    if (!TWI_ReadPacket(0x52, 10, 
	&motorStatus, sizeof(motorStatus),
	&readPacket, sizeof(readPacket))) {
	
	if (started && throttle == 0) {
	  throttle = 1;
	}
	
	fprintf(&USBSerialStream, "throttle: %d throttle:%d\r\n",motorStatus.throttle, throttle);
      }
*/

      if (!TWI_ReadPacket(0x52, 10, 
	&internalReadAddress, sizeof(internalReadAddress),
	&readPacket, sizeof(readPacket))) {
	
	//fprintf(&USBSerialStream, "%x ",addr);
	/*
	for (int i=0;i<sizeof(readPacket);i++) {
	  fprintf(&USBSerialStream, "%02x ", readPacket[i]);
	}
	fprintf(&USBSerialStream, "\r\n");
        */
        uint16_t currentRPM = readPacket[2];
        currentRPM <<= 8;
	currentRPM += readPacket[3];
	//fprintf(&USBSerialStream, "current: %d throttle: %d\r\n", currentRPM, throttle);
      
	if (started) {
	  if (currentRPM < STALL_RPM && throttle != 0) {
	    fprintf(&USBSerialStream, "current: %d throttle: %d stall:%d\r\n", currentRPM, throttle, stalled);
	    if (stalled++ > 20) { // Give up and start over.
	     throttle = 0;
	     started = 0;
	     stalled = 0;
	    } else if (stalled > 10) {
	      throttle += MAX_THROTTLE; // Give it some wellie to get it free.	      
	    }
	    
	    
	  } else if (throttle == 0) {
	    throttle = 1;
	    
	  } else {
	    
	    signed int error = currentRPM;
	    error -= TARGET_RPM;
	    
	    if (abs(error) > 0) {
	      
	      throttle -= error * P;	      
	    }
	    
	    
	    if (throttle > MAX_THROTTLE) {
	      throttle = MAX_THROTTLE;
	    }
	    if (throttle < 1) {
	      throttle = 1;
	    }
	    
	    fprintf(&USBSerialStream, "current: %d throttle: %d error:%d\r\n", currentRPM, throttle, error);
	  }
	}
      } else {
        fprintf(&USBSerialStream, "Error.\r\n");
      }
      
      PORTB &=~ _BV(PB5);
      LEDs_ToggleLEDs(LEDS_LED1);
    }
  }
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void) {
  /* Disable watchdog if enabled by bootloader/fuses */
  MCUSR &= ~(1 << WDRF);
  wdt_disable();
  
  /* Disable clock division */
  clock_prescale_set(clock_div_1);

  /* Hardware Initialization */
  LEDs_Init();
  USB_Init();
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void) {
  LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void) {
  LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void) {
  bool ConfigSuccess = true;
  ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);  
  LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void) {
  CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

