import os
import platform
import sys
import config


if os.getenv('SDK_ROOT'):
    SDK_ROOT = os.getenv('SDK_ROOT')
else:
    SDK_ROOT = os.path.normpath(os.getcwd())

Export('SDK_ROOT')
Export('config')


com_env = Environment(
	CC = config.CC, CCFLAGS = config.CFLAGS,
	CXX = config.CXX, CXXFLAGS = config.CXXFLAGS,
	LIBS = ['pthread', 'rt', 'ev', 'jsoncpp'],
	LINKFLAGS = config.LDFLAGS,
	CPPPATH = config.CPPPATH,
 	)

com_env.Append(CCCOMSTR  ='CC <============================================ $SOURCES')
com_env.Append(CXXCOMSTR ='CXX <=========================================== $SOURCES')
#com_env.Append(LINKCOMSTR='Link Target $SOURCES')


Export('com_env')

############################# Monitor ######################################
monitor_obj = SConscript('./init/SConscript')

MONITOR_EXE = 'out/monitor'
MONITOR_OBJS = monitor_obj
com_env.Program(target = MONITOR_EXE, source = MONITOR_OBJS)


############################ update_check ##################################
update_check_obj = SConscript('./update_check/SConscript')
com_env.Program('out/update_check', update_check_obj)


############################ update_app ####################################
update_app_obj = SConscript('./update_app/SConscript')
com_env.Program('out/update_app', update_app_obj)


############################ http_server ####################################
#http_server_obj = SConscript('./http_server/SConscript')
#com_env.Program('out/http_server', http_server_obj)
	

############################ bootanimation ##################################
bootan_obj = SConscript('./bootan/SConscript')
com_env.Program('out/bootanimation', bootan_obj)


############################ power_manager ##################################
power_obj = SConscript('./ui_service/power/SConscript')
com_env.Program('./out/power_manager', power_obj)

############################ pwr_ctl ##################################
pwr_obj = SConscript('./ui_service/pwr_ctl/SConscript')
com_env.Program('./out/pwr_ctl', pwr_obj)


############################ ui_service ##################################
ui_service_obj = SConscript('./ui_service/SConscript')
com_env.Program('./out/ui_service', ui_service_obj)


############################ mongoose ##################################
#mongoose_obj = SConscript('./http_file/SConscript')
#com_env.Program('./out/mongoose', mongoose_obj)


############################ time_tz ##################################
#time_tz_obj = SConscript('./time_tz/SConscript')
#com_env.Program('./out/time_tz', time_tz_obj)


############################ kern_log ##################################
#kern_log_obj = SConscript('./kern_log/SConscript')
#com_env.Program('./out/kern_log', kern_log_obj)

