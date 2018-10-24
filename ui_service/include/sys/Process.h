#ifndef _PROCESS_H
#define _PROCESS_H

class Process {
public:
    static void killProcessesWithOpenFiles(const char *path, int action);
    static int getPid(const char *s);
    static int checkSymLink(int pid, const char *path, const char *name);
    static int checkFileMaps(int pid, const char *path);
    static int checkFileMaps(int pid, const char *path, char *openFilename, size_t max);
    static int checkFileDescriptorSymLinks(int pid, const char *mountPoint);
    static int checkFileDescriptorSymLinks(int pid, const char *mountPoint, char *openFilename, size_t max);
    static void getProcessName(int pid, char *buffer, size_t max);
private:
    static int readSymLink(const char *path, char *link, size_t max);
    static int pathMatchesMountPoint(const char *path, const char *mountPoint);
};


#endif /* _PROCESS_H */