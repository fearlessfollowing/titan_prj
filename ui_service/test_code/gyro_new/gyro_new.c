/**
 *   @defgroup  eMPL
 *   @brief     Embedded Motion Processing Library
 *
 *   @{
 *       @file      main.c
 *       @brief     Test app for eMPL using the Motion Driver DMP image.
 */
 
/* Includes ------------------------------------------------------------------*/
#include "stdio.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "invensense.h"
#include "invensense_adv.h"
#include "eMPL_outputs.h"
#include "mltypes.h"
#include "mpu.h"
#include "log.h"
#include "packet.h"
#include "ins_i2c.h"
#include "gyro_local.h"
#include "gyro_data.h"

/* Private typedef -----------------------------------------------------------*/
/* Data read from MPL. */
#define PRINT_ACCEL     (0x01)
#define PRINT_GYRO      (0x02)
#define PRINT_QUAT      (0x04)
#define PRINT_COMPASS   (0x08)
#define PRINT_EULER     (0x10)
#define PRINT_ROT_MAT   (0x20)
#define PRINT_HEADING   (0x40)
#define PRINT_PEDO      (0x80)
#define PRINT_LINEAR_ACCEL (0x100)
#define PRINT_GRAVITY_VECTOR (0x200)

volatile uint32_t hal_timestamp = 0;
#define ACCEL_ON        (0x01)
#define GYRO_ON         (0x02)
#define COMPASS_ON      (0x04)

#define MOTION          (0)
#define NO_MOTION       (1)

/* Starting sampling rate. */
#define DEFAULT_MPU_HZ  100//(20)

#define FLASH_SIZE      (512)
#define FLASH_MEM_START ((void*)0x1800)

#define PEDO_READ_MS    (1000)
#define TEMP_READ_MS    (500)
#define COMPASS_READ_MS (100)

//#define OPEN_TEMP

struct rx_s {
    unsigned char header[3];
    unsigned char cmd;
};
struct hal_s {
    unsigned char lp_accel_mode;
    unsigned char sensors;
    unsigned char dmp_on;
    unsigned char wait_for_tap;
    volatile unsigned char new_gyro;
    unsigned char motion_int_mode;
    unsigned int no_dmp_hz;
    unsigned int next_pedo_ms;
    unsigned int next_temp_ms;
    unsigned int next_compass_ms;
    unsigned int report;
    unsigned short dmp_features;
    struct rx_s rx;
};
static struct hal_s hal = {0};

/* USB RX binary semaphore. Actually, it's just a flag. Not included in struct
 * because it's declared extern elsewhere.
 */
volatile unsigned char rx_new;

const float divide_gyro = 16.4f;
const float divide_accel = 16384.0f;

unsigned char *mpl_key = (unsigned char*)"eMPL 5.1";

/* Platform-specific information. Kinda like a boardfile. */
struct platform_data_s {
    signed char orientation[9];
};

/* The sensors can be mounted onto the board in any orientation. The mounting
 * matrix seen below tells the MPL how to rotate the raw data from the
 * driver(s).
 * TODO: The following matrices refer to the configuration on internal test
 * boards at Invensense. If needed, please modify the matrices to match the
 * chip-to-body matrix for your particular set up.
 */
static struct platform_data_s gyro_pdata = {
    .orientation = { 1, 0, 0,
                     0, 1, 0,
                     0, 0, 1}
};

#if defined MPU9150 || defined MPU9250
#ifdef COMPASS_ENABLED
static struct platform_data_s compass_pdata = {
    .orientation = { 0, 1, 0,
                     1, 0, 0,
                     0, 0, -1}
};
#endif
//#define COMPASS_ENABLED 0
#elif defined AK8975_SECONDARY
static struct platform_data_s compass_pdata = {
    .orientation = {-1, 0, 0,
                     0, 1, 0,
                     0, 0,-1}
};
//#define COMPASS_ENABLED 0
#elif defined AK8963_SECONDARY
static struct platform_data_s compass_pdata = {
    .orientation = {-1, 0, 0,
                     0,-1, 0,
                     0, 0, 1}
};
//#define COMPASS_ENABLED 0
#endif

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* ---------------------------------------------------------------------------*/
/* Get data from MPL.
 * TODO: Add return values to the inv_get_sensor_type_xxx APIs to differentiate
 * between new and stale data.
 */
