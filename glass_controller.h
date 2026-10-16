#pragma once
#include <set>
#include <ocidl.h>
#include "glass_protocol.h"
namespace glass {
class Controller {
    HANDLE mapping{};Config*data{};HMODULE module{};HOOKPROC hook{};UINT attach{},refresh{};
    std::set<DWORD> threads;std::map<DWORD,std::wstring> processes;
    std::wstring dllPath;bool failed=false;
    std::wstring process(DWORD id){auto found=processes.find(id);if(found!=processes.end())return found->second;wchar_t path[32768]{};DWORD size=32768;HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,id);if(p){QueryFullProcessImageNameW(p,0,path,&size);CloseHandle(p);}std::wstring value=path;auto at=value.find_last_of(L"\\");if(at!=std::wstring::npos)value.erase(0,at+1);return processes[id]=value;}
    bool target(HWND h){DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId())return false;auto exe=process(pid);wchar_t cls[128]{};GetClassNameW(h,cls,128);DWORD f=data?data->features:0;
        if(_wcsicmp(exe.c_str(),L"explorer.exe")==0){if((f&(Volume|Brightness|Taskbar))&&(wcscmp(cls,L"Shell_TrayWnd")==0||wcscmp(cls,L"Shell_SecondaryTrayWnd")==0||wcscmp(cls,L"XamlExplorerHostIslandWindow")==0))return true;return (f&Explorer)&&(wcscmp(cls,L"CabinetWClass")==0||wcscmp(cls,L"ExploreWClass")==0);}
        return ((f&Start)&&_wcsicmp(exe.c_str(),L"StartMenuExperienceHost.exe")==0)||((f&Notifications)&&_wcsicmp(exe.c_str(),L"ShellExperienceHost.exe")==0);
    }
    void attachWindow(HWND h){DWORD tid=GetWindowThreadProcessId(h,nullptr);if(!tid||threads.count(tid))return;auto installed=SetWindowsHookExW(WH_CALLWNDPROC,hook,module,tid);if(!installed){failed=true;return;}
        DWORD_PTR answer=0;if(SendMessageTimeoutW(h,attach,GetCurrentProcessId(),0,SMTO_ABORTIFHUNG|SMTO_BLOCK,300,&answer))threads.insert(tid);else failed=true;UnhookWindowsHookEx(installed);
    }
    static BOOL CALLBACK child(HWND h,LPARAM param){auto self=(Controller*)param;wchar_t name[128]{};GetClassNameW(h,name,128);if(wcsstr(name,L"Xaml")||wcsstr(name,L"DesktopWindow"))self->attachWindow(h);return TRUE;}
    static BOOL CALLBACK window(HWND h,LPARAM param){auto self=(Controller*)param;if(self->target(h)){self->attachWindow(h);EnumChildWindows(h,child,param);}return TRUE;}
public:
    ~Controller(){shutdown();}
    bool initialize(const std::wstring&dir){if(data)return true;
        // Extract the exact engine embedded in this EXE. A versioned filename
        // allows an old shell session to release its DLL naturally on sign-out.
        dllPath=dir+L"LiquidGlass.Engine-0.10.0.dll";auto resource=FindResourceW(nullptr,MAKEINTRESOURCEW(201),RT_RCDATA);if(!resource)return false;DWORD size=SizeofResource(nullptr,resource);auto bytes=LockResource(LoadResource(nullptr,resource));
        bool identical=false;HANDLE existing=CreateFileW(dllPath.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,0,nullptr);if(existing!=INVALID_HANDLE_VALUE){LARGE_INTEGER length{};GetFileSizeEx(existing,&length);if(length.QuadPart==size){std::vector<BYTE> content(size);DWORD n=0;if(ReadFile(existing,content.data(),size,&n,nullptr)&&n==size)identical=memcmp(content.data(),bytes,size)==0;}CloseHandle(existing);}
        if(!identical){HANDLE file=CreateFileW(dllPath.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(file==INVALID_HANDLE_VALUE)return false;DWORD n=0;BOOL ok=WriteFile(file,bytes,size,&n,nullptr);CloseHandle(file);if(!ok||n!=size)return false;}
        module=LoadLibraryExW(dllPath.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);if(!module)return false;hook=(HOOKPROC)GetProcAddress(module,"GlassHook");if(!hook)return false;
        auto name=mappingName(GetCurrentProcessId());mapping=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(Config),name.c_str());if(!mapping)return false;data=(Config*)MapViewOfFile(mapping,FILE_MAP_ALL_ACCESS,0,0,sizeof(Config));if(!data)return false;Config initial;initial.owner=GetCurrentProcessId();*data=initial;attach=RegisterWindowMessageW(AttachMessage);refresh=RegisterWindowMessageW(RefreshMessage);return true;
    }
    bool available()const{return data&&hook;}
    bool factoryTest(){
        using GetFactory=HRESULT(WINAPI*)(REFCLSID,REFIID,void**);
        auto get=module?(GetFactory)GetProcAddress(module,"DllGetClassObject"):nullptr;if(!get)return false;
        IClassFactory*f=nullptr;IObjectWithSite*s=nullptr;
        HRESULT hr=get(TapId,IID_IClassFactory,reinterpret_cast<void**>(&f));if(FAILED(hr)||!f)return false;
        hr=f->CreateInstance(nullptr,IID_IObjectWithSite,reinterpret_cast<void**>(&s));f->Release();if(FAILED(hr)||!s)return false;
        IUnknown*site=nullptr;bool ok=s->GetSite(IID_IUnknown,reinterpret_cast<void**>(&site))==E_FAIL;if(site)site->Release();s->Release();return ok;
    }
    void publish(Config c){if(!data)return;c=normalize(c);c.owner=GetCurrentProcessId();InterlockedIncrement(&data->sequence);MemoryBarrier();data->features=c.features;data->opacity=c.opacity;data->radius=c.radius;data->tint=c.tint;data->economy=c.economy;MemoryBarrier();InterlockedIncrement(&data->sequence);}
    void broadcast(){if(!data)return;HWND h=nullptr;while((h=FindWindowExW(HWND_MESSAGE,h,ThreadClass,nullptr))){DWORD_PTR value=0;SendMessageTimeoutW(h,refresh,0,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,200,&value);}}
    bool discover(){if(!data)return false;failed=false;threads.clear();processes.clear();if(data->features)EnumWindows(window,reinterpret_cast<LPARAM>(this));broadcast();return !failed;}
    unsigned sessions()const{unsigned count=0;HWND h=nullptr;while((h=FindWindowExW(HWND_MESSAGE,h,ThreadClass,nullptr)))++count;return count;}
    void shutdown(){if(data){Config c=*data;c.features=0;publish(c);broadcast();UnmapViewOfFile(data);data=nullptr;}if(mapping){CloseHandle(mapping);mapping=nullptr;}if(module){FreeLibrary(module);module=nullptr;}threads.clear();processes.clear();}
};
}
