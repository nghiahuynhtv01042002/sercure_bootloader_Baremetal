#include "sysclocks.h"
#include "usbfs.h"
#include "gpio.h"

// USB Device Descriptor (ST VCP compatible)
const uint8_t usb_device_desc[] = {
    18,         // bLength
    0x01,       // bDescriptorType
    0x00, 0x02, // bcdUSB (USB 2.0)
    0x02,       // bDeviceClass (CDC)
    0x00,       // bDeviceSubClass (changed from 0x02)
    0x00,       // bDeviceProtocol (changed from 0x01)
    64,         // bMaxPacketSize0
    0x83, 0x04, // idVendor (STMicroelectronics)
    0x40, 0x57, // idProduct (ST VCP)
    0x00, 0x02, // bcdDevice
    1,          // iManufacturer
    2,          // iProduct
    3,          // iSerialNumber
    1           // bNumConfigurations
};

// USB Configuration Descriptor (CDC)
const uint8_t usb_config_desc[] = {
    // Configuration Descriptor
    9,          // bLength
    0x02,       // bDescriptorType
    67, 0x00,   // wTotalLength
    0x02,       // bNumInterfaces
    0x01,       // bConfigurationValue
    0x00,       // iConfiguration
    0x80,       // bmAttributes (bus-powered, changed from 0xC0)
    0x32,       // bMaxPower (100mA)
    
    // Interface Descriptor (Control Interface)
    9,          // bLength
    0x04,       // bDescriptorType
    0x00,       // bInterfaceNumber
    0x00,       // bAlternateSetting
    0x01,       // bNumEndpoints
    0x02,       // bInterfaceClass (CDC)
    0x02,       // bInterfaceSubClass (ACM)
    0x01,       // bInterfaceProtocol (AT commands)
    0x00,       // iInterface
    
    // CDC Header Functional Descriptor
    5,          // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x00,       // bDescriptorSubtype (Header)
    0x10, 0x01, // bcdCDC (1.10)
    
    // CDC Call Management Functional Descriptor
    5,          // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x01,       // bDescriptorSubtype (Call Management)
    0x00,       // bmCapabilities
    0x01,       // bDataInterface
    
    // CDC ACM Functional Descriptor
    4,          // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x02,       // bDescriptorSubtype (ACM)
    0x02,       // bmCapabilities
    
    // CDC Union Functional Descriptor
    5,          // bLength
    0x24,       // bDescriptorType (CS_INTERFACE)
    0x06,       // bDescriptorSubtype (Union)
    0x00,       // bControlInterface
    0x01,       // bSubordinateInterface0
    
    // Endpoint Descriptor (Control IN)
    7,          // bLength
    0x05,       // bDescriptorType
    0x82,       // bEndpointAddress (IN endpoint 2)
    0x03,       // bmAttributes (Interrupt)
    0x08, 0x00, // wMaxPacketSize (8 bytes)
    0x10,       // bInterval (16ms)
    
    // Interface Descriptor (Data Interface)
    9,          // bLength
    0x04,       // bDescriptorType
    0x01,       // bInterfaceNumber
    0x00,       // bAlternateSetting
    0x02,       // bNumEndpoints
    0x0A,       // bInterfaceClass (CDC Data)
    0x00,       // bInterfaceSubClass
    0x00,       // bInterfaceProtocol
    0x00,       // iInterface
    
    // Endpoint Descriptor (Data OUT)
    7,          // bLength
    0x05,       // bDescriptorType
    0x01,       // bEndpointAddress (OUT endpoint 1)
    0x02,       // bmAttributes (Bulk)
    0x40, 0x00, // wMaxPacketSize (64 bytes)
    0x00,       // bInterval
    
    // Endpoint Descriptor (Data IN)
    7,          // bLength
    0x05,       // bDescriptorType
    0x81,       // bEndpointAddress (IN endpoint 1)
    0x02,       // bmAttributes (Bulk)
    0x40, 0x00, // wMaxPacketSize (64 bytes)
    0x00        // bInterval
};