static void read_from_mpl(GYRO_DAT *pstTmp)
{
    int /*msg,*/ data[9];
    int8_t accuracy;
    unsigned long long timestamp;
    float float_data[3] = {0.0};


    if (inv_get_sensor_type_quat(data, &accuracy, (inv_time_t*)&timestamp)) {
        pstTmp->quat_timestep = (long long)timestamp;

#ifdef ENABLE_DEBUG_INTERVAL
        if(pstTmp->raw_timestep != pstTmp->quat_timestep )
        {
            printf("ts mismatch(%lld %lld)\n",pstTmp->raw_timestep,pstTmp->quat_timestep);
        }
         printf("interval %lld\n",
               (pstTmp->quat_timestep - pstTmp->last_quat_timestep));
        pstTmp->last_quat_timestep = pstTmp->quat_timestep;
#endif
//        printf("send eMPL data\n");
//       /* Sends a quaternion packet to the PC. Since this is used by the Python
//        * test app to visually represent a 3D quaternion, it's sent each time
//        * the MPL has new data.
//        */
//        eMPL_send_quat(data);
//
//        /* Specific data packets can be sent or suppressed using USB commands. */
//        if (hal.report & PRINT_QUAT)
//            eMPL_send_data(PACKET_DATA_QUAT, data);
        memcpy(pstTmp->quat,data,4);
    }
    else
    {
        printf("read quta error\n");
    }

//    printf("hal.report 0x%x\n", hal.report);

    if (hal.report & PRINT_ACCEL) {
        if (inv_get_sensor_type_accel(data, &accuracy,
            (inv_time_t*)&timestamp))
        {
//            printf("new accel %d %d %d\n",data[0],data[1],data[2]);
//            if(((float)data[0]*1.0f/(1<<16) != pstTmp->acc_x) ||
//                    ((float)data[1]*1.0f/(1<<16) != pstTmp->acc_y) ||
//                    ((float)data[2]*1.0f/(1<<16) != pstTmp->acc_z))
//            {
//                printf("accel org %f %f %f\r\n"
//                               "new accel: %f %f %f\r\n",
//                       pstTmp->acc_x,
//                       pstTmp->acc_y,
//                       pstTmp->acc_z,
//                       (float)data[0]*1.0f/(1<<16),
//                       (float)data[1]*1.0f/(1<<16),
//                       (float)data[2]*1.0f/(1<<16));
//            }
            pstTmp->acc_x_new = (float)data[0]*1.0f/(1<<16);
            pstTmp->acc_y_new = (float)data[1]*1.0f/(1<<16);
            pstTmp->acc_z_new = (float)data[2]*1.0f/(1<<16);
        }
//            eMPL_send_data(PACKET_DATA_ACCEL, data);
    }

    if (hal.report & PRINT_GYRO) {
        if (inv_get_sensor_type_gyro(data, &accuracy,
            (inv_time_t*)&timestamp))
        {
//            printf("new gyro %d %d %d\n",data[0],data[1],data[2]);
//            printf("fnew gyro %f %f %f\n",(float)data[0]*1.0f/(1<<16),(float)data[1]*1.0f/(1<<16),(float)data[2]*1.0f/(1<<16));
//            if(((float)data[0]*1.0f/(1<<16) != pstTmp->gyro_x) ||
//               ((float)data[1]*1.0f/(1<<16) != pstTmp->gyro_y) ||
//               ((float)data[2]*1.0f/(1<<16) != pstTmp->gyro_z))
//            {
//                printf("gyro org %f %f %f\r\n"
//                               "gyro new %f %f %f\r\n",
//                       pstTmp->gyro_x,
//                       pstTmp->gyro_y,
//                       pstTmp->gyro_z,
//                       (float)data[0]*1.0f/(1<<16),
//                       (float)data[1]*1.0f/(1<<16),
//                       (float)data[2]*1.0f/(1<<16));
//            }
            pstTmp->gyro_x_new = (float)data[0]*1.0f/(1<<16);
            pstTmp->gyro_y_new = (float)data[1]*1.0f/(1<<16);
            pstTmp->gyro_z_new = (float)data[2]*1.0f/(1<<16);
        }
//      eMPL_send_data(PACKET_DATA_GYRO, data);
    }

#ifdef COMPASS_ENABLED
    if (hal.report & PRINT_COMPASS) {
        if (inv_get_sensor_type_compass(data, &accuracy,
            (inv_time_t*)&timestamp))
            eMPL_send_data(PACKET_DATA_COMPASS, data);
    }
#endif

    if (hal.report & PRINT_EULER) {
        if (inv_get_sensor_type_euler(data, &accuracy,
            (inv_time_t*)&timestamp))
            eMPL_send_data(PACKET_DATA_EULER, data);
    }
    if (hal.report & PRINT_ROT_MAT) {
        if (inv_get_sensor_type_rot_mat(data, &accuracy,
            (inv_time_t*)&timestamp))
            eMPL_send_data(PACKET_DATA_ROT, data);
    }
    if (hal.report & PRINT_HEADING) {
        if (inv_get_sensor_type_heading(data, &accuracy,
            (inv_time_t*)&timestamp))
            eMPL_send_data(PACKET_DATA_HEADING, data);
    }
    if (hal.report & PRINT_LINEAR_ACCEL) {
        if (inv_get_sensor_type_linear_acceleration(float_data, &accuracy, (inv_time_t*)&timestamp)) {
#ifdef ENABLE_DEBUG
            printf("Linear Accel: %7.5f %7.5f %7.5f accuracy %d\r\n",
        			float_data[0], float_data[1], float_data[2],accuracy);
#endif
            pstTmp->liner_accel_x = float_data[0];
            pstTmp->liner_accel_y = float_data[1];
            pstTmp->liner_accel_z = float_data[2];
         }
    }
    if (hal.report & PRINT_GRAVITY_VECTOR) {
            if (inv_get_sensor_type_gravity(float_data, &accuracy,
                (inv_time_t*)&timestamp))
            {
                float gravity_divide = 9.80665f;
#ifdef ENABLE_DEBUG
//                printf("Gravity Vector: %7.5f %7.5f %7.5f accuracy %d\r\n",
//            			float_data[0], float_data[1], float_data[2],accuracy);
        printf("Gravity Vector: %7.5f %7.5f %7.5f accuracy %d\r\n",
               float_data[0]/gravity_divide, float_data[1]/gravity_divide, float_data[2]/gravity_divide,accuracy);
#endif
                pstTmp->gravity_x = float_data[0]/gravity_divide;
                pstTmp->gravity_y = float_data[1]/gravity_divide;
                pstTmp->gravity_z = float_data[2]/gravity_divide;
            }
    }
    if (hal.report & PRINT_PEDO) {
        unsigned long long timestamp;
        nv_get_clock_ms(&timestamp);
        if (timestamp > hal.next_pedo_ms) {
            hal.next_pedo_ms = timestamp + PEDO_READ_MS;
            unsigned int step_count, walk_time;
            dmp_get_pedometer_step_count(&step_count);
            dmp_get_pedometer_walk_time(&walk_time);
            MPL_LOGI("Walked %u steps over %u milliseconds..\n", step_count,
            walk_time);
        }
    }

#if 0
    /* Whenever the MPL detects a change in motion state, the application can
     * be notified. For this example, we use an LED to represent the current
     * motion state.
     */
    msg = inv_get_message_level_0(INV_MSG_MOTION_EVENT |
            INV_MSG_NO_MOTION_EVENT);
    if (msg) {
        if (msg & INV_MSG_MOTION_EVENT) {
            printf("Motion!\n");
        } else if (msg & INV_MSG_NO_MOTION_EVENT) {
             printf("No motion!\n");
        }
    }
#endif
}

#ifdef COMPASS_ENABLED
void send_status_compass() {
	int data[3] = { 0 };
	int8_t accuracy = { 0 };
	unsigned int timestamp;
	inv_get_compass_set(data, &accuracy, (inv_time_t*) &timestamp);
	MPL_LOGI("Compass: %7.4f %7.4f %7.4f ",
			data[0]/65536.f, data[1]/65536.f, data[2]/65536.f);
	MPL_LOGI("Accuracy= %d\r\n", accuracy);
}
#endif

