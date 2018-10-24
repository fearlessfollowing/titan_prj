//
// Created by vans on 17-3-24.
//

#ifndef PROJECT_FAN_CONTROL_H
#define PROJECT_FAN_CONTROL_H

class fan_control {
public:
    //default 6000 rpm
    explicit fan_control(int on =150 , int off = 150);
    ~fan_control();

private:
//    void set_delay_us(int us);
    void init();
    void deinit();
    void control_thread();
    std::thread th_control_;
    bool bExit = false;
    int on_ms = 150;
    int off_ms = 150;
};

#endif //PROJECT_FAN_CONTROL_H
