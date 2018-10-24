
#include <common/include_common.h>
#include <update/dbg_util.h>
#include <hw/battery_interface.h>

#define TAG "update_util"


#define ENABLE_BAT_CHECK

//#define ENABLE_DUMP
static bool check_path_access(const char *path,int mode)
{
    bool bRet = false;
    if (access(path, mode) == -1) {
		//Log.e(TAG,"%s acces mode %d fail\n", path, mode);
    } else {
        bRet = true;
    }
    return bRet;
}


#if 0
bool check_path_exist(const char *path)
{
    return check_path_access(path, F_OK);
}
#endif

int create_dir(const char *path)
{
    int ret = 0;
    if (!check_path_exist(path)) {
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "mkdir -p %s", path);
        ret = exec_sh(cmd);
    }
    return ret;
}

bool check_path_r(const char *path)
{
    return check_path_access(path, R_OK);
}

bool check_path_rx(const char *path)
{
    return check_path_access(path, R_OK|X_OK);
}

bool check_path_w(const char *path)
{
    return check_path_access(path, W_OK);
}

bool check_path_rwx(const char *path)
{
    return check_path_access(path, R_OK|X_OK|W_OK);
}

#if 0
int exec_sh(const char *str)
{
    int status = system(str);
    int iRet = -1;

    if (-1 == status) {
	#ifdef __ANDROID__
        Log.e(TAG,"system %s error",str);
	#else
	DBG_ERR("system %s error", str);
	#endif

    } else {
       // printf("exit status value = [0x%x]\n", status);
        if (WIFEXITED(status)) {
            if (0 == WEXITSTATUS(status)) {
//                printf("%s suc.\n",str);
                iRet = 0;
            } else {
		#ifdef __ANDROID__
                Log.e(TAG,"%s fail script exit code: %d\n", str,WEXITSTATUS(status));
		#else
		DBG_ERR("%s fail script exit code: %d\n", str, WEXITSTATUS(status));
		#endif
            }
        } else {
	    #ifdef __ANDROID__
            Log.e(TAG,"exit status %s error  = [%d]\n",str, WEXITSTATUS(status));
	    # else
	    DBG_ERR("exit status %s error = [%d]\n", str, WEXITSTATUS(status));
	    #endif
        }
    }

    return iRet;
}
#endif


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



int move_bin(const char *src,const char *dest)
{
    char cmd[1024];

    snprintf(cmd,sizeof(cmd),"mv %s %s",src,dest);
    return exec_sh(cmd);
}


int chmod_x(const char *name)
{
    char buf[512];

    snprintf(buf,sizeof(buf),"chmod +x %s",name);

    return exec_sh(buf);
}


int chmod_777(const char *name)
{
    char buf[512];

    snprintf(buf,sizeof(buf),"chmod 777 %s",name);
    return exec_sh(buf);
}

int chmod_path_777(const char *name)
{
    char buf[512];

    snprintf(buf,sizeof(buf),"chmod 777 %s -R ",name);

    return exec_sh(buf);
}

int update_path(const char *src, const char *dest)
{
    int iRet = -1;
    char buf[512];
    int iErr = 0;
    int max = 3;

    if (!check_path_r(src)) {
#ifdef __ANDROID__
        Log.e(TAG,"update_path no read access path %s\n",src);
#else
        DBG_ERR("no read access path %s\n",src);
#endif
        goto EXIT;
    } else {
        if (chmod_path_777(src) != 0) {
#ifdef __ANDROID__
            Log.e(TAG, "chmod path 777 %s\n", src);
#else
            DBG_ERR( "chmod path 777 %s\n", src);
#endif
            goto EXIT;
        }
    }

    snprintf(buf,sizeof(buf),"cp -pR %s %s",src,dest);

    //TODO: add bake?
    for (iErr = 0; iErr < max; iErr++) {
        iRet = exec_sh(buf);
        if (iRet == 0) {
            break;
        }
    }
EXIT:
    return iRet;
}

//cp file from sd to built-bin rom
int update_sd_item(const char *src, const char *dest)
{
    int iRet = -1;
    char buf[512];
    int iErr = 0;
    int max = 3;

    if (!check_path_r(src)) {

	#ifdef __ANDROID__
        Log.e(TAG, "no read access file %s\n", src);
	#else
        DBG_ERR("update_sd_item no read access file %s\n", src);
	#endif
        goto EXIT;
    }

    snprintf(buf, sizeof(buf),"cp -pR %s %s", src, dest);
    for (iErr = 0; iErr < max; iErr++) {
        iRet = exec_sh(buf);
        if (iRet == 0) {
            chmod_777(dest);
            break;
        }
    }

EXIT:
    return iRet;
}


int update_item(const char *src, const char *dest)
{
    int iRet = -1;
    char buf[512];
    int iErr = 0;
    int max = 3;

    if (!check_path_r(src)) {
	#ifdef __ANDROID__
        Log.e(TAG, "update_item no read access file %s\n", src);
	#else
        DBG_ERR("no read access file %s\n",src);
	#endif
        goto EXIT;
    } else {
        if (chmod_777(src) != 0) {
		#ifdef __ANDROID__
            Log.e(TAG, "chmod item 777 %s\n", src);
		#else
            DBG_ERR("chmod item 777 %s\n", src);
		#endif
            goto EXIT;
        }
    }
    snprintf(buf, sizeof(buf), "cp -pR %s %s", src, dest);

    for (iErr = 0; iErr < max; iErr++) {
        iRet = exec_sh(buf);
        if (iRet == 0) {
            chmod_777(dest);
            break;
        }
    }

EXIT:
    return iRet;
}

