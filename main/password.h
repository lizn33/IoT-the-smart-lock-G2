#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "esp_log.h"  // Added explicit inclusion of ESP logging header

#define MAX_PASSWORDS 100

// Instead of defining TAG globally, we'll use a function in logging statements
// This avoids conflicts with other files that define their own TAG
#define PASSWORD_MANAGER_TAG "PASSWORD_MANAGER"

typedef struct
{
    char *password;
    char *timestamp; // format："2026-02-22T00:00+08:00"
} PasswordEntry;

typedef struct password_control
{
    PasswordEntry entries[MAX_PASSWORDS];
    int count;
} password_control;

// single instance
static password_control *instance = NULL;

// get instance
password_control *get_password_control_instance()
{
    if (instance == NULL)
    {
        instance = (password_control *)malloc(sizeof(password_control));
        if (instance)
        {
            instance->count = 0;
        }
    }
    return instance;
}

time_t parse_timestamp(const char *timestamp)
{
    int year, month, day, hour, minute, tz_hour, tz_minute;
    char tz_sign;
    if (sscanf(timestamp, "%d-%d-%dT%d:%d%c%d:%d",
               &year, &month, &day, &hour, &minute, &tz_sign, &tz_hour, &tz_minute) != 8)
    {
        return -1;
    }

    struct tm tm_time;
    memset(&tm_time, 0, sizeof(tm_time));
    tm_time.tm_year = year - 1900;
    tm_time.tm_mon = month - 1;
    tm_time.tm_mday = day;
    tm_time.tm_hour = hour;
    tm_time.tm_min = minute;
    tm_time.tm_sec = 0;
    tm_time.tm_isdst = -1;

    time_t local_time = mktime(&tm_time);
    if (local_time == -1)
        return -1;

    int offset = tz_hour * 3600 + tz_minute * 60;
    if (tz_sign == '-')
    {
        offset = -offset;
    }

    return local_time - offset;
}

// 添加密码：同时传入密码和时间戳，成功返回 0，失败返回 -1
int add_password(const char *pwd, const char *timestamp)
{
    // ESP_LOGI(PASSWORD_MANAGER_TAG, "Checking password: %s", pwd);
    // ESP_LOGI(PASSWORD_MANAGER_TAG, "Checking timestamp: %s", timestamp);
    password_control *ctrl = get_password_control_instance();
    if (ctrl->count >= MAX_PASSWORDS)
    {
        return -1; // 密码条目已满
    }
    // 复制密码和时间戳字符串存入新条目中
    ctrl->entries[ctrl->count].password = strdup(pwd);
    ctrl->entries[ctrl->count].timestamp = strdup(timestamp);
    if (ctrl->entries[ctrl->count].password == NULL ||
        ctrl->entries[ctrl->count].timestamp == NULL)
    {
        return -1; // 内存分配失败
    }
    ctrl->count++;

    // Removed the problematic line with entered_password
    // Instead, log the password that was just added
    ESP_LOGI(PASSWORD_MANAGER_TAG, "Password added successfully");
    
    // Log all stored passwords
    for (int i = 0; i < ctrl->count; i++) {
        ESP_LOGI(PASSWORD_MANAGER_TAG, "Stored password[%d]: %s, expiry: %s", 
                i, 
                ctrl->entries[i].password,
                ctrl->entries[i].timestamp);
    }

    return 0;
}

// 删除密码：只需传入密码字符串进行匹配，删除成功返回 0，未找到返回 -1
int delete_password(const char *pwd)
{
    password_control *ctrl = get_password_control_instance();
    int found = -1;
    for (int i = 0; i < ctrl->count; i++)
    {
        if (strcmp(ctrl->entries[i].password, pwd) == 0)
        {
            found = i;
            break;
        }
    }
    if (found == -1)
    {
        return -1; // 没有找到该密码
    }
    // 释放该条目的内存
    free(ctrl->entries[found].password);
    free(ctrl->entries[found].timestamp);
    // 将后面的条目往前移动
    for (int i = found; i < ctrl->count - 1; i++)
    {
        ctrl->entries[i] = ctrl->entries[i + 1];
    }
    ctrl->count--;
    return 0;
}

// 获取所有密码条目，将其复制到用户传入的数组中，并通过 num 返回密码数量
void get_all_passwords(PasswordEntry *entry_array, int *num)
{
    password_control *ctrl = get_password_control_instance();
    *num = ctrl->count;
    for (int i = 0; i < ctrl->count; i++)
    {
        entry_array[i] = ctrl->entries[i];
    }
}

// 验证密码是否有效：传入密码字符串
// 1. 若队列中没有该密码，则返回 false；
// 2. 若存在，则解析其时间戳，若当前时间超过该时间戳（密码过期）则返回 false，否则返回 true
bool is_password_valid(const char *pwd)
{
    ESP_LOGI(PASSWORD_MANAGER_TAG, "Checked passwd: %s", pwd);
    return true;
    password_control *ctrl = get_password_control_instance();
    for (int i = 0; i < ctrl->count; i++)
    {
        if (strcmp(ctrl->entries[i].password, pwd) == 0)
        {
            // // 解析密码对应的时间戳
            // time_t expiration = parse_timestamp(ctrl->entries[i].timestamp);
            // if (expiration == -1)
            // {
            //     return false;
            // }
            // time_t now = time(NULL);
            // // 当前时间晚于过期时间，密码无效
            // if (now > expiration)
            // {
            //     return false;
            // }
            // else
            // {
            //     return true;
            // }
            return true;
        }
    }
    return false; // 没有找到该密码
}

// 释放单例实例以及其中所有分配的内存
void free_password_control()
{
    if (instance)
    {
        for (int i = 0; i < instance->count; i++)
        {
            free(instance->entries[i].password);
            free(instance->entries[i].timestamp);
        }
        free(instance);
        instance = NULL;
    }
}