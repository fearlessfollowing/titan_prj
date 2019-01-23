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
	LIBS = ['pthread', 'rt', 'ev', 'jsoncpp', 'asound'],
	LINKFLAGS = config.LDFLAGS,
	CPPPATH = config.CPPPATH,
 	)

com_env.Append(CCCOMSTR  ='CC <============================================ $SOURCES')
com_env.Append(CXXCOMSTR ='CXX <=========================================== $SOURCES')
#com_env.Append(LINKCOMSTR='Link Target $SOURCES')

ui_service_env = com_env.Clone()


Export('com_env')
Export('ui_service_env')

############################# Monitor ######################################
monitor_obj = SConscript('./init/SConscript')

MONITOR_EXE = 'out/monitor'
MONITOR_OBJS = monitor_obj
#com_env.Program(target = MONITOR_EXE, source = MONITOR_OBJS)


############################ update_check ##################################
update_check_env = com_env.Clone()
update_check_obj = SConscript('./update_check/SConscript')
update_check_env.Program('out/update_check', update_check_obj)


############################ update_app ####################################
update_app_obj = SConscript('./update_app/SConscript')
com_env.Program('out/update_app', update_app_obj)


############################ http_server ####################################
#http_server_obj = SConscript('./http_server/SConscript')
#com_env.Program('out/http_server', http_server_obj)
	

############################ bootanimation ##################################
#bootan_obj = SConscript('./bootan/SConscript')
#com_env.Program('out/bootanimation', bootan_obj)


############################ power_manager ##################################
power_obj = SConscript('./ui_service/power/SConscript')
com_env.Program('./out/power_manager', power_obj)

############################ pwr_ctl ##################################
#pwr_obj = SConscript('./ui_service/pwr_ctl/SConscript')
#com_env.Program('./out/pwr_ctl', pwr_obj)


############################ ui_service ##################################
ui_service_env = com_env.Clone()
ui_service_env.Append(CXXFLAGS= '-DUSE_TRAN_SEND_MSG')
ui_service_env.Append(LIBS=['sqlite3'])
#ui_service_env.Append(LIBS=['tinyxml2'])
ui_service_obj = SConscript('./ui_service/SConscript')
ui_service_env.Program('./out/ui_service', ui_service_obj)


############################ mongoose ##################################
#mongoose_obj = SConscript('./http_file/SConscript')
#com_env.Program('./out/mongoose', mongoose_obj)


############################ time_tz ##################################
#time_tz_obj = SConscript('./time_tz/SConscript')
#com_env.Program('./out/time_tz', time_tz_obj)


############################ kern_log ##################################
#kern_log_obj = SConscript('./kern_log/SConscript')
#com_env.Program('./out/kern_log', kern_log_obj)



############################ factory_test ##################################
factory_test_obj = SConscript('./ui_service/factory_test/SConscript')
com_env.Program('./out/factory_test', factory_test_obj)