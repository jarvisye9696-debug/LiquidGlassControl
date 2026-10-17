#pragma once
#include <set>
#include <ocidl.h>
#include <tlhelp32.h>
#include <cstdio>
#include "glass_protocol.h"
namespace glass {
class Controller {
    HANDLE mapping{};Config*data{};HMODULE module{};HOOKPROC hook{},messageHook{};UINT attach{},refresh{};std::map<DWORD,HHOOK> pendingHooks;
    std::set<DWORD> threads;std::map<DWORD,std::wstring> processes;
    std::wstring dllPath,windowClass;bool failed=false;DWORD loadError=0;
    void trace(const char*stage,DWORD value){if(strcmp(stage,"packaged shell processes")==0)return;auto at=dllPath.find_last_of(L"\\");auto file=dllPath.substr(0,at+1)+L"glass-controller.log";HANDLE h=CreateFileW(file.c_str(),FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,0,nullptr);if(h!=INVALID_HANDLE_VALUE){char line[120]{};int n=snprintf(line,sizeof(line),"%s: %lu\r\n",stage,(unsigned long)value);DWORD written=0;WriteFile(h,line,n,&written,nullptr);CloseHandle(h);}}
    std::wstring process(DWORD id){auto found=processes.find(id);if(found!=processes.end())return found->second;wchar_t path[32768]{};DWORD size=32768;HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,id);if(p){QueryFullProcessImageNameW(p,0,path,&size);CloseHandle(p);}std::wstring value=path;auto at=value.find_last_of(L"\\");if(at!=std::wstring::npos)value.erase(0,at+1);return processes[id]=value;}
    bool target(HWND h){DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId())return false;auto exe=process(pid);wchar_t cls[128]{};GetClassNameW(h,cls,128);DWORD f=data?data->features:0;
        if(_wcsicmp(exe.c_str(),L"explorer.exe")==0){if((f&(Volume|Brightness|Taskbar|Dock))&&(wcscmp(cls,L"Shell_TrayWnd")==0||wcscmp(cls,L"Shell_SecondaryTrayWnd")==0||wcscmp(cls,L"XamlExplorerHostIslandWindow")==0))return true;return (f&Explorer)&&(wcscmp(cls,L"CabinetWClass")==0||wcscmp(cls,L"ExploreWClass")==0);}
        return ((f&Start)&&_wcsicmp(exe.c_str(),L"StartMenuExperienceHost.exe")==0)||((f&Notifications)&&(_wcsicmp(exe.c_str(),L"ShellExperienceHost.exe")==0||_wcsicmp(exe.c_str(),L"ShellHost.exe")==0));
    }
    void attachWindow(HWND h){DWORD tid=GetWindowThreadProcessId(h,nullptr);if(!tid||threads.count(tid))return;auto installed=SetWindowsHookExW(WH_CALLWNDPROC,hook,module,tid);if(!installed){failed=true;trace("hook error",GetLastError());return;}
        DWORD_PTR answer=0;if(SendMessageTimeoutW(h,attach,GetCurrentProcessId(),0,SMTO_ABORTIFHUNG|SMTO_BLOCK,300,&answer))threads.insert(tid);else failed=true;UnhookWindowsHookEx(installed);
    }
    static BOOL CALLBACK child(HWND h,LPARAM param){auto self=(Controller*)param;wchar_t name[128]{};GetClassNameW(h,name,128);if(wcsstr(name,L"Xaml")||wcsstr(name,L"DesktopWindow"))self->attachWindow(h);return TRUE;}
    static BOOL CALLBACK window(HWND h,LPARAM param){auto self=(Controller*)param;if(self->target(h)){self->attachWindow(h);EnumChildWindows(h,child,param);}return TRUE;}
    static BOOL CALLBACK threadWindow(HWND h,LPARAM param){auto self=(Controller*)param;self->attachWindow(h);return TRUE;}
    void attachThread(DWORD tid){if(threads.count(tid)||pendingHooks.count(tid)||pendingHooks.size()>=32)return;HHOOK h=SetWindowsHookExW(WH_GETMESSAGE,messageHook,module,tid);if(!h)return;if(!PostThreadMessageW(tid,attach,GetCurrentProcessId(),0)){UnhookWindowsHookEx(h);return;}pendingHooks[tid]=h;auto window=FindWindowW(L"LiquidGlass.Control",nullptr);if(window)SetTimer(window,5,2200,nullptr);}
    void discoverPackagedShell(){
        std::set<DWORD> ids;HANDLE p=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);if(p!=INVALID_HANDLE_VALUE){PROCESSENTRY32W e{};e.dwSize=sizeof(e);if(Process32FirstW(p,&e))do{bool start=(data->features&Start)&&_wcsicmp(e.szExeFile,L"StartMenuExperienceHost.exe")==0;bool notifications=(data->features&Notifications)&&(_wcsicmp(e.szExeFile,L"ShellExperienceHost.exe")==0||_wcsicmp(e.szExeFile,L"ShellHost.exe")==0);if(start||notifications)ids.insert(e.th32ProcessID);}while(Process32NextW(p,&e));CloseHandle(p);}
        trace("packaged shell processes",(DWORD)ids.size());if(ids.empty())return;HANDLE t=CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD,0);if(t==INVALID_HANDLE_VALUE)return;THREADENTRY32 e{};e.dwSize=sizeof(e);if(Thread32First(t,&e))do{if(ids.count(e.th32OwnerProcessID)){EnumThreadWindows(e.th32ThreadID,threadWindow,reinterpret_cast<LPARAM>(this));attachThread(e.th32ThreadID);}}while(Thread32Next(t,&e));CloseHandle(t);
    }