#if 0
/* Handle sensor on/off combinations. */
static void setup_gyro(void)
{
    unsigned char mask = 0, lp_accel_was_on = 0;
    if (hal.sensors & ACCEL_ON)
        mask |= INV_XYZ_ACCEL;
    if (hal.sensors & GYRO_ON) {
        mask |= INV_XYZ_GYRO;
        lp_accel_was_on |= hal.lp_accel_mode;
    }
#ifdef COMPASS_ENABLED
    if (hal.sensors & COMPASS_ON) {
        mask |= INV_XYZ_COMPASS;
        lp_accel_was_on |= hal.lp_accel_mode;
    }
#endif
    /* If you need a power transition, this function should be called with a
     * mask of the sensors still enabled. The driver turns off any sensors
     * excluded from this mask.
     */
    mpu_set_sensors(mask);
    mpu_configure_fifo(mask);
    if (lp_accel_was_on) {
        unsigned short rate;
        hal.lp_accel_mode = 0;
        /* Switching out of LP accel, notify MPL of new accel sampling rate. */
        mpu_get_sample_rate(&rate);
        inv_set_accel_sample_rate(1000000L / rate);
    }
}
#endif

#if 1
static void tap_cb(unsigned char direction, unsigned char count)
{
//    printf("direction %d count %d\n",direction,count);
    switch (direction) {
    case TAP_X_UP:
        MPL_LOGI("Tap X+ ");
        break;
    case TAP_X_DOWN:
        MPL_LOGI("Tap X- ");
        break;
    case TAP_Y_UP:
        MPL_LOGI("Tap Y+ ");
        break;
    case TAP_Y_DOWN:
        MPL_LOGI("Tap Y- ");
        break;
    case TAP_Z_UP:
        MPL_LOGI("Tap Z+ ");
        break;
    case TAP_Z_DOWN:
        MPL_LOGI("Tap Z- ");
        break;
    default:
        return;
    }
    MPL_LOGI("x%d\n", count);
    return;
}

static void android_orient_cb(unsigned char orientation)
{
//    printf("android_orient_cb orientatio %d\n",
//           orientation);
	switch (orientation) {
	case ANDROID_ORIENT_PORTRAIT:
        MPL_LOGI("Portrait\n");
        break;
	case ANDROID_ORIENT_LANDSCAPE:
        MPL_LOGI("Landscape\n");
        break;
	case ANDROID_ORIENT_REVERSE_PORTRAIT:
        MPL_LOGI("Reverse Portrait\n");
        break;
	case ANDROID_ORIENT_REVERSE_LANDSCAPE:
        MPL_LOGI("Reverse Landscape\n");
        break;
	default:
		return;
	}
}
#endif

int set_self_test_reg(GYRO_CAL_DAT *pstTmp)
{
    printf("set test reg (%d %d %d %d %d %d)\n",
           pstTmp->gyro[0],pstTmp->gyro[1],pstTmp->gyro[2],
           pstTmp->accel[0],pstTmp->accel[1],pstTmp->accel[2]);

    mpu_set_gyro_bias_reg(pstTmp->gyro);
#if defined (MPU6500) || defined (MPU9250)
    mpu_set_accel_bias_6500_reg(pstTmp->accel);
#elif defined (MPU6050) || defined (MPU9150)
    mpu_set_accel_bias_6050_reg(pstTmp->accel);
#endif
    return 0;
}

