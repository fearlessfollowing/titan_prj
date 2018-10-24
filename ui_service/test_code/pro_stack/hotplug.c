//
// Created by vans on 16-12-2.
//
#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fifo_event.h"
#include "libusb_mine.h"

//#define USE_DEF_USBCONTEXT

int send_usb_event(void *pV);
static int bExit = 0;
static int send_hp_event( char *action,int vid,int pid)
{
    //skip usb camera
    if(vid != 0x4255)
    {
        USB_HP_EVENT stHPEvent;

        memset(&stHPEvent,0, sizeof(USB_HP_EVENT));

        snprintf(stHPEvent.action,sizeof(stHPEvent.action),"%s",action);
        stHPEvent.iVID = vid;
        stHPEvent.iPID = pid;
        return send_usb_event((void *)&stHPEvent);
    }
    else
    {
        return 0;
    }
}

static void debug_usb_des(struct libusb_device_descriptor *pDesc)
{
    printf(" bLength %0x%x "
                   "bDescriptorType 0x%x "
                   "bcdUSB 0x%x "
           "bDeviceClass 0x%x "
                   "bDeviceSubClass 0x%x "
                   "bDeviceProtocol 0x%x "
                   "bMaxPacketSize0 0x%x "
                   "idVendor 0x%x "
                   "idProduct 0x%x "
                   "bcdDevice 0x%x "
                   "iManufacturer 0x%x "
                   "iProduct 0x%x "
                   "iSerialNumber 0x%x "
           "bNumConfigurations 0x%x\n",
           pDesc->bLength,
           pDesc->bDescriptorType,
           pDesc->bcdUSB,
           pDesc->bDeviceClass,
            pDesc->bDeviceSubClass,
           pDesc->bDeviceProtocol,
    pDesc->bMaxPacketSize0,
           pDesc->idVendor,
    pDesc->idProduct,
    pDesc->bcdDevice,
           pDesc->iManufacturer,
    pDesc->iProduct,
           pDesc->iSerialNumber,
    pDesc->bNumConfigurations);
}

static int LIBUSB_CALL hotplug_callback(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    struct libusb_device_descriptor desc;
    int rc;
    char *action = "add";

    rc = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "Error getting device descriptor\n");
    }
    printf ("2Device attached: %04x:%04x\n", desc.idVendor, desc.idProduct);
    debug_usb_des(&desc);
    return send_hp_event(action,desc.idVendor,desc.idProduct);
}

static int LIBUSB_CALL hotplug_callback_detach(libusb_context *ctx, libusb_device *dev, libusb_hotplug_event event, void *user_data)
{
    struct libusb_device_descriptor desc;
    char *action = "remove";
    int rc;

    rc = libusb_get_device_descriptor(dev, &desc);
    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "Error getting device descriptor\n");
    }

    printf ("Device dettached: %04x:%04x\n", desc.idVendor, desc.idProduct);
    debug_usb_des(&desc);
    return send_hp_event(action , desc.idVendor,desc.idProduct);
}

int stop_monitor_hotplug(void)
{
    bExit = 1;
    return 0;
}

int start_monitor_hotplug(void)
{
    libusb_hotplug_callback_handle hp[2];
    int product_id, vendor_id, class_id;
    int rc;

    vendor_id  =  LIBUSB_HOTPLUG_MATCH_ANY;
    product_id =  LIBUSB_HOTPLUG_MATCH_ANY;
    class_id   =  LIBUSB_HOTPLUG_MATCH_ANY;
    printf("start_monitor_hotplug\n");
#ifdef USE_DEF_USBCONTEXT
    rc = libusb_init (NULL);
    if (rc < 0)
    {
        printf("failed to initialise libusb: %s\n", libusb_error_name(rc));
        return EXIT_FAILURE;
    }

    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
        printf ("Hotplug capabilites are not supported on this platform\n");
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }

    rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0, vendor_id,
                                           product_id, class_id, hotplug_callback, NULL, &hp[0]);

    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "Error registering callback 0\n");
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }

    rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, vendor_id,
                                           product_id,class_id, hotplug_callback_detach, NULL, &hp[1]);

    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "Error registering callback 1\n");
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    while (!bExit) {
        rc = libusb_handle_events_timeout (NULL, &tv);
//        rc = libusb_handle_events(NULL);
        if (rc < 0)
        {
            printf("libusb_handle_events() failed: %s\n", libusb_error_name(rc));
        }
    }
    printf("start exit libusb\n");
    libusb_exit (NULL);
#else
    printf("use private usb context\n");
    libusb_context *ctx;
    rc = libusb_init (&ctx);
    if (rc < 0)
    {
        printf("failed to initialise libusb: %s\n", libusb_error_name(rc));
        return EXIT_FAILURE;
    }

    if (!libusb_has_capability (LIBUSB_CAP_HAS_HOTPLUG)) {
        printf ("Hotplug capabilites are not supported on this platform\n");
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }

    rc = libusb_hotplug_register_callback (ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED, 0, vendor_id,
                                           product_id, class_id, hotplug_callback, NULL, &hp[0]);

    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "Error registering callback 0\n");
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }

    rc = libusb_hotplug_register_callback (ctx, LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0, vendor_id,
                                           product_id,class_id, hotplug_callback_detach, NULL, &hp[1]);

    if (LIBUSB_SUCCESS != rc) {
        fprintf (stderr, "Error registering callback 1\n");
        libusb_exit (NULL);
        return EXIT_FAILURE;
    }

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    while (!bExit) {
        rc = libusb_handle_events_timeout (ctx, &tv);
//        rc = libusb_handle_events(NULL);
        if (rc < 0)
        {
            printf("libusb_handle_events() failed: %s\n", libusb_error_name(rc));
        }
    }
    printf("start exit libusb\n");
    libusb_exit (ctx);
#endif
    return EXIT_SUCCESS;
}
#endif