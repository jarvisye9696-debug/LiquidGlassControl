#pragma once
#include <shellapi.h>
#include <functional>
namespace taskbar_hide {
inline bool intersects(RECT a,RECT b){RECT r{};return IntersectRect(&r,&a,&b)!=FALSE;}
class Controller {
    HWND owner{};std::wstring ini;std::vector<HWINEVENTHOOK> hooks;bool running=false,original=false,last=false,owned=false,switching=false;
    inline static Controller*active=nullptr;
    std::function<DWORD()> stateProvider;std::function<bool(bool)> stateWriter;
    DWORD state()const{if(stateProvider)return stateProvider();APPBARDATA a{};a.cbSize=sizeof(a);return (DWORD)SHAppBarMessage(ABM_GETSTATE,&a);}
    static bool shellSurface(HWND window){DWORD pid=0;GetWindowThreadProcessId(window,&pid);HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);if(!process)return false;wchar_t path[32768]{};DWORD size=32768;BOOL ok=QueryFullProcessImageNameW(process,0,path,&size);CloseHandle(process);if(!ok)return false;auto name=wcsrchr(path,L'\\');name=name?name+1:path;return _wcsicmp(name,L"StartMenuExperienceHost.exe")==0||_wcsicmp(name,L"ShellExperienceHost.exe")==0||_wcsicmp(name,L"ShellHost.exe")==0||_wcsicmp(name,L"SearchHost.exe")==0;}
    bool write(bool hide){if(stateWriter)return stateWriter(hide);APPBARDATA a{};a.cbSize=sizeof(a);a.hWnd=FindWindowW(L"Shell_TrayWnd",nullptr);a.lParam=(state()&~ABS_AUTOHIDE)|(hide?ABS_AUTOHIDE:0);SHAppBarMessage(ABM_SETSTATE,&a);return ((state()&ABS_AUTOHIDE)!=0)==hide;}
    void number(const wchar_t*key,bool value){WritePrivateProfileStringW(L"TaskbarRecovery",key,value?L"1":L"0",ini.c_str());}
    static void CALLBACK event(HWINEVENTHOOK,DWORD kind,HWND h,LONG object,LONG child,DWORD,DWORD){
        if(!active)return;
        if(kind==EVENT_SYSTEM_SWITCHSTART){active->switching=true;KillTimer(active->owner,7);return;}
        if(kind==EVENT_SYSTEM_SWITCHEND){active->switching=false;SetTimer(active->owner,7,180,nullptr);return;}
        if(active->switching||!h||object!=OBJID_WINDOW||child!=CHILDID_SELF)return;
        if(kind!=EVENT_OBJECT_DESTROY&&GetAncestor(h,GA_ROOT)!=h)return;
        DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId())return;
        SetTimer(active->owner,7,180,nullptr);
    }
    bool covered(){
        struct Search{std::vector<RECT> bars;bool hit=false;} search;
        EnumWindows([](HWND h,LPARAM p)->BOOL{wchar_t c[64]{};GetClassNameW(h,c,64);if(wcscmp(c,L"Shell_TrayWnd")&&wcscmp(c,L"Shell_SecondaryTrayWnd"))return TRUE;auto&s=*(Search*)p;RECT r{};GetWindowRect(h,&r);MONITORINFO m{};m.cbSize=sizeof(m);GetMonitorInfoW(MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST),&m);int thickness=MulDiv(48,GetDpiForWindow(h),96);if(r.bottom-r.top<4){r=m.rcMonitor;r.top=r.bottom-thickness;}s.bars.push_back(r);return TRUE;},(LPARAM)&search);
        EnumWindows([](HWND h,LPARAM p)->BOOL{auto&s=*(Search*)p;if(!IsWindowVisible(h)||IsIconic(h)||(GetWindowLongPtrW(h,GWL_EXSTYLE)&(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE)))return TRUE;
            wchar_t c[128]{};GetClassNameW(h,c,128);if(wcscmp(c,L"Shell_TrayWnd")==0||wcscmp(c,L"Shell_SecondaryTrayWnd")==0||wcscmp(c,L"Progman")==0||wcscmp(c,L"WorkerW")==0||wcscmp(c,L"XamlExplorerHostIslandWindow")==0||wcscmp(c,L"#32768")==0||wcscmp(c,L"MultitaskingViewFrame")==0||wcscmp(c,L"TaskSwitcherWnd")==0||shellSurface(h))return TRUE;
            DWORD cloaked=0;DwmGetWindowAttribute(h,DWMWA_CLOAKED,&cloaked,sizeof(cloaked));if(cloaked)return TRUE;RECT r{};if(FAILED(DwmGetWindowAttribute(h,DWMWA_EXTENDED_FRAME_BOUNDS,&r,sizeof(r))))GetWindowRect(h,&r);auto monitor=MonitorFromWindow(h,MONITOR_DEFAULTTONEAREST);
            for(auto bar:s.bars){if(intersects(r,bar)||(IsZoomed(h)&&MonitorFromRect(&bar,MONITOR_DEFAULTTONEAREST)==monitor)){s.hit=true;return FALSE;}}return TRUE;},(LPARAM)&search);return search.hit;
    }
