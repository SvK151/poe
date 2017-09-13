#include <stdint.h>
#include <stdio.h>

#define internal static
#define local_persist static
#define global_variable static

#define Assert(Expression)

#define Bytes(Value) (Value)
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define Seconds(Value) ((Value)*1000LL)

#define MAX_PATH_LENGTH 260
#define DEFAULT_HTTP_MS_DELAY 1000

#define DEFLATE_SIZE Megabytes(5)

#define Pi32 3.14159265359f

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef int32_t b32;

typedef float f32;
typedef double f64;

struct memory{
    u32 size;
    void *pointer;
};

struct time_state{
    u32 frequency;
    u32 start;
    u32 end;
};

struct time{
    u32 year;
    u32 month;
    u32 day;
    u32 hour;
    u32 minute;
    u32 second;
    u32 millisecond;
};

#include "string.h"

struct log_data{
    void *log_file;
    string file_name;
    u32 file_date;
    string log_text;
    
    f32 elapsed_request;
    f32 elapsed_download;
    f32 elapsed_disk_write;
    
    u32 start_request;
    u32 start_download;
    u32 start_disk_write;
};

struct net_handles{
    HINTERNET open;
    HINTERNET connect;
    HINTERNET request;
};

enum HTTPR_FAILURE{
    HTTP_OPEN,
    HTTP_CONNECT,
    HTTP_REQUEST,
    HTTP_SEND_REQUEST,
    HTTP_QUERY_INFO,
    HTTP_WRONG_STATUS
};

struct http_request{
    string server_name;
    string api_call;
    
    b32 status;
    net_handles handles;
    HTTPR_FAILURE fail_step;
    u32 win32_last_error;
};

struct app_state{
    void *last_id_file;
    time_state time_log;
    time current_time;
    memory working_memory;
    
    string previous_id;
    string current_id;
    string next_id;
    
    string output_file;
    
    memory deflate_file;
    memory inflate_file;
    
    log_data log;
    http_request request;
    u32 sleep_ms;
};

struct read_result{
    b32 result;
    u32 size;
}; 

internal u32 
PlatformGetTimeFreq();

internal time 
PlatformGetTime();

internal u32 
PlatformGetDate();

internal void* 
PlatformOpenFile(char *Location, log_data *Log);

internal void* 
PlatformOpenFile(char *Location);

internal read_result 
PlatformReadFile(void *FileHandle, char *Memory, u32 MaxBytes);

internal read_result 
PlatformReadFile(void *FileHandle, char *Memory);

internal b32 
PlatformWriteFile(char *Location, memory *Memory);

internal b32
PlatformWriteFile(void *Handle, string *String);

internal b32
PlatformCloseFile(void *Handle);

internal b32
PlatformCloseFile(void *Handle, log_data *Log);

internal u32 
PlatformGetHDTime();

internal memory 
PlatformGetMemory(u32 Size);

internal b32 
PlatformFreeMemory(memory *Memory);

internal b32 
PlatformMakeHTTPRequest(http_request *Request);

internal b32 
PlatformGetHTTPData(http_request *Request, memory *DeflateFile);

internal void 
PlatformSleep(u32 MilliSeconds);

internal void 
PlatformLogError(log_data *Log, char *Text, u32 Error);

internal void 
PlatformLogError(log_data *Log, char *Text, string Error);

internal b32
PlatformLog(log_data *Log, string CurrentID);

internal b32
PlatformSetFileEnd(void *Handle);