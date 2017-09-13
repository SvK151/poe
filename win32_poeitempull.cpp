#include <windows.h>
#include <wininet.h>
#include "zlib.h"

#include "platform.h"
#include "mainloop.h"

internal memory 
PlatformGetMemory(u32 Size){
    memory r = {};
    r.pointer = VirtualAlloc(0,Size, 
                             MEM_RESERVE|MEM_COMMIT,
                             PAGE_READWRITE);
    r.size = Size;
    return(r);
}

internal b32 
PlatformFreeMemory(memory Memory){
    b32 r = VirtualFree(Memory.pointer,0,MEM_RELEASE);
    return(r);
}

internal void 
PlatformGetLastError(http_request *Request, HTTPR_FAILURE Fail){
    Request->fail_step = Fail;
    Request->win32_last_error = GetLastError();
    Request->status = false;
}

inline b32 
CheckHTTPStatus(char *c){
    b32 r = false;
    if(c[0] == 50 && c[1] == 48 && c[2] == 48){ r = true; }
    return(r);
}

internal void 
CloseNetHandles(net_handles *h){
    if(InternetCloseHandle(h->request)){ h->request = 0; }
    if(InternetCloseHandle(h->connect)){ h->connect = 0; }
    if(InternetCloseHandle(h->open)){ h->open = 0; } 
}

internal b32 
PlatformMakeHTTPRequest(http_request *Request){
    b32 r = false;
    
    net_handles *h = &Request->handles;
    char *Server = Request->server_name.data;
    char *URL = Request->api_call.data;
    
    DWORD Access = INTERNET_OPEN_TYPE_DIRECT;
    INTERNET_PORT Port = INTERNET_DEFAULT_HTTP_PORT;
    DWORD Type = INTERNET_SERVICE_HTTP;
    DWORD Flags = INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_PRAGMA_NOCACHE;
    
    PCTSTR Types[] = {"application/json",NULL};
    PCTSTR Headers = "Accept-Encoding: gzip";  
    
    DWORD BufferSize = STRING_SIZE;
    char Buffer[STRING_SIZE] = {};
    char *pBuffer = &Buffer[0];
    DWORD Index = {};
    DWORD InfoLevel = HTTP_QUERY_STATUS_CODE;
    
    //TODO(MIKE): Keep InternetOpen & InternetConnect handle open so I dont need to call this each pass through the loop?
    h->open = InternetOpen("webdevsucks",Access,NULL,NULL,0);
    if(h->open){
        h->connect = InternetConnect(h->open,Server,Port,0,0,Type,0,0);
        if(h->connect){
            h->request = HttpOpenRequest(h->connect,"GET",URL,
                                         "HTTP/1.1",NULL,Types,Flags,0);
            if(h->request){
                if(HttpSendRequest(h->request,Headers,-1LL,0,0)){
                    if(HttpQueryInfo(h->request,InfoLevel,pBuffer,
                                     &BufferSize,&Index)){
                        if(CheckHTTPStatus(pBuffer)){
                            Request->status = true;
                            r = true;
                        } else {
                            PlatformGetLastError(Request,HTTP_WRONG_STATUS);
                        }
                    } else {
                        PlatformGetLastError(Request,HTTP_QUERY_INFO);
                    }
                } else {
                    PlatformGetLastError(Request,HTTP_SEND_REQUEST);
                }
            } else {
                PlatformGetLastError(Request,HTTP_REQUEST);
            }
        } else {
            PlatformGetLastError(Request,HTTP_CONNECT);
        }
    } else {
        PlatformGetLastError(Request,HTTP_OPEN);
    }
    return(r);
}

internal b32 
PlatformGetHTTPData(http_request *Request,
                    memory *DeflateFile){
    b32 r = false;
    
    DWORD BufferSize = DEFLATE_SIZE;
    DWORD BytesRead = 0;
    
    u32 LoopCount = 0;
    u32 DataSize = 0;
    b32 Running = true;
    
    u32 Size = 0;
    u8 *Memory = (u8*)DeflateFile->pointer;
    HINTERNET Req = Request->handles.request;
    
    u32 Loops = 0;
    DWORD e = {};
    DWORD BytesRemain = 0;
    
    u32 ms[100];
    
    while(Running){
        Running = InternetReadFile(Req,Memory,
                                   BufferSize,&BytesRead);
        Loops += 1;
        if(BytesRead && Running != 0 ){
            Size += BytesRead;
            Memory += BytesRead;
            BufferSize -= BytesRead;
            BytesRead = 0;
        } else if( Running && BytesRead == 0 ) {
            DeflateFile->size = Size;
            Running = false;
            r = true;
        } else {
            Running = false;
            DeflateFile->size = 0;
            r = false;
        }
    }
    CloseNetHandles(&Request->handles);
    return(r);
}