public:
    bool recover(const std::wstring&path){ini=path;if(!GetPrivateProfileIntW(L"TaskbarRecovery",L"Pending",0,ini.c_str()))return true;original=GetPrivateProfileIntW(L"TaskbarRecovery",L"Original",0,ini.c_str())!=0;last=GetPrivateProfileIntW(L"TaskbarRecovery",L"Applied",0,ini.c_str())!=0;if(((state()&ABS_AUTOHIDE)!=0)==last&&!write(original))return false;WritePrivateProfileStringW(L"TaskbarRecovery",nullptr,nullptr,ini.c_str());return true;}
    bool enable(HWND window,const std::wstring&path){if(running)return true;if(!FindWindowW(L"Shell_TrayWnd",nullptr))return false;owner=window;if(!recover(path))return false;original=last=(state()&ABS_AUTOHIDE)!=0;active=this;
        const DWORD ranges[][2]={{EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND},{EVENT_SYSTEM_MINIMIZESTART,EVENT_SYSTEM_MINIMIZEEND},{EVENT_SYSTEM_SWITCHSTART,EVENT_SYSTEM_SWITCHEND},{EVENT_OBJECT_DESTROY,EVENT_OBJECT_HIDE},{EVENT_OBJECT_LOCATIONCHANGE,EVENT_OBJECT_LOCATIONCHANGE},{EVENT_OBJECT_CLOAKED,EVENT_OBJECT_UNCLOAKED}};
        for(auto&r:ranges){auto hook=SetWinEventHook(r[0],r[1],nullptr,event,0,0,WINEVENT_OUTOFCONTEXT);if(!hook){disable();return false;}hooks.push_back(hook);}running=true;return update();}
    bool transition(bool desired){bool current=(state()&ABS_AUTOHIDE)!=0;if(owned&&current!=last){disable();return false;}if(desired==current)return true;
        number(L"Original",original);number(L"Applied",desired);number(L"Pending",true);WritePrivateProfileStringW(nullptr,nullptr,nullptr,ini.c_str());owned=true;
        last=desired;return write(desired);}
    bool update(){KillTimer(owner,7);if(!running||switching)return true;return transition(covered());}
    bool disable(){for(auto h:hooks)UnhookWinEvent(h);hooks.clear();if(owner)KillTimer(owner,7);if(active==this)active=nullptr;running=false;switching=false;if(owned){if(((state()&ABS_AUTOHIDE)!=0)==last&&!write(original))return false;WritePrivateProfileStringW(L"TaskbarRecovery",nullptr,nullptr,ini.c_str());owned=false;}return true;}
    ~Controller(){disable();}
    static bool regressionTest(const std::wstring&path){
        DWORD fake=ABS_ALWAYSONTOP;unsigned writes=0;Controller c;c.ini=path;c.original=c.last=false;
        c.stateProvider=[&]{return fake;};c.stateWriter=[&](bool hide){++writes;fake=(fake&~ABS_AUTOHIDE)|(hide?ABS_AUTOHIDE:0);return true;};
        c.running=true;c.switching=true;bool suspended=c.update()&&writes==0;c.running=false;c.switching=false;
        bool ok=suspended&&c.transition(true)&&(fake&ABS_AUTOHIDE)&&c.transition(true)&&writes==1&&c.disable()&&fake==ABS_ALWAYSONTOP&&writes==2;
        // Simulate a crash after applying auto-hide: the next instance must
        // restore it using only its journal and preserve unrelated flags.
        c.number(L"Original",false);c.number(L"Applied",true);c.number(L"Pending",true);fake|=ABS_AUTOHIDE;
        ok=c.recover(path)&&fake==ABS_ALWAYSONTOP&&writes==3&&ok;
        ok=c.transition(true)&&ok;fake&=~ABS_AUTOHIDE;unsigned before=writes;
        ok=!c.transition(true)&&writes==before&&fake==ABS_ALWAYSONTOP&&ok;
        c.disable();DeleteFileW(path.c_str());return ok;
    }
};
}
