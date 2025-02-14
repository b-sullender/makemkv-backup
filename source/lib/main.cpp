#include "makemkvcon.h"

char *processString(char *in, char *buffer, int bufSize) {
    int out = 0;
    bool escaped = false;

    while (*in != 0) {
        if (*in == '"') {
            if (escaped) {
                buffer[out++] = *in;
            } else {
                break;
            }
        } else if (*in == '\\') {
            if (escaped) {
                buffer[out++] = *in;
            } else {
                escaped = true;
                in++;
                continue;
            }
        } else if (*in == 't' && escaped) {
            buffer[out++] = '\t';
        } else if (*in == 'n' && escaped) {
            buffer[out++] = '\n';
        } else {
            buffer[out++] = *in;
        }

        if (out >= (bufSize - 1)) {
            break;
        }

        escaped = false;
        in++;
    }

    buffer[out] = 0;

    return in;
}

#define makemkv_scan_int(index, destination) \
    token = strtok_r(NULL, ",", state); \
    if (token == NULL) { \
        return index; \
    } \
    destination = atoi(token);

#define makemkv_scan_string(index, buffer, length) \
    token = strtok_r(NULL, ",", state); \
    if (token == NULL) { \
        return index; \
    } \
    if (*token != '"') { \
        return index; \
    } \
    processString(token + 1, buffer, length);

int makemkv_scan_msg(makemkv_message &mkv_msg, char **state) {
    char *token;

    makemkv_scan_int(0, mkv_msg.code);
    makemkv_scan_int(1, mkv_msg.flags);
    makemkv_scan_int(2, mkv_msg.count);
    makemkv_scan_string(3, mkv_msg.message, 4096);

    return 4;
}

int makemkv_scan_drv(makemkv_drive &mkv_drive, char **state) {
    char *token;

    makemkv_scan_int(0, mkv_drive.index);
    makemkv_scan_int(1, mkv_drive.visible);
    makemkv_scan_int(2, mkv_drive.enabled);
    makemkv_scan_int(3, mkv_drive.flags);
    makemkv_scan_string(4, mkv_drive.name, 1024);
    makemkv_scan_string(5, mkv_drive.disc_name, 1024);
    makemkv_scan_string(6, mkv_drive.device, 1024);

    return 7;
}

int makemkv_scan_tcount(makemkv_disc_info &mkv_disk_info, char **state) {
    char *token;

    makemkv_scan_int(0, mkv_disk_info.count);

    return 1;
}

int makemkv_scan_cinfo(makemkv_ts_info &mkv_ts_info, char **state) {
    char *token;

    makemkv_scan_int(0, mkv_ts_info.id);
    makemkv_scan_int(1, mkv_ts_info.code);
    makemkv_scan_string(2, mkv_ts_info.value, 1024);

    return 3;
}

int makemkv_scan_tinfo(makemkv_ts_info &mkv_ts_info, char **state) {
    return makemkv_scan_cinfo(mkv_ts_info, state);
}

int makemkv_scan_sinfo(makemkv_ts_info &mkv_ts_info, char **state) {
    return makemkv_scan_cinfo(mkv_ts_info, state);
}

int makemkv_scan_prgv(makemkv_progress &mkv_progress, char **state) {
    char *token;

    makemkv_scan_int(0, mkv_progress.current);
    makemkv_scan_int(1, mkv_progress.total);
    makemkv_scan_int(2, mkv_progress.max);

    return 3;
}

int makemkv_scan_prgc(makemkv_progress_title &mkv_progress_title, char **state) {
    char *token;

    makemkv_scan_int(0, mkv_progress_title.code);
    makemkv_scan_int(1, mkv_progress_title.id);
    makemkv_scan_string(2, mkv_progress_title.name, 1024);

    return 3;
}

int makemkv_scan_prgt(makemkv_progress_title &mkv_progress_title, char **state) {
    return makemkv_scan_prgc(mkv_progress_title, state);
}

// Returns the drive index or -1 on error
int makemkv_get_drive_index(const char *device) {
    FILE *pipe = popen("makemkvcon -r --cache=1 --noscan info disc:9999", "r");
    if (!pipe) {
        fprintf(stderr, "%s\n", "Error executing makemkvcon to get drive index");
        return -1;
    }

    char *output = (char*)malloc(4096);

    char *token;
    char *state;
    makemkv_drive mkv_drive;

    int index = -1;

    while (fgets(output, 4096, pipe)) {
        token = strtok_r(output, ":", &state);

        if (token != NULL) {
            if (strcmp(token, "DRV") == 0) {
                makemkv_scan_drv(mkv_drive, &state);
                if (strcmp(mkv_drive.device, device) == 0) {
                    // Found the device
                    index = mkv_drive.index;
                }
            }
        }
    }

    free(output);

    // Close the pipe
    pclose(pipe);

    return index;
}
