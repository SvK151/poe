#define STRING_SIZE 256

struct string{
    u32 size;
    char *data;
};

inline u32 
CharLength(char *c){
    u32 r = {};
    
    for(u32 i = 0; c[i] != 0; ++i){
        if(c[i+1] == 0){ 
            r = i+1; 
        }
    }
    return(r);
}

inline u32 
CopyChar(char *src, char *dest){
    u32 r = 0;
    u32 l1 = CharLength(src);
    u32 l2 = 0;
    
    for(u32 i = 0; i < l1; ++i){
        dest[i] = src[i];
    }
    dest[l1] = 0;
    
    l2 = CharLength(dest);
    if( l1 == l2 ){
        r = l2;
    }
    
    return(r);
}

inline u32 
CopyChar(char *src,u32 n, char *dest){
    u32 r = 0;
    u32 l2 = 0;
    
    for(u32 i = 0; i < n; ++i){
        dest[i] = src[i];
    }
    dest[n] = 0;
    
    l2 = CharLength(dest);
    if( n == l2 ){
        r = l2;
    }
    
    return(r);
}

inline u32 
StringLength(string s){ 
    return(CharLength(s.data)); 
}

inline string
MakeString(char *c){
    string r;
    
    r.data = c;
    r.size = CharLength(c);
    
    return(r);
}

inline string
MakeString(char *c, void* m){
    string r = {};
    
    r.size = CopyChar(c,(char*)m);
    r.data = (char*)m;
    
    return(r);
}

//TODO(MIKE): Handle this differently
inline string
MakeString(char *c, void* m, u32 x){
    u8 *m2 = (u8*)m;
    m2 += ( STRING_SIZE * (x));
    return(MakeString(c,(void*)m2));
}

inline b32 
UpdateString(string *String, //String to update
             u32 Start, //Postition to start the update
             char *Char){ //string to replace existing
    b32 r = false;
    char *pString = String->data;
    pString += Start;
    
    u32 NewChar = CopyChar(Char,pString);
    
    if(NewChar){
        String->size = Start + NewChar;
        r = true;
    }
    return(r);
}

inline s32
FindNextValue(char *s, char c){
    s32 x = -1;
    u32 i = 0;
    b32 loop = true;
    
    while( loop ){
        if(*s == c) {
            x = (s32)i;
            loop = false;
        }
        ++s;
        ++i;
    }
    return(x);
}

inline string &
operator+=(string &a, char *b){
    u32 l = CharLength(b);
    for(u32 x = 0; x < l; ++x){
        a.data[a.size+x] = b[x];
    }
    a.size += l;
    return(a);
}