// String Descriptors
const uint8_t usb_string_langid[] = {4, 0x03, 0x09, 0x04};
const uint8_t usb_string_mfr[] = {18, 0x03, 'S',0,'T',0,'M',0,'i',0,'c',0,'r',0,'o',0,'s',0};
const uint8_t usb_string_product[] = {22, 0x03, 'S',0,'T',0,'M',0,'3',0,'2',0,' ',0,'V',0,'C',0,'P',0};
const uint8_t usb_string_serial[] = {26, 0x03, '0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'0',0,'1',0};

// CDC Line Coding structure
typedef struct {
    uint32_t bitrate;
    uint8_t format;
    uint8_t parity;
    uint8_t databits;
} CDC_LineCoding_t;

CDC_LineCoding_t line_coding = {115200, 0, 0, 8}; // 115200 bps, 1 stop, no parity, 8 data bits
static void USB_EP0_Send(const uint8_t *data, uint16_t len) {
    if (len > 64) len = 64;

    // Configure transfer length and packet count for IN endpoint
    // DIEPTSIZ: [18:0] XFRSIZ, [28:29] PKTCNT
    OTG_FS_DIEPTSIZ0 = (len & 0x7FFFF) | (1 << 19); // 1 packet

    // Write payload to FIFO0 (word writes)
    if (len > 0 && data != NULL) {
        uint32_t words = (len + 3) / 4;
        const uint8_t *p = data;
        for (uint32_t i = 0; i < words; i++) {
            uint32_t w = 0;
            for (int j = 0; j < 4; j++) {
                uint32_t idx = i * 4 + j;
                if (idx < len) w |= ((uint32_t)p[idx] << (8*j));
            }
            OTG_FS_FIFO0 = w;
        }
    }
    // Clear NAK and enable IN endpoint to start transfer
    OTG_FS_DIEPCTL0 |= (1 << 26); // CNAK (clear NAK)
    OTG_FS_DIEPCTL0 |= (1 << 31); // EPENA
}
void USB_GPIO_Init(void){
    // Enable GPIOA clock
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOA_EN;
    
    // Configure PA11, PA12 as AF10 (USB)
    GPIOA_MODER &= ~((3 << 22) | (3 << 24));
    GPIOA_MODER |= ((2 << 22) | (2 << 24));   // AF mode
    
    GPIOA_AFRH &= ~((0x0F << 12) | (0x0F << 16));
    GPIOA_AFRH |= ((0x0A << 12) | (0x0A << 16));  // AF10
    
    GPIOA_OSPEEDR |= ((3 << 22) | (3 << 24));     // High speed
    GPIOA_PUPDR &= ~((3 << 22) | (3 << 24));      // No pull
}

void USB_core_device_init(void){
    // Enable USB OTG FS clock
    RCC_AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    
    // Small delay after clock enable
    for(volatile int i = 0; i < 1000; i++);
    
    // Wait for AHB idle
    while (!(OTG_FS_GRSTCTL & OTG_FS_GRSTCTL_AHBIDL));
    
    // Core soft reset
    OTG_FS_GRSTCTL |= OTG_FS_GRSTCTL_CSRST;
    while(OTG_FS_GRSTCTL & OTG_FS_GRSTCTL_CSRST);
    
    // Wait for AHB idle again
    while (!(OTG_FS_GRSTCTL & OTG_FS_GRSTCTL_AHBIDL));
    
    // Force device mode
    OTG_FS_GUSBCFG |= OTG_FS_GUSBCFG_FDMOD;
    OTG_FS_GUSBCFG &= ~OTG_FS_GUSBCFG_FHMOD;
    
    // USB turnaround time - critical for timing
    OTG_FS_GUSBCFG &= ~(0x0F << 10);
    OTG_FS_GUSBCFG |= (0x05 << 10);  // 5 for full speed
    
    // Delay for mode change
    for(volatile int i = 0; i < 10000; i++);
    
    // Configure PHY
    OTG_FS_GCCFG = 0; // Clear first
    OTG_FS_GCCFG |= OTG_FS_GCCFG_PWRDWN;         // Power down deactivated
    OTG_FS_GCCFG |= OTG_FS_GCCFG_VBUSBSEN;      // VBUS sensing enabled
    
    // Device configuration
    OTG_FS_DCFG = 0; // Clear first
    OTG_FS_DCFG |= (3 << 0);                    // Full speed
    OTG_FS_DCFG |= OTG_FS_DCFG_NZLSOHSK;        // Non-zero length status OUT
    
    // Configure FIFOs - CRITICAL for proper operation
    OTG_FS_GRXFSIZ = 128;                        // RX FIFO: 128 words
    OTG_FS_DIEPTXF0 = (64 << 16) | 128;         // EP0 TX: 64 words at offset 128
    
    // Clear all interrupts
    OTG_FS_GINTSTS = 0xFFFFFFFF;
    
    // Enable core interrupts
    OTG_FS_GINTMSK = 0;
    OTG_FS_GINTMSK |= (1 << 12)    // USB reset
                    | (1 << 13)    // Enumeration done
                    | (1 << 4);    // RX FIFO non-empty
    
    // Device endpoint interrupts
    OTG_FS_DAINTMSK = 0x00010001;  // EP0 IN and OUT
    OTG_FS_DIEPMSK = (1 << 0);     // Transfer completed
    OTG_FS_DOEPMSK = (1 << 0) | (1 << 3); // Transfer completed + Setup phase done
    
    // Initialize EP0
    USB_EP0_Init();
    
    // Enable global interrupts - DO THIS LAST
    OTG_FS_GAHBCFG |= (1 << 0);
    
    // Soft connect - this triggers enumeration
    OTG_FS_DCTL &= ~(1 << 1);  // Clear soft disconnect
}


uint8_t ep0_buffer[64];
volatile uint8_t usb_device_state = USB_STATE_DEFAULT;
volatile uint8_t usb_config_value = 0;
volatile uint8_t usb_device_address = 0;
USB_SetupPacket_t setup_packet;

void USB_EP0_Init(void) {
    // Clear DIEPCTL0 / DOEPCTL0 then set MPS = 64 (MPSIZ in bits [0:10])
    OTG_FS_DIEPCTL0 = (64 & 0x7FF); // set max packet size = 64
    OTG_FS_DOEPCTL0 = (64 & 0x7FF);

    // Prepare to receive SETUP packet: XFRSIZ = 8, PKTCNT = 1, SETUPCNT = 1
    // DOEPTSIZ: [19:0] XFRSIZ, [28:29] PKTCNT, [30:31] SETUPCNT
    OTG_FS_DOEPTSIZ0 = (8 & 0x7FFFF)        // XFRSIZ = 8 bytes (setup)
                    | (1 << 19)             // PKTCNT = 1
                    | (1 << 29);            // SETUPCNT = 1

    // Enable OUT endpoint 0 and clear NAK (set USBACTEP and EPENA as needed)
    OTG_FS_DOEPCTL0 |= (1 << 15); // USBACTEP (device active on EP) - bit15
    OTG_FS_DOEPCTL0 |= (1 << 31); // EPENA - enable endpoint
}

static void USB_ApplyAddress(void) {
    OTG_FS_DCFG &= ~(0x7F << 4);
    OTG_FS_DCFG |= (usb_device_address << 4);
}

void USB_EP0_SetupHandler(void) {
    // Read setup packet from RX FIFO
    uint32_t word1 = OTG_FS_FIFO0;
    uint32_t word2 = OTG_FS_FIFO0;
    
    setup_packet.bmRequestType = word1 & 0xFF;
    setup_packet.bRequest = (word1 >> 8) & 0xFF;
    setup_packet.wValue = (word1 >> 16) & 0xFFFF;
    setup_packet.wIndex = word2 & 0xFFFF;
    setup_packet.wLength = (word2 >> 16) & 0xFFFF;

    // Handle standard requests
    if ((setup_packet.bmRequestType & 0x60) == 0x00) {
        switch (setup_packet.bRequest) {
        case 0x06: // GET_DESCRIPTOR
            switch (setup_packet.wValue >> 8) {
            case 1: // Device descriptor
                USB_EP0_Send(usb_device_desc, sizeof(usb_device_desc));
                break;
            case 2: // Configuration descriptor
                USB_EP0_Send(usb_config_desc, sizeof(usb_config_desc));
                break;
            case 3: // String descriptor
                switch (setup_packet.wValue & 0xFF) {
                case 0:
                    USB_EP0_Send(usb_string_langid, sizeof(usb_string_langid));
                    break;
                case 1:
                    USB_EP0_Send(usb_string_mfr, sizeof(usb_string_mfr));
                    break;
                case 2:
                    USB_EP0_Send(usb_string_product, sizeof(usb_string_product));
                    break;
                case 3:
                    USB_EP0_Send(usb_string_serial, sizeof(usb_string_serial));
                    break;
                default:
                    // Stall
                    OTG_FS_DIEPCTL0 |= (1 << 21);
                    OTG_FS_DOEPCTL0 |= (1 << 21);
                    return;
                }
                break;
            default:
                // Stall
                OTG_FS_DIEPCTL0 |= (1 << 21);
                OTG_FS_DOEPCTL0 |= (1 << 21);
                return;
            }
            break;

        case 0x05: // SET_ADDRESS
            usb_device_address = (uint8_t)(setup_packet.wValue & 0x7F);
            USB_EP0_Send(NULL, 0);  // ACK with zero-length packet
            usb_device_state = USB_STATE_ADDRESSED;
            break;

        case 0x09: // SET_CONFIGURATION
            usb_config_value = (uint8_t)(setup_packet.wValue & 0xFF);
            if (usb_config_value == 1) {
                usb_device_state = USB_STATE_CONFIGURED;
            }
            USB_EP0_Send(NULL, 0);  // ACK
            break;

        case 0x08: // GET_CONFIGURATION
            ep0_buffer[0] = usb_config_value;
            USB_EP0_Send(ep0_buffer, 1);
            break;

        default:
            // Stall unknown standard requests
            OTG_FS_DIEPCTL0 |= (1 << 21);
            OTG_FS_DOEPCTL0 |= (1 << 21);
            break;
        }
    } 
    // Handle CDC class requests
    else if ((setup_packet.bmRequestType & 0x60) == 0x20) {
        switch (setup_packet.bRequest) {
        case 0x20: // SET_LINE_CODING
            // For now, just ACK - data will come in OUT stage
            USB_EP0_Send(NULL, 0);
            break;
            
        case 0x21: // GET_LINE_CODING
            USB_EP0_Send((uint8_t*)&line_coding, sizeof(line_coding));
            break;
            
        case 0x22: // SET_CONTROL_LINE_STATE
            // ACK the control line state
            USB_EP0_Send(NULL, 0);
            break;
            
        default:
            // Stall unknown class requests
            OTG_FS_DIEPCTL0 |= (1 << 21);
            OTG_FS_DOEPCTL0 |= (1 << 21);
            break;
        }
    } else {
        // Stall all other requests
        OTG_FS_DIEPCTL0 |= (1 << 21);
        OTG_FS_DOEPCTL0 |= (1 << 21);
    }
}

void OTG_FS_IRQHandler(void) {
    uint32_t gintsts = OTG_FS_GINTSTS;

    // USB Reset
    if (gintsts & (1 << 12)) {
        usb_device_state = USB_STATE_DEFAULT;
        usb_device_address = 0;
        usb_config_value = 0;
        
        // Reinitialize EP0
        USB_EP0_Init();
        
        OTG_FS_GINTSTS = (1 << 12); // Clear interrupt
    }

    // Enumeration Done  
    if (gintsts & (1 << 13)) {
        // Set device speed to full speed
        OTG_FS_DCFG &= ~(3 << 0);
        OTG_FS_DCFG |= (3 << 0);
        
        OTG_FS_GINTSTS = (1 << 13); // Clear interrupt
    }

    // RX FIFO Non-Empty
    if (gintsts & (1 << 4)) {
        uint32_t grxstsp = OTG_FS_GRXSTSP;
        uint8_t epnum = grxstsp & 0xF;
        uint8_t pktsts = (grxstsp >> 17) & 0xF;
        uint16_t bcnt = (grxstsp >> 4) & 0x7FF;
        
        if (epnum == 0) {
            if (pktsts == 6) { // SETUP packet
                USB_EP0_SetupHandler();
            } else if (pktsts == 2) { // OUT packet
                // Read and discard data for now
                for (int i = 0; i < (bcnt + 3) / 4; i++) {
                    volatile uint32_t dummy = OTG_FS_FIFO0;
                    (void)dummy;
                }
            }
        }
    }

    // IN Endpoint interrupt
    if (gintsts & (1 << 18)) {
        uint32_t diepint = OTG_FS_DIEPINT0;
        
        if (diepint & (1 << 0)) { // Transfer completed
            // Apply address after status stage
            if (usb_device_address != 0 && usb_device_state == USB_STATE_ADDRESSED) {
                USB_ApplyAddress();
            }
        }
        
        OTG_FS_DIEPINT0 = diepint; // Clear
        OTG_FS_GINTSTS = (1 << 18);
    }

    // OUT Endpoint interrupt  
    if (gintsts & (1 << 19)) {
        uint32_t doepint = OTG_FS_DOEPINT0;
        
        if (doepint & (1 << 3)) { // Setup phase done
            // Re-enable EP0 OUT for next setup
            OTG_FS_DOEPTSIZ0 = (3 << 29) | (1 << 19) | 64;
            OTG_FS_DOEPCTL0 |= (1 << 31) | (1 << 26);
        }
        
        OTG_FS_DOEPINT0 = doepint; // Clear
        OTG_FS_GINTSTS = (1 << 19);
    }
}


// Simple test function to check if USB is enumerated
uint8_t USB_IsConfigured(void) {
    return (usb_device_state == USB_STATE_CONFIGURED);
}

