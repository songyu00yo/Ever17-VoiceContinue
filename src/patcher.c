#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
#define PATCH_ORIGINAL 1
#define PATCH_APPLIED  2
#define MAX_SIG_LEN 80
#define SCAN_CHUNK 4096

static const BYTE PATCH_A[5] = {0xE9,0x40,0x00,0x00,0x00};
static const BYTE PATCH_B[5] = {0xE9,0x3A,0x00,0x00,0x00};

static const BYTE SIG_A[] = {
    0,0,0,0,0,
    0xC7,0x05,0,0,0,0,0x08,0,0,0,
    0x0F,0xAF,0x05,0,0,0,0,
    0x8D,0x04,0x80,0x8D,0x0C,0xC0,0xB8,0x59,0x17,0xB7,0xD1,
    0x03,0xC9,0xF7,0xE1,0xC1,0xEA,0x0D,0x52,0x57,
    0xE8,0,0,0,0,0x83,0xC4,0x08,
    0x89,0x1D,0,0,0,0,
    0x89,0x1D,0,0,0,0,
    0xEB,0x05,0xE8,0,0,0,0
};
static const BYTE MASK_A[] = {
    0,0,0,0,0,
    1,1,0,0,0,0,1,1,1,1,
    1,1,1,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,1,1,1,
    1,1,0,0,0,0,
    1,1,0,0,0,0,
    1,1,1,0,0,0,0
};

static const BYTE SIG_B[] = {
    0,0,0,0,0,
    0xC7,0x44,0x24,0,0,0,0,0,
    0x0F,0xAF,0x05,0,0,0,0,
    0xC7,0x05,0,0,0,0,0x08,0,0,0,
    0x8D,0x04,0x80,0x8D,0x0C,0xC0,0xB8,0x59,0x17,0xB7,0xD1,
    0x03,0xC9,0xF7,0xE1,0xC1,0xEA,0x0D,0x52,0x6A,0x01,
    0xE8,0,0,0,0,
    0x8B,0x44,0x24,0x08,0x83,0xC4,0x08
};
static const BYTE MASK_B[] = {
    0,0,0,0,0,
    1,1,1,1,1,1,1,1,
    1,1,1,0,0,0,0,
    1,1,0,0,0,0,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,
    1,0,0,0,0,
    1,1,1,1,1,1,1
};

typedef struct {
    DWORD offA, offB;
    DWORD hitsA, hitsB;
    DWORD stateA, stateB;
} MATCHES;

static DWORD rd32(const BYTE *p) {
    return (DWORD)p[0] | ((DWORD)p[1] << 8) | ((DWORD)p[2] << 16) | ((DWORD)p[3] << 24);
}
static WORD rd16(const BYTE *p) {
    return (WORD)(p[0] | ((WORD)p[1] << 8));
}
static void wr32(BYTE *p, DWORD v) {
    p[0]=(BYTE)v; p[1]=(BYTE)(v>>8); p[2]=(BYTE)(v>>16); p[3]=(BYTE)(v>>24);
}
static BOOL masked_match(const BYTE *p, const BYTE *sig, const BYTE *mask, DWORD n) {
    DWORD i;
    for (i=0;i<n;i++) if (mask[i] && p[i]!=sig[i]) return FALSE;
    return TRUE;
}
static BOOL valid_original_prefix(const BYTE *p, DWORD imul_addr_off) {
    if (p[0] != 0xA1) return FALSE;
    return rd32(p+1) == rd32(p+imul_addr_off) + 0x0C;
}
static BOOL valid_patch_prefix(const BYTE *p, const BYTE patch[5]) {
    return memcmp(p, patch, 5) == 0;
}