#if 1
int run_self_test(GYRO_CAL_DAT *pstTmp)
{
    int result;
    int gyro[3], accel[3];

    printf("run_self_test \n");
#if defined (MPU6500) || defined (MPU9250)
    result = mpu_run_6500_self_test(gyro, accel, 1);
#elif defined (MPU6050) || defined (MPU9150)
    result = mpu_run_self_test(gyro, accel);
#endif
    printf("new result is 0x%x\n", result);
    if (result == 0x7) {
	    MPL_LOGI("Passed!\n");
//        printf("accel: %7.4f %7.4f %7.4f\n",
//                    accel[0]/65536.f,
//                    accel[1]/65536.f,
//                    accel[2]/65536.f);
//        printf("gyro: %7.4f %7.4f %7.4f\n",
//                    gyro[0]/65536.f,
//                    gyro[1]/65536.f,
//                    gyro[2]/65536.f);

//        pstTmp->gyro_x = (float)(gyro[0]/65536.f);
//        pstTmp->gyro_y = (float)(gyro[1]/65536.f);
//        pstTmp->gyro_z = (float)(gyro[2]/65536.f);
//        pstTmp->acc_x = (float)(accel[0]/65536.f);
//        pstTmp->acc_y = (float)(accel[1]/65536.f);
//        pstTmp->acc_z = (float)(accel[2]/65536.f);
        /* Test passed. We can trust the gyro data here, so now we need to update calibrated data*/

#ifdef USE_CAL_HW_REGISTERS
        /*
         * This portion of the code uses the HW offset registers that are in the MPUxxxx devices
         * instead of pushing the cal data to the MPL software library
         */
        unsigned char i = 0;
        printf("cal hw reg\n");
        for(i = 0; i<3; i++) {
        	gyro[i] = (int)(gyro[i] * 32.8f); //convert to +-1000dps
        	accel[i] *= 2048.f; //convert to +-16G
        	accel[i] = accel[i] >> 16;
        	gyro[i] = (int)(gyro[i] >> 16);
        }
        printf("test reg (%d %d %d %d %d %d)\n",
           gyro[0],gyro[1],gyro[2],
           accel[0],accel[1],accel[2]);

        memcpy((char *)pstTmp->gyro,
        (char *)gyro,sizeof(gyro));
        memcpy((char *)pstTmp->accel,(char *)accel,sizeof(accel));
        printf("get test reg (%d %d %d %d %d %d)\n",
           pstTmp->gyro[0],pstTmp->gyro[1],pstTmp->gyro[2],
           pstTmp->accel[0],pstTmp->accel[1],pstTmp->accel[2]);

        mpu_set_gyro_bias_reg(gyro);
#if defined (MPU6500) || defined (MPU9250)
        mpu_set_accel_bias_6500_reg(accel);
#elif defined (MPU6050) || defined (MPU9150)
        mpu_set_accel_bias_6050_reg(accel);
#endif
#else
        /* Push the calibrated data to the MPL library.
         *
         * MPL expects biases in hardware units << 16, but self test returns
		 * biases in g's << 16.
		 */
    	unsigned short accel_sens;
    	float gyro_sens;

		mpu_get_accel_sens(&accel_sens);
		accel[0] *= accel_sens;
		accel[1] *= accel_sens;
		accel[2] *= accel_sens;
		inv_set_accel_bias(accel, 3);
		mpu_get_gyro_sens(&gyro_sens);
		gyro[0] = (int) (gyro[0] * gyro_sens);
		gyro[1] = (int) (gyro[1] * gyro_sens);
		gyro[2] = (int) (gyro[2] * gyro_sens);
		inv_set_gyro_bias(gyro, 3);
#endif
    }
    else {
            if (!(result & 0x1))
                MPL_LOGE("Gyro failed.\n");
            if (!(result & 0x2))
                MPL_LOGE("Accel failed.\n");
            if (!(result & 0x4))
                MPL_LOGE("Compass failed.\n");
     }
    return result;
}
#endif
#if 0
static void handle_input(void)
{
  
    char c = USART_ReceiveData(USART2);

    switch (c) {
    /* These commands turn off individual sensors. */
    case '8':
        hal.sensors ^= ACCEL_ON;
        setup_gyro();
        if (!(hal.sensors & ACCEL_ON))
            inv_accel_was_turned_off();
        break;
    case '9':
        hal.sensors ^= GYRO_ON;
        setup_gyro();
        if (!(hal.sensors & GYRO_ON))
            inv_gyro_was_turned_off();
        break;
#ifdef COMPASS_ENABLED
    case '0':
        hal.sensors ^= COMPASS_ON;
        setup_gyro();
        if (!(hal.sensors & COMPASS_ON))
            inv_compass_was_turned_off();
        break;
#endif
    /* The commands send individual sensor data or fused data to the PC. */
    case 'a':
        hal.report ^= PRINT_ACCEL;
        break;
    case 'g':
        hal.report ^= PRINT_GYRO;
        break;
#ifdef COMPASS_ENABLED
    case 'c':
        hal.report ^= PRINT_COMPASS;
        break;
#endif
    case 'e':
        hal.report ^= PRINT_EULER;
        break;
    case 'r':
        hal.report ^= PRINT_ROT_MAT;
        break;
    case 'q':
        hal.report ^= PRINT_QUAT;
        break;
    case 'h':
        hal.report ^= PRINT_HEADING;
        break;
    case 'i':
        hal.report ^= PRINT_LINEAR_ACCEL;
        break;
    case 'o':
        hal.report ^= PRINT_GRAVITY_VECTOR;
        break;
#ifdef COMPASS_ENABLED
	case 'w':
		send_status_compass();
		break;
#endif
    /* This command prints out the value of each gyro register for debugging.
     * If logging is disabled, this function has no effect.
     */
    case 'd':
        mpu_reg_dump();
        break;
    /* Test out low-power accel mode. */
    case 'p':
        if (hal.dmp_on)
            /* LP accel is not compatible with the DMP. */
            break;
        mpu_lp_accel_mode(20);
        /* When LP accel mode is enabled, the driver automatically configures
         * the hardware for latched interrupts. However, the MCU sometimes
         * misses the rising/falling edge, and the hal.new_gyro flag is never
         * set. To avoid getting locked in this state, we're overriding the
         * driver's configuration and sticking to unlatched interrupt mode.
         *
         * TODO: The MCU supports level-triggered interrupts.
         */
        mpu_set_int_latched(0);
        hal.sensors &= ~(GYRO_ON|COMPASS_ON);
        hal.sensors |= ACCEL_ON;
        hal.lp_accel_mode = 1;
        inv_gyro_was_turned_off();
        inv_compass_was_turned_off();
        break;
    /* The hardware self test can be run without any interaction with the
     * MPL since it's completely localized in the gyro driver. Logging is
     * assumed to be enabled; otherwise, a couple LEDs could probably be used
     * here to display the test results.
     */
    case 't':
        run_self_test();
        /* Let MPL know that contiguity was broken. */
        inv_accel_was_turned_off();
        inv_gyro_was_turned_off();
        inv_compass_was_turned_off();
        break;
    /* Depending on your application, sensor data may be needed at a faster or
     * slower rate. These commands can speed up or slow down the rate at which
     * the sensor data is pushed to the MPL.
     *
     * In this example, the compass rate is never changed.
     */
    case '1':
        if (hal.dmp_on) {
            dmp_set_fifo_rate(10);
            inv_set_quat_sample_rate(100000L);
        } else
            mpu_set_sample_rate(10);
        inv_set_gyro_sample_rate(100000L);
        inv_set_accel_sample_rate(100000L);
        break;
    case '2':
        if (hal.dmp_on) {
            dmp_set_fifo_rate(20);
            inv_set_quat_sample_rate(50000L);
        } else
            mpu_set_sample_rate(20);
        inv_set_gyro_sample_rate(50000L);
        inv_set_accel_sample_rate(50000L);
        break;
    case '3':
        if (hal.dmp_on) {
            dmp_set_fifo_rate(40);
            inv_set_quat_sample_rate(25000L);
        } else
            mpu_set_sample_rate(40);
        inv_set_gyro_sample_rate(25000L);
        inv_set_accel_sample_rate(25000L);
        break;
    case '4':
        if (hal.dmp_on) {
            dmp_set_fifo_rate(50);
            inv_set_quat_sample_rate(20000L);
        } else
            mpu_set_sample_rate(50);
        inv_set_gyro_sample_rate(20000L);
        inv_set_accel_sample_rate(20000L);
        break;
    case '5':
        if (hal.dmp_on) {
            dmp_set_fifo_rate(100);
            inv_set_quat_sample_rate(10000L);
        } else
            mpu_set_sample_rate(100);
        inv_set_gyro_sample_rate(10000L);
        inv_set_accel_sample_rate(10000L);
        break;
	case ',':
        /* Set hardware to interrupt on gesture event only. This feature is
         * useful for keeping the MCU asleep until the DMP detects as a tap or
         * orientation event.
         */
        dmp_set_interrupt_mode(DMP_INT_GESTURE);
        break;
    case '.':
        /* Set hardware to interrupt periodically. */
        dmp_set_interrupt_mode(DMP_INT_CONTINUOUS);
        break;
    case '6':
        /* Toggle pedometer display. */
        hal.report ^= PRINT_PEDO;
        break;
    case '7':
        /* Reset pedometer. */
        dmp_set_pedometer_step_count(0);
        dmp_set_pedometer_walk_time(0);
        break;
    case 'f':
        if (hal.lp_accel_mode)
            /* LP accel is not compatible with the DMP. */
            return;
        /* Toggle DMP. */
        if (hal.dmp_on) {
            unsigned short dmp_rate;
            unsigned char mask = 0;
            hal.dmp_on = 0;
            mpu_set_dmp_state(0);
            /* Restore FIFO settings. */
            if (hal.sensors & ACCEL_ON)
                mask |= INV_XYZ_ACCEL;
            if (hal.sensors & GYRO_ON)
                mask |= INV_XYZ_GYRO;
            if (hal.sensors & COMPASS_ON)
                mask |= INV_XYZ_COMPASS;
            mpu_configure_fifo(mask);
            /* When the DMP is used, the hardware sampling rate is fixed at
             * 200Hz, and the DMP is configured to downsample the FIFO output
             * using the function dmp_set_fifo_rate. However, when the DMP is
             * turned off, the sampling rate remains at 200Hz. This could be
             * handled in inv_mpu.c, but it would need to know that
             * inv_mpu_dmp_motion_driver.c exists. To avoid this, we'll just
             * put the extra logic in the application layer.
             */
            dmp_get_fifo_rate(&dmp_rate);
            mpu_set_sample_rate(dmp_rate);
            inv_quaternion_sensor_was_turned_off();
            MPL_LOGI("DMP disabled.\n");
        } else {
            unsigned short sample_rate;
            hal.dmp_on = 1;
            /* Preserve current FIFO rate. */
            mpu_get_sample_rate(&sample_rate);
            dmp_set_fifo_rate(sample_rate);
            inv_set_quat_sample_rate(1000000L / sample_rate);
            mpu_set_dmp_state(1);
            MPL_LOGI("DMP enabled.\n");
        }
        break;
    case 'm':
        /* Test the motion interrupt hardware feature. */
	#ifndef MPU6050 // not enabled for 6050 product
	hal.motion_int_mode = 1;
	#endif 
        break;

    case 'v':
        /* Toggle LP quaternion.
         * The DMP features can be enabled/disabled at runtime. Use this same
         * approach for other features.
         */
        hal.dmp_features ^= DMP_FEATURE_6X_LP_QUAT;
        dmp_enable_feature(hal.dmp_features);
        if (!(hal.dmp_features & DMP_FEATURE_6X_LP_QUAT)) {
            inv_quaternion_sensor_was_turned_off();
            MPL_LOGI("LP quaternion disabled.\n");
        } else
            MPL_LOGI("LP quaternion enabled.\n");
        break;
    default:
        break;
    }
    hal.rx.cmd = 0;
}
#endif

