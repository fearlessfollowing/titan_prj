//
// Created by vans on 16-12-2.
//

#ifndef INC_360PRO_SERVICE_FIFO_EVENT_H
#define INC_360PRO_SERVICE_FIFO_EVENT_H

typedef struct usb_hp_event {
    char action[32];
    int iVID;
    int iPID;
    int iSerialNum;
    //2 --usb2.0, 3 usb3.0
    int usb_version;
} USB_HP_EVENT;

//typedef struct oled_event
//{
//    int oled_cmd;
//    char *str;
//}OLED_EVENT;


//typedef struct fifo_event
//{
//    int cmd;
//    void* pstEventDetail;
//}FIFO_EVENT;

//enum
//{
//    FIFO_USB_HP,
//    FIFO_BATTERY,
//};
#endif //INC_360PRO_SERVICE_FIFO_EVENT_H