static void consider_window(const BYTE *buf, DWORD len, DWORD file_base, MATCHES *m) {
    DWORD i;
    if (len >= sizeof(SIG_A)) {
        for (i=0; i + sizeof(SIG_A) <= len; i++) {
            const BYTE *p=buf+i;
            if (masked_match(p,SIG_A,MASK_A,(DWORD)sizeof(SIG_A))) {
                DWORD st=0;
                if (valid_original_prefix(p,18)) st=PATCH_ORIGINAL;
                else if (valid_patch_prefix(p,PATCH_A)) st=PATCH_APPLIED;
                if (st) { m->hitsA++; m->offA=file_base+i; m->stateA=st; }
            }
        }
    }
    if (len >= sizeof(SIG_B)) {
        for (i=0; i + sizeof(SIG_B) <= len; i++) {
            const BYTE *p=buf+i;
            if (masked_match(p,SIG_B,MASK_B,(DWORD)sizeof(SIG_B))) {
                DWORD st=0;
                if (valid_original_prefix(p,16)) st=PATCH_ORIGINAL;
                else if (valid_patch_prefix(p,PATCH_B)) st=PATCH_APPLIED;
                if (st) { m->hitsB++; m->offB=file_base+i; m->stateB=st; }
            }
        }
    }
}

static BOOL scan_section(HANDLE h, DWORD raw, DWORD size, MATCHES *m) {
    BYTE buf[SCAN_CHUNK + MAX_SIG_LEN];
    BYTE carry[MAX_SIG_LEN];
    DWORD pos=0, carry_n=0;
    SetFilePointer(h,(LONG)raw,NULL,FILE_BEGIN);
    while (pos < size) {
        DWORD want=size-pos, got=0, total, base, i;
        if (want > SCAN_CHUNK) want=SCAN_CHUNK;
        memcpy(buf,carry,carry_n);
        if (!ReadFile(h,buf+carry_n,want,&got,NULL)) return FALSE;
        if (!got) break;
        total=carry_n+got;
        base=raw+pos-carry_n;
        consider_window(buf,total,base,m);
        carry_n=total;
        if (carry_n > MAX_SIG_LEN-1) carry_n=MAX_SIG_LEN-1;
        for (i=0;i<carry_n;i++) carry[i]=buf[total-carry_n+i];
        pos+=got;
    }
    return TRUE;
}

static int scan_exe(const wchar_t *path, MATCHES *m) {
    HANDLE h;
    BYTE dos[64], peh[4096];
    DWORD got=0, peoff, need, i;
    WORD machine,nsec,opt;

    ZeroMemory(m,sizeof(*m));
    h=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (h==INVALID_HANDLE_VALUE) return 0;

    if (!ReadFile(h,dos,sizeof(dos),&got,NULL) || got<64 || dos[0]!='M' || dos[1]!='Z') {
        CloseHandle(h); return 0;
    }
    peoff=rd32(dos+0x3C);
    if (peoff>0x1000000UL) { CloseHandle(h); return 0; }

    SetFilePointer(h,(LONG)peoff,NULL,FILE_BEGIN);
    if (!ReadFile(h,peh,sizeof(peh),&got,NULL) || got<24) { CloseHandle(h); return 0; }
    if (peh[0]!='P'||peh[1]!='E'||peh[2]!=0||peh[3]!=0) { CloseHandle(h); return 0; }

    machine=rd16(peh+4);
    nsec=rd16(peh+6);
    opt=rd16(peh+20);
    if (machine!=0x014c || nsec==0 || nsec>32 || opt<0x60) { CloseHandle(h); return 0; }

    need=24+(DWORD)opt+(DWORD)nsec*40;
    if (need>got) { CloseHandle(h); return 0; }

    for (i=0;i<nsec;i++) {
        BYTE *s=peh+24+opt+i*40;
        DWORD rawsz=rd32(s+16), raw=rd32(s+20), ch=rd32(s+36);
        if ((ch & IMAGE_SCN_MEM_EXECUTE) && rawsz) {
            if (!scan_section(h,raw,rawsz,m)) { CloseHandle(h); return 0; }
        }
    }
    CloseHandle(h);

    if (m->hitsA!=1 || m->hitsB!=1) return 0;
    if (m->stateA!=m->stateB) return -1;
    return (int)m->stateA;
}

static BOOL read_at(HANDLE h,DWORD off,BYTE *b,DWORD n) {
    DWORD got=0;
    SetFilePointer(h,(LONG)off,NULL,FILE_BEGIN);
    return ReadFile(h,b,n,&got,NULL) && got==n;
}
static BOOL write5(HANDLE h,DWORD off,const BYTE b[5]) {
    DWORD wrote=0;
    SetFilePointer(h,(LONG)off,NULL,FILE_BEGIN);
    return WriteFile(h,b,5,&wrote,NULL) && wrote==5;
}