/* Every time new gyro data is available, this function is called in an
 * ISR context. In this example, it sets a flag protecting the FIFO read
 * function.
 */
void gyro_data_ready_cb(void)
{
    hal.new_gyro = 1;
}
/*******************************************************************************/

/**
  * @brief main entry point.
  * @par Parameters None
  * @retval void None
  * @par Required preconditions: None
  */

int start_read_gyro(GYRO_DAT *pstTmp)
{
    int ret = -1;
    short old_gyro[3];
    short old_accel[3];

#ifdef COMPASS_ENABLED
    unsigned char new_compass = 0;
#endif

    unsigned long long sensor_timestamp;
    unsigned char new_temp = 0;
    int new_data = 0;

//    hal.new_gyro = 1;
    if (!hal.sensors || !hal.new_gyro) {
//        ins_delay_ms(1000/DEFAULT_MPU_HZ);
//        ins_delay_ms(10);
        ins_delay_ms(1);
        hal.new_gyro = 1;
    }

//#ifdef COMPASS_ENABLED
//    /* We're not using a data ready interrupt for the compass, so we'll
//         * make our compass reads timer-based instead.
//         */
//        if ((timestamp > hal.next_compass_ms) && !hal.lp_accel_mode &&
//            hal.new_gyro && (hal.sensors & COMPASS_ON)) {
//            hal.next_compass_ms = timestamp + COMPASS_READ_MS;
//            new_compass = 1;
//        }
//#endif
    /* Temperature data doesn't need to be read with every gyro sample.
     * Let's make them timer-based like the compass reads.
     */
#ifdef OPEN_TEMP
    unsigned long long timestamp;
    nv_get_clock_ms(&timestamp);
    if (timestamp > hal.next_temp_ms) {
        hal.next_temp_ms = timestamp + TEMP_READ_MS;
        new_temp = 1;
    }
#endif

    if (hal.new_gyro && hal.lp_accel_mode) {
        short accel_short[3];
        int accel[3];
        mpu_get_accel_reg(accel_short, &sensor_timestamp);
        accel[0] = (int)accel_short[0];
        accel[1] = (int)accel_short[1];
        accel[2] = (int)accel_short[2];
        inv_build_accel(accel, 0, sensor_timestamp);
        new_data = 1;
        hal.new_gyro = 0;
    } else if (hal.new_gyro && hal.dmp_on) {
        short gyro[3], accel_short[3], sensors;
        unsigned char more;
        int accel[3], quat[4], temperature;
        /* This function gets new data from the FIFO when the DMP is in
         * use. The FIFO can contain any combination of gyro, accel,
         * quaternion, and gesture data. The sensors parameter tells the
         * caller which data fields were actually populated with new data.
         * For example, if sensors == (INV_XYZ_GYRO | INV_WXYZ_QUAT), then
         * the FIFO isn't being filled with accel data.
         * The driver parses the gesture data to determine if a gesture
         * event has occurred; on an event, the application will be notified
         * via a callback (assuming that a callback function was properly
         * registered). The more parameter is non-zero if there are
         * leftover packets in the FIFO.
         */

//        printf("org gyro %d %d %d\n",
//               test_data[0],
//               test_data[1],
//               test_data[2]);

//        printf("fifo gyro %f %f %f\n",
//               (float)test_data[0]/divide_gyro,
//               (float)test_data[1]/divide_gyro,
//               (float)test_data[2]/divide_gyro);

        mpu_get_gyro_reg(old_gyro,0);
        mpu_get_accel_reg(old_accel,0);
        dmp_read_fifo(gyro, accel_short, quat, &sensor_timestamp, &sensors, &more);

        pstTmp->org_gyro_x = (float)old_gyro[0]/divide_gyro;
        pstTmp->org_gyro_y = (float)old_gyro[1]/divide_gyro;
        pstTmp->org_gyro_z = (float)old_gyro[2]/divide_gyro;
        pstTmp->org_accel_x = (float)old_accel[0]/divide_accel;
        pstTmp->org_accel_y = (float)old_accel[1]/divide_accel;
        pstTmp->org_accel_z = (float)old_accel[2]/divide_accel;

//        printf("raw %f %f %f %f %f %f\n",
//               pstTmp->org_gyro_x,
//               pstTmp->org_gyro_y,
//               pstTmp->org_gyro_z,
//               pstTmp->org_accel_x,
//        pstTmp->org_accel_y,
//        pstTmp->org_accel_z);

        pstTmp->raw_timestep = (long long)sensor_timestamp;
//            printf("more %d sensors 0x%x "
//                           "gyro 0x%x 0x%x 0x%x "
//                           "accel_short 0x%x 0x%x 0x%x\n",
//                   more,sensors,gyro[0],gyro[1],gyro[2] ,
//                   accel_short[0],accel_short[1],accel_short[2]);

#ifdef COMPASS_ENABLED
        /* We're not using a data ready interrupt for the compass, so we'll
         * make our compass reads timer-based instead.
         */
        if ((pstTmp->raw_timestep > hal.next_compass_ms) && !hal.lp_accel_mode &&
            hal.new_gyro && (hal.sensors & COMPASS_ON)) {
            hal.next_compass_ms = pstTmp->raw_timestep + COMPASS_READ_MS;
            new_compass = 1;
        }
#endif

        if (!more)
            hal.new_gyro = 0;

        if (sensors & INV_XYZ_GYRO) {
            /* Push the new data to the MPL. */
            inv_build_gyro(gyro, sensor_timestamp);
//                printf("3gyro %d %d %d new_temp %d\n",
//                       gyro[0],gyro[1],gyro[2],new_temp);
#ifdef ENABLE_DEBUG
            printf("org gyro %d %d %d\n",
                   gyro[0],
                   gyro[1],
                   gyro[2]);
            printf("gyro %f %f %f\n",
                   (float)gyro[0]/divide_gyro,
                   (float)gyro[1]/divide_gyro,
                   (float)gyro[2]/divide_gyro);
#endif
            pstTmp->gyro_x = (float)gyro[0]/divide_gyro;
            pstTmp->gyro_y = (float)gyro[1]/divide_gyro;
            pstTmp->gyro_z = (float)gyro[2]/divide_gyro;

            new_data = 1;
            if (new_temp) {
                new_temp = 0;
                /* Temperature only used for gyro temp comp. */
                mpu_get_temperature(&temperature, &sensor_timestamp);
                inv_build_temp(temperature, sensor_timestamp);
            }
        }
        if (sensors & INV_XYZ_ACCEL) {
            accel[0] = (int)accel_short[0];
            accel[1] = (int)accel_short[1];
            accel[2] = (int)accel_short[2];
            inv_build_accel(accel, 0, sensor_timestamp);
#ifdef ENABLE_DEBUG
            printf("org accel %d %d %d\n",
                   accel[0],
                   accel[1],
                   accel[2]);
            printf("accel %f %f %f\n",(float)accel[0]/divide_accel,
                   (float)accel[1]/divide_accel,
                   (float)accel[2]/divide_accel);
#endif
            pstTmp->acc_x = (float)accel[0]/divide_accel;
            pstTmp->acc_y = (float)accel[1]/divide_accel;
            pstTmp->acc_z = (float)accel[2]/divide_accel;

            new_data = 1;
        }
        if (sensors & INV_WXYZ_QUAT) {
            inv_build_quat(quat, 0, sensor_timestamp);
            new_data = 1;
        }
    } else if (hal.new_gyro) {
        short gyro[3], accel_short[3];
        unsigned char sensors, more;
        int accel[3], temperature;
        /* This function gets new data from the FIFO. The FIFO can contain
         * gyro, accel, both, or neither. The sensors parameter tells the
         * caller which data fields were actually populated with new data.
         * For example, if sensors == INV_XYZ_GYRO, then the FIFO isn't
         * being filled with accel data. The more parameter is non-zero if
         * there are leftover packets in the FIFO. The HAL can use this
         * information to increase the frequency at which this function is
         * called.
         */
        hal.new_gyro = 0;
        mpu_read_fifo(gyro, accel_short, &sensor_timestamp,
                      &sensors, &more);

        printf("2more %d sensors 0x%x gyro 0x%x 0x%x 0x%x accel_short 0x%x 0x%x 0x%x\n",
               more,sensors,gyro[0],gyro[1],gyro[2] ,
               accel_short[0],accel_short[1],accel_short[2]);

        if (more)
            hal.new_gyro = 1;
        if (sensors & INV_XYZ_GYRO) {
            /* Push the new data to the MPL. */
            inv_build_gyro(gyro, sensor_timestamp);
            new_data = 1;
            if (new_temp) {
                new_temp = 0;
                /* Temperature only used for gyro temp comp. */
                mpu_get_temperature(&temperature, &sensor_timestamp);
                inv_build_temp(temperature, sensor_timestamp);
            }
        }
        if (sensors & INV_XYZ_ACCEL) {
            accel[0] = (int)accel_short[0];
            accel[1] = (int)accel_short[1];
            accel[2] = (int)accel_short[2];
            inv_build_accel(accel, 0, sensor_timestamp);
            new_data = 1;
        }
    }
#ifdef COMPASS_ENABLED
    if (new_compass) {
            short compass_short[3];
            int compass[3];
            new_compass = 0;
            /* For any MPU device with an AKM on the auxiliary I2C bus, the raw
             * magnetometer registers are copied to special gyro registers.
             */
            if (!mpu_get_compass_reg(compass_short, &sensor_timestamp)) {
                compass[0] = (int)compass_short[0];
                compass[1] = (int)compass_short[1];
                compass[2] = (int)compass_short[2];
                /* NOTE: If using a third-party compass calibration library,
                 * pass in the compass data in uT * 2^16 and set the second
                 * parameter to INV_CALIBRATED | acc, where acc is the
                 * accuracy from 0 to 3.
                 */
                inv_build_compass(compass, 0, sensor_timestamp);
                printf("get compass %d %d %d\n",compass[0],compass[1],compass[2]);
            }
            new_data = 1;
        }
#endif
//      printf("new data is %d\n",new_data);
    if (new_data) {
        if(inv_execute_on_data()) {
            printf("inv_execute_on_data error\n");
//            MPL_LOGE("ERROR execute on data\n");
        }
        else
        {
            /* This function reads bias-compensated sensor data and sensor
             * fusion outputs from the MPL. The outputs are formatted as seen
             * in eMPL_outputs.c. This function only needs to be called at the
             * rate requested by the host.
             */
            read_from_mpl(pstTmp);
#ifdef ENABLE_DEBUG
            printf("timestep %lld %lld %llu \n",pstTmp->raw_timestep,
            pstTmp->quat_timestep,
            sensor_timestamp);
#endif
            ret = 0;
        }
    }
#if 0
    if(ret == 0)
    {
        printf("2lib %f %f %f %f %f %f ret %d \n",
               pstTmp->org_gyro_x,
               pstTmp->org_gyro_y,
               pstTmp->org_gyro_z,
               pstTmp->org_accel_x,
               pstTmp->org_accel_y,
               pstTmp->org_accel_z,ret);
//        printf("%d %d %d %d %d %d\n",
//               old_gyro[0],old_gyro[1],
//               old_gyro[2], old_accel[0],old_accel[1],old_accel[2]);
    }
#endif
    return ret;
}

