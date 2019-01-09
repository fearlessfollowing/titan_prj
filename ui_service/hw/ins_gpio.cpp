#include <common/include_common.h>
#include <log/log_wrapper.h>


#define GPIO_ROOT       "/sys/class/gpio"
#define GPIO_EXPORT     GPIO_ROOT "/export"
#define GPIO_UNEXPORT   GPIO_ROOT "/unexport"
#define GPIO_DIRECTION  GPIO_ROOT "/gpio%d/direction"
#define GPIO_ACTIVELOW  GPIO_ROOT "/gpio%d/active_low"
#define GPIO_VALUE      GPIO_ROOT "/gpio%d/value"


#undef  TAG
#define TAG "HwGpio"


static int gpio_write_value(const char *pathname, const char *buf, size_t count)
{
    int fd;

    if ((fd = open(pathname, O_WRONLY)) == -1) {
        LOGDBG(TAG, "gpio_write_value open path %s buf %s count %zd error", pathname, buf, count);
        return -1;
    }

    if ((size_t)write(fd, buf, count) != count) {
        close(fd);
        LOGDBG(TAG, "gpio_write_value write path %s buf %s count %zd error", pathname, buf, count);
        return -1;
    }

    return close(fd);
}

int gpio_is_requested(unsigned int gpio)
{
    int rv;
    char pathname[255] = {0};
    snprintf(pathname, sizeof(pathname), GPIO_VALUE, gpio);
    if ((rv = access(pathname, R_OK)) == -1) {
        return -1;
    }
    return (rv == 0);
}


int gpio_request(unsigned int gpio)
{
    if (gpio_is_requested(gpio) > 0) {
        return 0;
    } else {
        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%d\n", gpio);
        return gpio_write_value(GPIO_EXPORT, buffer, strlen(buffer));
    }
}



int gpio_free(unsigned int gpio)
{
    char buffer[16];
    char pathname[255];
    snprintf(pathname, sizeof(pathname), GPIO_VALUE, gpio);

    if (access(pathname, R_OK) == -1) {
        LOGERR(TAG, "gpio_free gpio %d error", gpio);
        return -1;
    }

    snprintf(buffer, sizeof(buffer), "%d\n", gpio);
    return gpio_write_value(GPIO_UNEXPORT, buffer, strlen(buffer));
}


int gpio_direction_input(unsigned int gpio)
{
    char pathname[255];
    snprintf(pathname, sizeof(pathname), GPIO_DIRECTION, gpio);
    return gpio_write_value(pathname, "in", strlen("in") + 1);
}

int gpio_direction_output(unsigned int gpio, int value)
{
    char pathname[255];
    const char *val;
    snprintf(pathname, sizeof(pathname), GPIO_DIRECTION, gpio);
    val = value ? "high" : "low";
    return gpio_write_value(pathname, val, strlen(val) + 1);
}

int gpio_get_value(unsigned int gpio)
{
    int fd;
    char pathname[255];
    char buffer;

    snprintf(pathname, sizeof(pathname), GPIO_VALUE, gpio);

    if ((fd = open(pathname, O_RDONLY)) == -1) {
        LOGERR(TAG, "gpio_get_value open gpio %d error\n", gpio);
        return -1;
    }

    if (read(fd, &buffer, sizeof(buffer)) != sizeof(buffer)) {
        LOGERR(TAG, "gpio_get_value read gpio %d error\n", gpio);
        close(fd);
        return -1;
    }

    if (close(fd) == -1) {
        LOGERR(TAG, "gpio_get_value close gpio %d error", gpio);
        return -1;
    }

    return buffer - '0';
}

int gpio_set_value(unsigned int gpio, int value)
{
    char pathname[255];
    snprintf(pathname, sizeof(pathname), GPIO_VALUE, gpio);
    return gpio_write_value(pathname, value ? "1" : "0", 2);
}
