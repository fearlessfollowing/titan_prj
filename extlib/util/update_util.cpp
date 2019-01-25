#include <common/include_common.h>
#include <hw/battery_interface.h>

#define TAG "update_util"

void str_trim(char* pStr) 
{ 
	char *pTmp = pStr; 

	while (*pStr != '\0') { 
		if (*pStr != ' ' && *pStr != '\r' && *pStr != '\n') { 
			*pTmp++ = *pStr; 
		} 
		++pStr; 
	} 
	*pTmp = '\0'; 
} 



unsigned int bytes_to_int(const u8 *buf)
{
    return (buf[0] << 24 | buf[1] <<16 | buf[2] << 8 | buf[3]);
}

void int_to_bytes(u8 *buf, unsigned int val)
{
    buf[0] = (u8)((val >> 24) &0xff);
    buf[1] = (u8)((val >> 16) &0xff);
    buf[2] = (u8)((val >> 8) &0xff);
    buf[3] = (u8)((val &0xff));
}


int tar_zip(const char *zip_name, const char* dest_path)
{
    int iRet = -1;

    char cmd[512];

    snprintf(cmd,sizeof(cmd),"unzip -o -q %s -d %s", zip_name, dest_path);
    iRet = system(cmd);
    if (iRet == 0) {
        msg_util::sleep_ms(10);
    }
    return iRet;
}

bool gen_file(const char *name, u32 file_size, FILE *fp_read)
{
    u32 gen_file_size = 0;
    u32 read_len;
    u32 write_len;

    u32 max_read_size;
    u8 buf[1024 * 1024];
    bool bRet = false;
    FILE *fp_write = nullptr;

    unlink(name);

    fp_write = fopen(name, "wb+");
    if (fp_write) {
        max_read_size = file_size;
        fseek(fp_write, 0L, SEEK_SET);
        memset(buf, 0, sizeof(buf));
        if (max_read_size > sizeof(buf)) {
            u32 read_bytes = sizeof(buf);
            while ((read_len = fread(buf, 1, read_bytes, fp_read)) > 0) {
                write_len = fwrite(buf, 1, read_len, fp_write);
                if (write_len != read_len) {
                    // force to zero while fwrite error
                    goto EXIT;
                } else {
                    gen_file_size += write_len;
                }
                max_read_size -= read_len;
                if (max_read_size > sizeof(buf)) {
                    read_bytes = sizeof(buf);
                } else {
                    read_bytes = max_read_size;
                }
                memset(buf, 0, sizeof(buf));
            }
        } else {
            read_len = fread(buf, 1, max_read_size, fp_read);
            if (read_len != max_read_size) {
                goto EXIT;
            }
			
            write_len = fwrite(buf, 1, read_len, fp_write);
            if (write_len != read_len) {
                // force to zero while fwrite error
                goto EXIT;
            } else {
                gen_file_size += write_len;
            }
        }

        if (gen_file_size != file_size) {
        } else {
            bRet = true;
            // confirm data written to file
            sync();
            msg_util::sleep_ms(200);
        }
EXIT:
        if (fp_write) {
            fclose(fp_write);
        }
    }
    return bRet;
}


#define ENABLE_BAT_CHECK
bool is_bat_enough()
{
#ifdef __ANDROID__
#ifdef ENABLE_BAT_CHECK
    bool ret = false;
    sp<battery_interface> mBat = sp<battery_interface>(new battery_interface());
    if (mBat == nullptr) {
    } else {
        if (mBat->is_enough() == 0) {
            ret = true;
        }
    }	
    return ret;
	
#else
    return true;
#endif

#else
    return true;
#endif
}