int start_reset_fifo(void)
{
    int ret = mpu_reset_fifo();
    printf("mpu_reset_fifo ret %d\n",ret);
    if(ret != 0)
    {
        printf("reset fifo error\n");
    }
    return ret;
}

int start_mpu_deinit(void)
{
    ins_i2c_close();
    return 0;
}

int start_mpu_init(int bReset)
{
    inv_error_t result;
    unsigned char accel_fsr;
    unsigned short gyro_rate, gyro_fsr;
//    unsigned long long timestamp;
    struct int_param_s int_param;
#ifdef COMPASS_ENABLED
    unsigned short compass_fsr;
#endif
    result = mpu_init(&int_param, bReset);
    if (result) {
      MPL_LOGE("Could not initialize gyro.\n");
    }
    /* If you're not using an MPU9150 AND you're not using DMP features, this
     * function will place all slaves on the primary bus.
     * mpu_set_bypass(1);
     */
    result = inv_init_mpl();
//    printf("inv_init_mpl result %d\n",result);
    if (result) {
      MPL_LOGE("Could not initialize MPL.\n");
    }
    /* Compute 6-axis and 9-axis quaternions. */
    inv_enable_quaternion();
    inv_enable_9x_sensor_fusion();
    /* The MPL expects compass data at a constant rate (matching the rate
     * passed to inv_set_compass_sample_rate). If this is an issue for your
     * application, call this function, and the MPL will depend on the
     * timestamps passed to inv_build_compass instead.
     *
     * inv_9x_fusion_use_timestamps(1);
     */

    /* This function has been deprecated.
     * inv_enable_no_gyro_fusion();
     */

    /* Update gyro biases when not in motion.
     * WARNING: These algorithms are mutually exclusive.
     */
    inv_enable_fast_nomot();
    /* inv_enable_motion_no_motion(); */
    /* inv_set_no_motion_time(1000); */

    /* Update gyro biases when temperature changes. */
    inv_enable_gyro_tc();

    /* This algorithm updates the accel biases when in motion. A more accurate
     * bias measurement can be made when running the self-test (see case 't' in
     * handle_input), but this algorithm can be enabled if the self-test can't
     * be executed in your application.
     *
     * inv_enable_in_use_auto_calibration();
     */
#ifdef COMPASS_ENABLED
    printf("compass enabled\n");
    /* Compass calibration algorithms. */
    inv_enable_vector_compass_cal();
    inv_enable_magnetic_disturbance();
#endif

    /* If you need to estimate your heading before the compass is calibrated,
     * enable this algorithm. It becomes useless after a good figure-eight is
     * detected, so we'll just leave it out to save memory.
     * inv_enable_heading_from_gyro();
     */

    /* Allows use of the MPL APIs in read_from_mpl. */
    inv_enable_eMPL_outputs();
    result = inv_start_mpl();
//    printf("inv_start_mpl result %d \n",result);
    if (result == INV_ERROR_NOT_AUTHORIZED) {
//      while (1) {
          printf("Not authorized.\n");
//      }
    }
    if (result) {
      printf("Could not start the MPL.\n");
    }

    /* Get/set hardware configuration. Start gyro. */
    /* Wake up all sensors. */
#ifdef COMPASS_ENABLED
    mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL | INV_XYZ_COMPASS);
