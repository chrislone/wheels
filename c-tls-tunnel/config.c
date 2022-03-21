#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include "deps/cJSON.h"
#include "config.h"

#define NEG_STATUS -1
#define POS_STATUS 0

static FILE *fp_config_json = NULL;
static char *config_json_buf;

static const char *CONFIG_FILE_NAME = "config.json";
static struct stat conf_json_stat_buf;

static char *make_string_copy_pt(char *src) {
    size_t len = strlen(src);
    char *temp = (char *) malloc(sizeof(char) * len + 1);
    memset(temp, 0, sizeof(*temp));
    strcpy(temp, src);
    return temp;
}

int config_init() {
    fp_config_json = fopen(CONFIG_FILE_NAME, "r");
    if (fp_config_json == NULL) {
        return NEG_STATUS;
    }
    int stat_ret = stat("config.json", &conf_json_stat_buf);
    if (stat_ret == -1) {
        return NEG_STATUS;
    }
    size_t size = 0;
    config_json_buf = (char *) malloc(conf_json_stat_buf.st_size * sizeof(char) + 1);
    memset(config_json_buf, 0, sizeof(*config_json_buf));
    do {
        int len = 128;
        char local_buf[len];
        memset(local_buf, 0, sizeof(local_buf));
        size = fread(local_buf, sizeof(char), len, fp_config_json);
        strncat(config_json_buf, local_buf, size);
    } while (size != 0);

    cJSON *cJSON_conf_json = cJSON_Parse(config_json_buf);
    cJSON *cJSON_conf_json_key_file = cJSON_GetObjectItem(cJSON_conf_json, "keyFilePath");
    cJSON *cJSON_conf_json_crt_file = cJSON_GetObjectItem(cJSON_conf_json, "crtFilePath");

    if (!cJSON_conf_json_crt_file->valuestring || !cJSON_conf_json_key_file->valuestring) {
        printf("config.c: invalid crtFilePath or keyFilePath filed\n");
        return NEG_STATUS;
    }

    config_crt_file_path = make_string_copy_pt(cJSON_conf_json_crt_file->valuestring);
    config_key_file_path = make_string_copy_pt(cJSON_conf_json_key_file->valuestring);

    return POS_STATUS;
}