public:
    ~Controller(){shutdown();}
    bool initialize(const std::wstring&dir){if(data)return true;
        // Extract the exact engine embedded in this EXE. A versioned filename
        // allows an old shell session to release its DLL naturally on sign-out.
        auto resource=FindResourceW(nullptr,MAKEINTRESOURCEW(201),RT_RCDATA);if(!resource)return false;DWORD size=SizeofResource(nullptr,resource);auto bytes=LockResource(LoadResource(nullptr,resource));
        unsigned long long hash=14695981039346656037ull;for(DWORD i=0;i<size;++i){hash^=((const BYTE*)bytes)[i];hash*=1099511628211ull;}dllPath=dir+L"LiquidGlass.Engine-"+std::to_wstring(hash)+L".dll";
        windowClass=threadClassName(dllPath);bool identical=false;HANDLE existing=CreateFileW(dllPath.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);if(existing!=INVALID_HANDLE_VALUE){LARGE_INTEGER length{};GetFileSizeEx(existing,&length);if(length.QuadPart==size){std::vector<BYTE> content(size);DWORD n=0;if(ReadFile(existing,content.data(),size,&n,nullptr)&&n==size)identical=memcmp(content.data(),bytes,size)==0;}CloseHandle(existing);}
        if(!identical){HANDLE file=CreateFileW(dllPath.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return false;DWORD n=0;BOOL ok=WriteFile(file,bytes,size,&n,nullptr);CloseHandle(file);if(!ok||n!=size)return false;}
        DWORD previousMode=0;BOOL changedMode=SetThreadErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX,&previousMode);
        module=LoadLibraryExW(dllPath.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);loadError=module?0:GetLastError();
        if(changedMode)SetThreadErrorMode(previousMode,nullptr);
        if(!module){trace("engine load error",loadError);return false;}hook=(HOOKPROC)GetProcAddress(module,"GlassHook");messageHook=(HOOKPROC)GetProcAddress(module,"GlassMessageHook");if(!hook||!messageHook){trace("engine export error",GetLastError());return false;}
        auto name=mappingName(GetCurrentProcessId());mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(Config),name.c_str());if(!mapping)return false;data=(Config*)MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(Config));if(!data)return false;Config initial;initial.owner=GetCurrentProcessId();*data=initial;attach=RegisterWindowMessageW(AttachMessage);refresh=RegisterWindowMessageW(RefreshMessage);return true;
    }
    bool available()const{return data&&hook;}
    bool policyBlocked()const{return loadError==4551||loadError==577||loadError==1260;}
    void acknowledge(DWORD tid){auto i=pendingHooks.find(tid);if(i!=pendingHooks.end()){UnhookWindowsHookEx(i->second);pendingHooks.erase(i);threads.insert(tid);}}
    void expireHooks(){for(auto&i:pendingHooks)UnhookWindowsHookEx(i.second);pendingHooks.clear();}
    const std::wstring&runtimePath()const{return dllPath;}
    bool factoryTest(){
        using GetFactory=HRESULT(WINAPI*)(REFCLSID,REFIID,void**);
        auto get=module?(GetFactory)GetProcAddress(module,"DllGetClassObject"):nullptr;if(!get)return false;
        IClassFactory*f=nullptr;IObjectWithSite*s=nullptr;
        HRESULT hr=get(runtimeTapId(dllPath),IID_IClassFactory,reinterpret_cast<void**>(&f));if(FAILED(hr)||!f)return false;
        hr=f->CreateInstance(nullptr,IID_IObjectWithSite,reinterpret_cast<void**>(&s));f->Release();if(FAILED(hr)||!s)return false;
        IUnknown*site=nullptr;bool ok=s->GetSite(IID_IUnknown,reinterpret_cast<void**>(&site))==E_FAIL;if(site)site->Release();s->Release();return ok;
    }
    HRESULT graphicsTest(){using Test=HRESULT(WINAPI*)();auto test=module?(Test)GetProcAddress(module,"GlassGraphicsTest"):nullptr;return test?test():E_NOINTERFACE;}
    void publish(Config c){if(!data)return;c=normalize(c);c.owner=GetCurrentProcessId();InterlockedIncrement(&data->sequence);MemoryBarrier();data->features=c.features;data->opacity=c.opacity;data->radius=c.radius;data->tint=c.tint;data->blur=c.blur;data->economy=c.economy;MemoryBarrier();InterlockedIncrement(&data->sequence);}
    void broadcast(){if(!data)return;HWND h=nullptr;while((h=FindWindowExW(HWND_MESSAGE,h,windowClass.c_str(),nullptr))){DWORD_PTR value=0;SendMessageTimeoutW(h,refresh,0,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,200,&value);}}
    bool discover(){if(!data)return false;failed=false;threads.clear();processes.clear();if(data->features){EnumWindows(window,reinterpret_cast<LPARAM>(this));discoverPackagedShell();}broadcast();return !failed;}
    unsigned sessions()const{unsigned count=0;HWND h=nullptr;while((h=FindWindowExW(HWND_MESSAGE,h,windowClass.c_str(),nullptr)))++count;return count;}
    void shutdown(){expireHooks();if(data){Config c=*data;c.features=0;publish(c);broadcast();UnmapViewOfFile(data);data=nullptr;}if(mapping){CloseHandle(mapping);mapping=nullptr;}if(module){FreeLibrary(module);module=nullptr;}threads.clear();processes.clear();}
};
}
