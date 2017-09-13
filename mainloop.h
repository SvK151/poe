internal void
GetLastID(void *LastIDFile, string *String){
    
    PlatformReadFile(LastIDFile,String->data,STRING_SIZE - 1);
    
    //TODO(MIKE):Read data directory for any file with the latest id.
    if(String->data[0] == 0){ 
        //if no/empty file
        String->data[0] = 48; //0 (ascii)
        String->size = 1;
    } else {
        String->size = CharLength(String->data);
    }
}

internal app_state 
InitializeState(){
    //TODO(MIKE): If any of this fails return a nulled out object.
    
    app_state r = {};
    
    r.last_id_file = PlatformOpenFile("./cfg/LastID.ini");
    r.time_log.frequency = PlatformGetTimeFreq();
    r.working_memory = PlatformGetMemory(Megabytes(6));
    
    void *m = r.working_memory.pointer;
    
    r.request.server_name = MakeString("api.pathofexile.com",m,1);
    
    r.current_id = MakeString("",m,2);
    GetLastID(r.last_id_file,&r.current_id);
    
    r.request.api_call = MakeString("public-stash-tabs/?id=",m,3);
    
    r.previous_id = MakeString("",m,4);
    r.next_id = MakeString("",m,5);
    
    r.output_file = MakeString("./data/",m,6);
    r.log.file_date = PlatformGetDate();
    r.log.file_name = MakeString("./logs/",m,7);
    
    sprintf(r.log.file_name.data+7,"%i.log",r.log.file_date);
    
    r.log.log_file = PlatformOpenFile(r.log.file_name.data);
    
    PlatformSetFileEnd(r.log.log_file);
    
    r.log.log_text = MakeString("",m,8);
    
    //256bytes : inflate_file
    //1mb-256bytes : Strings
    //5mb: deflate_file
    
    r.deflate_file.pointer = (void*)((u8*)m + Megabytes(1));
    r.inflate_file.pointer = m; 
    
    return(r);
}


internal f32 
GetLogTime(time_state t){
    f32 r = {};
    r = t.end - t.start;
    r *= 1000000;
    r /= t.frequency;
    r /= 1000;
    
    return(r);
}

inline b32 
CheckData(u8 *Memory){
    b32 r = false;
    //The 3rd character should be lowercase e (ascii) if being throttled
    if(Memory[2] != 101){
        r = true;
    }
    return(r);
}

internal b32
UpdateID(string *ID, void *TempMemory){
    b32 r = false;
    u32 Length;
    char *Memory = (char*)TempMemory;
    
    Memory += 19;
    Length = FindNextValue(Memory,34); //Double Quote (ascii)
    CopyChar(Memory,Length,ID->data);
    if(CharLength(ID->data) == Length){
        ID->size = Length;
        r = true;
    }
    return(r);
}

internal void
IncrementIDs(app_state *State){
    char *temp = State->previous_id.data;
    
    State->previous_id = State->current_id;
    State->current_id = State->next_id;
    
    State->next_id.data = temp;
    *State->next_id.data = 0;
    State->next_id.size = 0;
}

internal void 
MainLoop(){
    app_state State = InitializeState();
    b32 Running = true;
    
    while(Running){
        UpdateString(&State.request.api_call,
                     22,
                     State.current_id.data);
        
        State.time_log.start = PlatformGetHDTime();
        if(PlatformMakeHTTPRequest(&State.request)){
            State.time_log.end = PlatformGetHDTime();
            State.log.elapsed_request = GetLogTime(State.time_log);
            
            
            State.time_log.start = PlatformGetHDTime();
            if(State.request.status != false && PlatformGetHTTPData(&State.request,&State.deflate_file)){
                
                State.time_log.end = PlatformGetHDTime();
                State.log.elapsed_download = GetLogTime(State.time_log);
                
                State.time_log.start = PlatformGetHDTime();
                z_stream Stream = {};
                Stream.next_in = (Bytef*)State.deflate_file.pointer;
                Stream.avail_in = State.deflate_file.size;
                Stream.next_out = (Bytef*)State.inflate_file.pointer;
                Stream.data_type = Z_TEXT;
                Stream.avail_out = Bytes(STRING_SIZE - 1); 
                
                if(inflateInit2(&Stream,16+MAX_WBITS) == Z_OK){
                    s32 r = 0;
                    r = inflate(&Stream, Z_NO_FLUSH);
                    switch(r){
                        case Z_OK:{
                            State.inflate_file.size = Stream.total_out;
                            if(CheckData((u8*)State.inflate_file.pointer)){
                                UpdateID(&State.next_id,State.inflate_file.pointer);
                                State.output_file.size = 7;
                                State.output_file.size += CopyChar(State.current_id.data,
                                                                   State.output_file.data+7);
                                
                                State.output_file += ".zip";
                                
                                if(PlatformWriteFile(State.output_file.data,&State.deflate_file)){
                                    if(PlatformWriteFile(State.last_id_file,&State.next_id)){
                                        State.time_log.end = PlatformGetHDTime();
                                        State.log.elapsed_disk_write = GetLogTime(State.time_log);
                                        
                                        PlatformLog(&State.log, State.current_id);
                                        IncrementIDs(&State);
                                    } else {
                                        PlatformLogError(&State.log,"File write failed (nextid) ", State.next_id);
                                        Running = false;
                                    }
                                } else {
                                    PlatformLogError(&State.log,"File write failed (deflate) ", State.current_id);
                                    Running = false;
                                }
                            } else {
                                //NOTE(MIKE): Rerun loop we are throttled
                                State.sleep_ms = Seconds(5);
                                PlatformLogError(&State.log,"api throttled retrying ", State.current_id);
                            }
                        } break;
                        
                        default:{
                            PlatformLogError(&State.log,"zlib inflate error ",r);
                            Running = false;
                        } break;
                    }
                } else {
                    PlatformLogError(&State.log,"Failed to initialize zlib stream ", State.current_id);
                    Running = false;
                }
            } else { 
                PlatformLogError(&State.log,"http request bad status ", State.request.fail_step);
                Running = false;
            }
        } else {
            PlatformLogError(&State.log,"http request bad status ", State.request.fail_step);
            Running = false;
        }
        
        if(Running){
            if(!State.sleep_ms){
                State.time_log.start = State.log.start_disk_write;
                State.time_log.end = PlatformGetHDTime();
                State.sleep_ms = GetLogTime(State.time_log);
                if(State.sleep_ms > DEFAULT_HTTP_MS_DELAY){
                    State.sleep_ms = DEFAULT_HTTP_MS_DELAY;
                }
            } 
            PlatformSleep(State.sleep_ms);
        }
        
        if(Running && !(State.sleep_ms >= DEFAULT_HTTP_MS_DELAY) ){
            PlatformSleep(DEFAULT_HTTP_MS_DELAY - State.sleep_ms);
        }
        State.sleep_ms = 0;
    }
    
    PlatformCloseFile(State.last_id_file, &State.log);
    PlatformCloseFile(State.log.log_file);
}