internal u32 
PlatformGetTimeFreq(){
    LARGE_INTEGER temp = {};
    QueryPerformanceFrequency(&temp);
    u32 r = temp.QuadPart;
    return(r);
}

internal u32 
PlatformGetHDTime(){
    LARGE_INTEGER temp = {};
    QueryPerformanceCounter(&temp);
    u32 r = temp.QuadPart;
    return(r);
}

internal time 
PlatformGetTime(){
    SYSTEMTIME t = {};
    time r = {};
    
    GetLocalTime(&t);
    r.year = t.wYear;
    r.month = t.wMonth;
    r.day = t.wDay;
    r.hour = t.wHour;
    r.minute = t.wMinute;
    r.second = t.wSecond;
    r.millisecond = t.wMilliseconds;
    return(r);
};

internal u32 
PlatformGetDate(){
    SYSTEMTIME t = {};
    u32 r = {};
    
    GetLocalTime(&t);
    r += t.wYear*10000;
    r += t.wMonth*100;
    r += t.wDay;
    return(r);
};

internal void 
PlatformSleep(u32 MilliSeconds){
    Sleep(MilliSeconds);
}

internal b32
PlatformSetFileEnd(void *Handle){
    b32 r = false;
    if(SetFilePointer(Handle,NULL,NULL,FILE_END) != INVALID_SET_FILE_POINTER){
        r = true;
    }
    return(r);
}

internal void *
PlatformOpenFile(char *Location, log_data *Log){
    void *r = {};
    //TODO(MIKE):If directory does not exist create
    r = CreateFile(Location,
                   GENERIC_READ|GENERIC_WRITE,
                   FILE_SHARE_READ,
                   NULL,
                   OPEN_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL);
    
    if(r != INVALID_HANDLE_VALUE){
        SetLastError(0);
    } else {
        PlatformLogError(Log, "Win32 File Open",GetLastError());
        r = 0;
    }
    
    return (r);
}

internal void *
PlatformOpenFile(char *Location){
    void *r = {};
    //TODO(MIKE):If directory does not exist create
    r = CreateFile(Location,
                   GENERIC_READ|GENERIC_WRITE,
                   FILE_SHARE_READ,
                   NULL,
                   OPEN_ALWAYS,
                   FILE_ATTRIBUTE_NORMAL,
                   NULL);
    return (r);
}

internal read_result
PlatformReadFile(void *FileHandle,char *Memory,u32 MaxBytes){
    read_result r = {};
    
    LARGE_INTEGER FileSize;
    if( GetFileSizeEx(FileHandle,&FileSize) && MaxBytes >= FileSize.QuadPart ){
        DWORD BytesRead = {};
        DWORD u32Size = FileSize.QuadPart;
        if(ReadFile(FileHandle,Memory,u32Size,&BytesRead,0) &&
           u32Size == BytesRead){
            r.result = true;
            r.size = BytesRead;
        } else {
            DWORD Error = 0;
            Error = GetLastError();
            SetLastError(0);
        }
    }
    return(r);
}
internal read_result
PlatformReadFile(void *FileHandle,char *Memory){
    return(PlatformReadFile(FileHandle,Memory,(u32)0xFFFFFFFF));
}