static BOOL patch_target(const wchar_t *path,const MATCHES *m,BOOL restore) {
    HANDLE h;
    BYTE p[5],tmp[4];
    DWORD addr;

    h=CreateFileW(path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if (h==INVALID_HANDLE_VALUE) return FALSE;

    if (!restore) {
        if (!read_at(h,m->offA,p,5) || p[0]!=0xA1 || !write5(h,m->offA,PATCH_A)) { CloseHandle(h); return FALSE; }
        if (!read_at(h,m->offB,p,5) || p[0]!=0xA1 || !write5(h,m->offB,PATCH_B)) { CloseHandle(h); return FALSE; }
    } else {
        if (!read_at(h,m->offA+18,tmp,4)) { CloseHandle(h); return FALSE; }
        addr=rd32(tmp)+0x0C; p[0]=0xA1; wr32(p+1,addr);
        if (!write5(h,m->offA,p)) { CloseHandle(h); return FALSE; }

        if (!read_at(h,m->offB+16,tmp,4)) { CloseHandle(h); return FALSE; }
        addr=rd32(tmp)+0x0C; p[0]=0xA1; wr32(p+1,addr);
        if (!write5(h,m->offB,p)) { CloseHandle(h); return FALSE; }
    }
    CloseHandle(h);
    return TRUE;
}

static BOOL is_self(const wchar_t *name,const wchar_t *self) {
    return _wcsicmp(name,self)==0;
}

static int find_target(const wchar_t *selfname,wchar_t *target,size_t cap,MATCHES *out,DWORD *state) {
    WIN32_FIND_DATAW fd;
    HANDLE f;
    DWORD count=0;

    f=FindFirstFileW(L"*.exe",&fd);
    if (f==INVALID_HANDLE_VALUE) return 0;

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && !is_self(fd.cFileName,selfname)) {
            MATCHES m;
            int st=scan_exe(fd.cFileName,&m);
            if (st==-1) { FindClose(f); return -1; }
            if (st==PATCH_ORIGINAL || st==PATCH_APPLIED) {
                count++;
                wcsncpy_s(target,cap,fd.cFileName,_TRUNCATE);
                *out=m;
                *state=(DWORD)st;
            }
        }
    } while (FindNextFileW(f,&fd));
    FindClose(f);

    if (count>1) return -2;
    return count==1 ? 1 : 0;
}

static void maybe_disable_proxy(void) {
    BOOL d=GetFileAttributesW(L"dsound.dll")!=INVALID_FILE_ATTRIBUTES;
    BOOL m=GetFileAttributesW(L"MMDevAPI.dll")!=INVALID_FILE_ATTRIBUTES;
    if (!d && !m) return;

    if (MessageBoxW(NULL,
        L"检测到当前目录存在 dsound.dll 或 MMDevAPI.dll。\n\n"
        L"如果这是 AudioSpeedHack / ZeroInterrupt，它会导致新语音排队。\n\n"
        L"是否临时改名禁用？文件不会被删除。",
        L"Ever17 Voice Continue",MB_YESNO|MB_ICONQUESTION)==IDYES) {

        if (d && GetFileAttributesW(L"dsound.voicecontinue.disabled.dll")==INVALID_FILE_ATTRIBUTES)
            MoveFileW(L"dsound.dll",L"dsound.voicecontinue.disabled.dll");
        if (m && GetFileAttributesW(L"MMDevAPI.voicecontinue.disabled.dll")==INVALID_FILE_ATTRIBUTES)
            MoveFileW(L"MMDevAPI.dll",L"MMDevAPI.voicecontinue.disabled.dll");
    }
}

static void restore_proxy(void) {
    if (GetFileAttributesW(L"dsound.dll")==INVALID_FILE_ATTRIBUTES &&
        GetFileAttributesW(L"dsound.voicecontinue.disabled.dll")!=INVALID_FILE_ATTRIBUTES)
        MoveFileW(L"dsound.voicecontinue.disabled.dll",L"dsound.dll");

    if (GetFileAttributesW(L"MMDevAPI.dll")==INVALID_FILE_ATTRIBUTES &&
        GetFileAttributesW(L"MMDevAPI.voicecontinue.disabled.dll")!=INVALID_FILE_ATTRIBUTES)
        MoveFileW(L"MMDevAPI.voicecontinue.disabled.dll",L"MMDevAPI.dll");
}