#else
    mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
#endif
    /* Push both gyro and accel data into the FIFO. */
//    printf("INV_XYZ_GYRO 0x%x INV_XYZ_ACCEL 0x%x\n",
//    INV_XYZ_GYRO,INV_XYZ_ACCEL);

    mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    mpu_set_sample_rate(DEFAULT_MPU_HZ);
#ifdef COMPASS_ENABLED
    /* The compass sampling rate can be less than the gyro/accel sampling rate.
     * Use this function for proper power management.
     */
    mpu_set_compass_sample_rate(1000 / COMPASS_READ_MS);
#endif
    /* Read back configuration in case it was set improperly. */
    mpu_get_sample_rate(&gyro_rate);
    mpu_get_gyro_fsr(&gyro_fsr);
    mpu_get_accel_fsr(&accel_fsr);
#ifdef COMPASS_ENABLED
    mpu_get_compass_fsr(&compass_fsr);
#endif

    printf("get gyro rate %d "
                   "gyro_fsr %d accel_fsr %d\n",
           gyro_rate,gyro_fsr,accel_fsr);
    /* Sync driver configuration with MPL. */
    /* Sample rate expected in microseconds. */
    inv_set_gyro_sample_rate(1000000L / gyro_rate);
    inv_set_accel_sample_rate(1000000L / gyro_rate);
#ifdef COMPASS_ENABLED
    /* The compass rate is independent of the gyro and accel rates. As long as
     * inv_set_compass_sample_rate is called with the correct value, the 9-axis
     * fusion algorithm's compass correction gain will work properly.
     */
    inv_set_compass_sample_rate(COMPASS_READ_MS * 1000L);
#endif
    /* Set chip-to-body orientation matrix.
     * Set hardware units to dps/g's/degrees scaling factor.
     */
    inv_set_gyro_orientation_and_scale(
            inv_orientation_matrix_to_scalar(gyro_pdata.orientation),
            (int)gyro_fsr<<15);

    inv_set_accel_orientation_and_scale(
            inv_orientation_matrix_to_scalar(gyro_pdata.orientation),
            (int)accel_fsr<<15);

#ifdef COMPASS_ENABLED
    inv_set_compass_orientation_and_scale(
            inv_orientation_matrix_to_scalar(compass_pdata.orientation),
            (int)compass_fsr<<15);
#endif
    /* Initialize HAL state variables. */
#ifdef COMPASS_ENABLED
    hal.sensors = ACCEL_ON | GYRO_ON | COMPASS_ON;
#else
    hal.sensors = ACCEL_ON | GYRO_ON;
#endif
    hal.dmp_on = 0;
    hal.report = 0;
    hal.rx.cmd = 0;
    hal.next_pedo_ms = 0;
    hal.next_compass_ms = 0;
    hal.next_temp_ms = 0;

  /* Compass reads are handled by scheduler. */