internal b32
PlatformWriteFile(char *Location,memory *Memory){
    b32 r = false;
    void *Handle = {};
    
    Handle = CreateFile(Location, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if(Handle != INVALID_HANDLE_VALUE){
        u32 Bytes = 0;
        
        if(WriteFile(Handle, Memory->pointer, Memory->size, (LPDWORD)&Bytes, NULL) && 
           Bytes == Memory->size){
            r = true;
        }
    }
    
    CloseHandle(Handle);
    return(r);
}

internal b32
PlatformWriteFile(void *Handle,string *String){
    b32 r = false;
    
    SetFilePointer(Handle,0,NULL,FILE_BEGIN);
    
    u32 Bytes = 0;
    if(WriteFile(Handle, String->data, String->size, (LPDWORD)&Bytes, NULL) && Bytes == String->size){
        if(SetEndOfFile(Handle)){
            r = true;
        }
    }
    return(r);
}

internal b32
PlatformLog(log_data *Log, string CurrentID){
    b32 r = false;
    u32 CurrentDate = PlatformGetDate();
    
    if(Log->file_date != CurrentDate){
        PlatformCloseFile(Log->log_file);
        Log->log_file = 0;
        //TODO(MIKE): write own implementation of sprintf
        sprintf(Log->file_name.data+7,"%i.log",CurrentDate);
        
        Log->log_file = PlatformOpenFile(Log->file_name.data);
        Log->file_date = CurrentDate;
    }
    
    if(Log->log_file){
        time Time = PlatformGetTime();
        char *c = Log->log_text.data;
        u32 Bytes = 0;
        u32 x = 0;
        
        Log->log_text.size = 0;
        *c = 0;
        
        sprintf(c,"%i%02i%02i\t%02i%02i%02i%03i\t",
                Time.year,Time.month,Time.day,Time.hour,
                Time.minute,Time.second,Time.millisecond);
        x = CharLength(c);
        
        WriteFile(Log->log_file,c,x,(LPDWORD)&Bytes,NULL);
        WriteFile(Log->log_file,CurrentID.data,CurrentID.size,(LPDWORD)&Bytes,NULL);
        
        Log->log_text.size = 0;
        *c = 0;
        
        sprintf(c,"\t%.2f\t%.2f\t%.2f\r\n",
                Log->elapsed_request,
                Log->elapsed_download,
                Log->elapsed_disk_write);
        x = CharLength(c);
        
        WriteFile(Log->log_file,c,x,(LPDWORD)&Bytes,NULL);
        
        r = true;
    }
    
    return(r);
}

//TODO(MIKE): Clean up?
internal void
PlatformLogError(log_data *Log, char *Text, u32 Error){
    
    time Time = PlatformGetTime();
    char *c = Log->log_text.data;
    u32 Bytes = 0;
    u32 x = 0;
    
    Log->log_text.size = 0;
    c[0] = 0;
    
    sprintf(c,"%i%02i%02i\t%02i%02i%02i%03i\t",
            Time.year,Time.month,Time.day,Time.hour,
            Time.minute,Time.second,Time.millisecond);
    x = CharLength(c);
    
    WriteFile(Log->log_file,c,x,(LPDWORD)&Bytes,NULL);
    
    x = CharLength(Text);
    WriteFile(Log->log_file,Text,x,(LPDWORD)&Bytes,NULL);
    
    Log->log_text.size = 0;
    c[0] = 0;
    
    sprintf(c,"%i",Error);
    x = CharLength(c);
    
    WriteFile(Log->log_file,c,x,(LPDWORD)&Bytes,NULL);
    
}

//TODO(MIKE): Clean up?
internal void
PlatformLogError(log_data *Log, char *Text, string Error){
    
    time Time = PlatformGetTime();
    char *c = Log->log_text.data;
    u32 Bytes = 0;
    u32 x = 0;
    
    Log->log_text.size = 0;
    c[0] = 0;
    
    sprintf(c,"%i%02i%02i\t%02i%02i%02i%03i\t",
            Time.year,Time.month,Time.day,Time.hour,
            Time.minute,Time.second,Time.millisecond);
    x = CharLength(c);
    
    WriteFile(Log->log_file,c,x,(LPDWORD)&Bytes,NULL);
    
    x = CharLength(Text);
    WriteFile(Log->log_file,Text,x,(LPDWORD)&Bytes,NULL);
    
    Log->log_text.size = 0;
    c[0] = 0;
    
    WriteFile(Log->log_file,Error.data,Error.size,(LPDWORD)&Bytes,NULL);
    
}

internal b32
PlatformCloseFile(void* Handle, log_data *Log){
    b32 r = false;
    r = CloseHandle(Handle);
    
    if(!r){
        PlatformLogError(Log,"Win32 File Close",GetLastError());
    }
    return(r);
}

internal b32
PlatformCloseFile(void* Handle){
    b32 r = false;
    r = CloseHandle(Handle);
    return(r);
}

s32 CALLBACK 
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,int nCmdShow){
    
    MainLoop();
    return(0);
}