static void launch_game(const wchar_t *path) {
    STARTUPINFOW si={0};
    PROCESS_INFORMATION pi={0};
    wchar_t cmd[MAX_PATH*2];
    si.cb=sizeof(si);
    swprintf_s(cmd,ARRAY_LEN(cmd),L"\"%s\"",path);
    if (CreateProcessW(NULL,cmd,NULL,NULL,FALSE,0,NULL,NULL,&si,&pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
}

int WINAPI wWinMain(HINSTANCE hInst,HINSTANCE hPrev,LPWSTR cmdLine,int show) {
    wchar_t self[MAX_PATH],selfname[MAX_PATH],dir[MAX_PATH];
    wchar_t target[MAX_PATH],backup[MAX_PATH*2];
    wchar_t *slash;
    MATCHES m;
    DWORD state=0;
    int f;
    BOOL restore=(wcsstr(GetCommandLineW(),L"/restore")!=NULL);

    (void)hInst; (void)hPrev; (void)cmdLine; (void)show;

    if (!GetModuleFileNameW(NULL,self,ARRAY_LEN(self))) return 1;
    wcsncpy_s(selfname,ARRAY_LEN(selfname),self,_TRUNCATE);
    slash=wcsrchr(selfname,L'\\');
    if (slash) memmove(selfname,slash+1,(wcslen(slash+1)+1)*sizeof(wchar_t));

    wcsncpy_s(dir,ARRAY_LEN(dir),self,_TRUNCATE);
    slash=wcsrchr(dir,L'\\');
    if (slash) *slash=0;
    SetCurrentDirectoryW(dir);

    f=find_target(selfname,target,ARRAY_LEN(target),&m,&state);
    if (f==-1) {
        MessageBoxW(NULL,L"检测到目标 EXE 处于非标准/部分修改状态，已安全停止。",L"Ever17 Voice Continue",MB_OK|MB_ICONERROR);
        return 2;
    }
    if (f==-2) {
        MessageBoxW(NULL,L"检测到多个符合特征的 EXE。请暂时移走额外 EXE 后再运行。",L"Ever17 Voice Continue",MB_OK|MB_ICONERROR);
        return 3;
    }
    if (f==0) {
        MessageBoxW(NULL,
            L"没有找到可安全识别的 Ever17 2002 PC 引擎 EXE。\n\n"
            L"该版本使用代码特征扫描，不会对未知布局猜偏移写入。",
            L"Ever17 Voice Continue",MB_OK|MB_ICONERROR);
        return 4;
    }

    if (restore) {
        if (state==PATCH_APPLIED && !patch_target(target,&m,TRUE)) {
            MessageBoxW(NULL,L"恢复失败，未继续修改。",L"Ever17 Voice Continue",MB_OK|MB_ICONERROR);
            return 5;
        }
        restore_proxy();
        MessageBoxW(NULL,L"已恢复原版语音行为。",L"Ever17 Voice Continue",MB_OK|MB_ICONINFORMATION);
        return 0;
    }

    if (state==PATCH_ORIGINAL) {
        swprintf_s(backup,ARRAY_LEN(backup),L"%s.voicecontinue.bak",target);
        if (GetFileAttributesW(backup)==INVALID_FILE_ATTRIBUTES)
            CopyFileW(target,backup,TRUE);

        if (!patch_target(target,&m,FALSE)) {
            MessageBoxW(NULL,L"补丁写入失败。",L"Ever17 Voice Continue",MB_OK|MB_ICONERROR);
            return 6;
        }
        if (scan_exe(target,&m)!=PATCH_APPLIED) {
            MessageBoxW(NULL,L"补丁写入后的验证失败。",L"Ever17 Voice Continue",MB_OK|MB_ICONERROR);
            return 7;
        }
        maybe_disable_proxy();
        MessageBoxW(NULL,
            L"Voice Continue 已安装。\n\n"
            L"翻到无新语音文本：当前语音继续。\n"
            L"出现新语音：旧语音立即停止，新语音立即开始。\n"
            L"原速、不排队、不重叠。",
            L"Ever17 Voice Continue",MB_OK|MB_ICONINFORMATION);
    } else {
        maybe_disable_proxy();
    }

    launch_game(target);
    return 0;
}
