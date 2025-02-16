#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

// For MSG
struct makemkv_message {
    int code;
    int flags;
    int count;
    char *message;

    makemkv_message() : code(0), flags(0), count(0) {
        message = (char*)malloc(4096);
    }

    ~makemkv_message() {
        free(message);
    }
};

// For PRGC and PRGT
struct makemkv_progress_title {
    int code;
    int id;
    char *name;

    makemkv_progress_title() : code(0), id(0) {
        name = (char*)malloc(1024);
    }

    ~makemkv_progress_title() {
        free(name);
    }
};

// For PRGV
struct makemkv_progress {
    int current;
    int total;
    int max;

    makemkv_progress() : current(0), total(0), max(0) {
        // Nothing here
    }
};

// For DRV
struct makemkv_drive {
    int index;
    int visible;
    int enabled;
    int flags;
    char *name;
    char *disc_name;
    char *device;

    makemkv_drive() : index(0), visible(0), enabled(0), flags(0) {
        name = (char*)malloc(1024);
        disc_name = (char*)malloc(1024);
        device = (char*)malloc(1024);
    }

    ~makemkv_drive() {
        free(name);
        free(disc_name);
        free(device);
    }
};

// For TCOUT
struct makemkv_disc_info {
    int count;

    makemkv_disc_info() : count(0) {
        // Nothing here
    }
};

// For CINFO, TINFO and SINFO
struct makemkv_ts_info {
    int id;
    int code;
    char *value;

    makemkv_ts_info() : id(0), code(0) {
        value = (char*)malloc(1024);
    }

    ~makemkv_ts_info() {
        free(value);
    }
};

int makemkv_scan_msg(makemkv_message &mkv_msg, char **state);
int makemkv_scan_drv(makemkv_drive &mkv_drive, char **state);
int makemkv_scan_tcount(makemkv_disc_info &mkv_disk_info, char **state);
int makemkv_scan_cinfo(makemkv_ts_info &mkv_ts_info, char **state);
int makemkv_scan_tinfo(makemkv_ts_info &mkv_ts_info, char **state);
int makemkv_scan_sinfo(makemkv_ts_info &mkv_ts_info, char **state);
int makemkv_scan_prgv(makemkv_progress &mkv_progress, char **state);
int makemkv_scan_prgc(makemkv_progress_title &mkv_progress_title, char **state);
int makemkv_scan_prgt(makemkv_progress_title &mkv_progress_title, char **state);
int makemkv_get_drive_index(const char *device);