//    nv_get_clock_ms(&timestamp);

    /* To initialize the DMP:
     * 1. Call dmp_load_motion_driver_firmware(). This pushes the DMP image in
     *    inv_mpu_dmp_motion_driver.h into the MPU memory.
     * 2. Push the gyro and accel orientation matrix to the DMP.
     * 3. Register gesture callbacks. Don't worry, these callbacks won't be
     *    executed unless the corresponding feature is enabled.
     * 4. Call dmp_enable_feature(mask) to enable different features.
     * 5. Call dmp_set_fifo_rate(freq) to select a DMP output rate.
     * 6. Call any feature-specific control functions.
     *
     * To enable the DMP, just call mpu_set_dmp_state(1). This function can
     * be called repeatedly to enable and disable the DMP at runtime.
     *
     * The following is a short summary of the features supported in the DMP
     * image provided in inv_mpu_dmp_motion_driver.c:
     * DMP_FEATURE_LP_QUAT: Generate a gyro-only quaternion on the DMP at
     * 200Hz. Integrating the gyro data at higher rates reduces numerical
     * errors (compared to integration on the MCU at a lower sampling rate).
     * DMP_FEATURE_6X_LP_QUAT: Generate a gyro/accel quaternion on the DMP at
     * 200Hz. Cannot be used in combination with DMP_FEATURE_LP_QUAT.
     * DMP_FEATURE_TAP: Detect taps along the X, Y, and Z axes.
     * DMP_FEATURE_ANDROID_ORIENT: Google's screen rotation algorithm. Triggers
     * an event at the four orientations where the screen should rotate.
     * DMP_FEATURE_GYRO_CAL: Calibrates the gyro data after eight seconds of
     * no motion.
     * DMP_FEATURE_SEND_RAW_ACCEL: Add raw accelerometer data to the FIFO.
     * DMP_FEATURE_SEND_RAW_GYRO: Add raw gyro data to the FIFO.
     * DMP_FEATURE_SEND_CAL_GYRO: Add calibrated gyro data to the FIFO. Cannot
     * be used in combination with DMP_FEATURE_SEND_RAW_GYRO.
     */
    if (dmp_load_motion_driver_firmware() != 0) {
            printf("Could not download DMP.\n");
        return -1;
    }
    dmp_set_orientation(
        inv_orientation_matrix_to_scalar(gyro_pdata.orientation));

    dmp_register_tap_cb(tap_cb);

    dmp_register_android_orient_cb(android_orient_cb);

    /*
     * Known Bug -
     * DMP when enabled will sample sensor data at 200Hz and output to FIFO at the rate
     * specified in the dmp_set_fifo_rate API. The DMP will then sent an interrupt once
     * a sample has been put into the FIFO. Therefore if the dmp_set_fifo_rate is at 25Hz
     * there will be a 25Hz interrupt from the MPU device.
     *
     * There is a known issue in which if you do not enable DMP_FEATURE_TAP
     * then the interrupts will be at 200Hz even if fifo rate
     * is set at a different rate. To avoid this issue include the DMP_FEATURE_TAP
     *
     * DMP sensor fusion works only with gyro at +-2000dps and accel +-2G
     */
#if 1
    hal.dmp_features = DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_TAP |
        DMP_FEATURE_ANDROID_ORIENT | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
        DMP_FEATURE_GYRO_CAL;
#else
    hal.dmp_features = DMP_FEATURE_6X_LP_QUAT  | DMP_FEATURE_SEND_RAW_ACCEL | DMP_FEATURE_SEND_CAL_GYRO |
                       DMP_FEATURE_GYRO_CAL;
#endif

    dmp_enable_feature(hal.dmp_features);
    dmp_set_fifo_rate(DEFAULT_MPU_HZ);

    mpu_set_dmp_state(1);
    hal.dmp_on = 1;
//    hal.report = (PRINT_GRAVITY_VECTOR | PRINT_LINEAR_ACCEL);
    hal.report = (PRINT_GRAVITY_VECTOR | PRINT_ACCEL | PRINT_GYRO );
//    printf("gyro new init over hal.motion_int_mode %d "
//                   "hal.sensors %d hal.new_gyro %d\n",
//           hal.motion_int_mode,
//           hal.sensors,
//           hal.new_gyro);
//    printf("read 5\n");
//    mpu_read_6500_gyro_bias();
    printf("init gyro suc %s hal.dmp_features 0x%x\n",
           __DATE__,hal.dmp_features);
#if 0
    short test_data[3];
    while(1)
    {
        ins_delay_ms(1000/DEFAULT_MPU_HZ);
        mpu_get_gyro_reg(test_data,0);
        printf("org gyro %d %d %d\n",
               test_data[0],
               test_data[1],
               test_data[2]);
        printf("gyro %f %f %f\n",
               (float)test_data[0]/divide_gyro,
               (float)test_data[1]/divide_gyro,
               (float)test_data[2]/divide_gyro);

        mpu_get_accel_reg(test_data,0);
        printf("org aceel %d %d %d\n",
               test_data[0],
               test_data[1],
               test_data[2]);
        printf("accel %f %f %f\n",(float)test_data[0]/divide_accel,
               (float)test_data[1]/divide_accel,
               (float)test_data[2]/divide_accel);
    }
    return 0;
#endif

#if 0
//    mpu_reg_dump();

  while(1){
//    if (USART_GetITStatus(USART2, USART_IT_RXNE)) {
//        /* A bytee has been received via USART. See handle_input for a list of
//         * valid commands.
//         */
//        USART_ClearITPendingBit(USART2, USART_IT_RXNE);
//        handle_input();
//    }
#if 0
    if (hal.motion_int_mode) {
        printf("enter hal.motion_int_mode\n");
        /* Enable motion interrupt. */
        mpu_lp_motion_interrupt(500, 1, 5);
        /* Notify the MPL that contiguity was broken. */
        inv_accel_was_turned_off();
        inv_gyro_was_turned_off();
        inv_compass_was_turned_off();
        inv_quaternion_sensor_was_turned_off();
        /* Wait for the MPU interrupt. */
        while (!hal.new_gyro) {}
        /* Restore the previous sensor configuration. */
        mpu_lp_motion_interrupt(0, 0, 0);
        hal.motion_int_mode = 0;
    }

#endif
    if (!hal.sensors || !hal.new_gyro) {
        ins_delay_ms(1000/DEFAULT_MPU_HZ);
//        ins_delay_ms(10);
        hal.new_gyro = 1;
    }
//    GYRO_DAT stTmp;
//    start_read_gyro(&stTmp);
//        printf("hal.new_gyro is %d hal.lp_accel_mode %d"
//                       " hal.sensors 0x%x hal.dmp_on %d\n",
//               hal.new_gyro,hal.lp_accel_mode,hal.sensors,hal.dmp_on);
    }
#endif
    return 0;
}

//int main(int argc,char *argv[])
//{
//    int ret = 0;
//
//    start_mpu_init();
//    return ret;
//}