unsigned int bytes_to_int(const u8 *buf)
{
//    printf("0x%x 0x%x 0x%x 0x%x\n",(u32)buf[0]<<24,(u32)buf[1]<<16, (u32)buf[2] << 8,(u32)buf[3]);
    return (buf[0] << 24 | buf[1] <<16 | buf[2] << 8 | buf[3]);
}

void int_to_bytes(u8 *buf, unsigned int val)
{
    buf[0] = (u8)((val >> 24) &0xff);
    buf[1] = (u8)((val >> 16) &0xff);
    buf[2] = (u8)((val >> 8) &0xff);
    buf[3] = (u8)((val &0xff));

//    printf("val 0x%x bytes_to_int 0x%x\n",val,bytes_to_int(buf));
}

void dump_bytes( u8 *buf,u32 len ,const char * str)
{
#ifdef ENABLE_DUMP
    printf("dump %s\n",str);
    for (u32 i = 0; i < len; i++) {
        printf("0x%x ", buf[i]);
        if ((i + 1)%10 == 0) {
            printf("\n");
        }
    }
    printf("\n\n");
#endif
}

int rm_file(const char *name)
{
    char cmd[1024];
    int ret = 0;
    if (check_path_exist(name)) {
        snprintf(cmd, sizeof(cmd),"rm -rf %s", name);
        ret = exec_sh(cmd);
        if (ret != 0) {
        #ifdef __ANDROID__
            Log.e(TAG,"rm file %s error\n",name);
		#else
            DBG_ERR("rm file %s error\n",name);
		#endif
        }
    }
    return ret;
}

void MyPrintf(const char *cmd, ...)
{
    printf("%s %s ", __DATE__, __TIME__);
    va_list args;       //定义一个va_list类型的变量，用来储存单个参数
    va_start(args,cmd); //使args指向可变参数的第一个参数
    vprintf(cmd,args);  //必须用vprintf等带V的
    va_end(args);       //结束可变参数的获取
    printf("\n");
}

int tar_zip(const char *zip_name, const char* dest_path)
{
    int iRet = -1;

    char cmd[512];

    snprintf(cmd,sizeof(cmd),"unzip -o -q %s -d %s", zip_name, dest_path);

    iRet = exec_sh(cmd);
    if (iRet == 0) {
        msg_util::sleep_ms(10);
    }
    return iRet;
}



/*
 * gen_file - 
 */
bool gen_file(const char *name, u32 file_size, FILE *fp_read)
{
    u32 gen_file_size = 0;
    u32 read_len;
    u32 write_len;

    u32 max_read_size;
    u8 buf[1024 * 1024];
    bool bRet = false;
    FILE *fp_write = nullptr;

    if (rm_file(name) != 0) {
        goto REACH_END;
    }

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
				#ifdef __ANDROID__
                    Log.e(TAG, "1 write %s  mismatch(%d %d)\n", name, write_len, read_len);
				#else
                    DBG_ERR("1 write %s  mismatch(%d %d)\n", name,write_len, read_len);
				#endif
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
			#ifdef __ANDROID__
                Log.e(TAG,"2read %s len mismatch(%d %d)\n",name,read_len,max_read_size);
			#else
                DBG_ERR("2read %s len mismatch(%d %d)\n",name,read_len,max_read_size);
			#endif
                goto EXIT;
            }
			
            write_len = fwrite(buf, 1, read_len, fp_write);
            if (write_len != read_len) {
            #ifdef __ANDROID__
                Log.e(TAG,"2write %s mismatch(%d %d)\n",name, write_len, read_len);
			#else
                DBG_ERR("2write %s mismatch(%d %d)\n",name, write_len, read_len);
			#endif
                // force to zero while fwrite error
                goto EXIT;
            } else {
                gen_file_size += write_len;
            }
        }

        if (gen_file_size != file_size) {
		#ifdef __ANDROID__
            Log.e(TAG, "gen %s file size mismatch(%d %d)\n", name, gen_file_size, file_size);
		#else
            DBG_ERR("gen %s file size mismatch(%d %d)\n", name, gen_file_size, file_size);
		#endif
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
REACH_END:
//    DBG_PRINT("gen %s file over bRet %d\n", name,bRet);
    return bRet;
}

int remount_sys()
{
    char cmd[1024];

    snprintf(cmd,sizeof(cmd),"mount -o remount,rw /system");
    return exec_sh(cmd);
}

int start_reboot()
{
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "%s", "reboot");
    return exec_sh(cmd);
}

void start_app_directly()
{
    const char *start_cmd = "sh /system/bin/init.mbx_new.sh &";
    exec_sh(start_cmd);
}

int update_test_itself()
{
    int ret;
#if 1
    printf("2update test new \n");
    const char *src = "/system/bin/update_test_new";
    const char *dest = "/system/bin/update_test";
    if (check_path_exist(src)) {
        ret = move_bin(src,dest);
        printf("update test new2 ret %d\n",ret);
    } else {
        ret  = 0;
        printf("update test new 3\n");
    }
#endif
    return ret;
}



bool is_bat_enough()
{

#ifdef ENABLE_BAT_CHECK

    bool ret = false;
    sp<battery_interface> mBat = sp<battery_interface>(new battery_interface());
    if (mBat == nullptr) {
        Log.e(TAG,"bat creat error\n");
    } else {
        if (mBat->is_enough() == 0) {
            ret = true;
        }
    }
	
    if (!ret) {
        Log.e(TAG,"is_bat_enough false\n");
    }
    return ret;
	
#else
    Log.e(TAG, "disable bat check\n");
    return true;
#endif

}
