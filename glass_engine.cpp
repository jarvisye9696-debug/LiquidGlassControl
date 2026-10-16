// Original standalone material engine. Uses Windows XAML diagnostics, not the
// Windhawk runtime, SDK, settings registry, theme parser, or bundled mod DLLs.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define UNICODE
#include <windows.h>
#include <objbase.h>
#include <ocidl.h>
#include <roapi.h>
#include <winstring.h>
#include <xamlom.h>
#include <weakreference.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <atomic>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include "glass_protocol.h"
namespace {
template<class T> struct Ptr {
    T*p=nullptr;Ptr()=default;explicit Ptr(T*v):p(v){};~Ptr(){if(p)p->Release();}
    Ptr(const Ptr&o):p(o.p){if(p)p->AddRef();}Ptr(Ptr&&o)noexcept:p(o.p){o.p=nullptr;}
    Ptr&operator=(Ptr o){std::swap(p,o.p);return *this;}T*operator->()const{return p;}explicit operator bool()const{return p!=nullptr;}
    T**put(){if(p){p->Release();p=nullptr;}return &p;}
};
struct XamlReader: IInspectable {virtual HRESULT STDMETHODCALLTYPE Load(HSTRING,IInspectable**)=0;virtual HRESULT STDMETHODCALLTYPE LoadValidated(HSTRING,IInspectable**)=0;};
constexpr GUID ReaderIds[]={{0x9891c6bd,0x534f,0x4955,{0xb8,0x5a,0x8a,0x8d,0xc0,0xdc,0xa6,0x02}},{0x82a4cd9e,0x435e,0x5aeb,{0x8c,0x4f,0x30,0x0c,0xec,0xe4,0x5c,0xae}}};
// Optional diagnostics cache-release ABI. If unavailable, decline attachment
// rather than keep a strong reference to every element reported by the shell.
constexpr GUID ReleaseId={0x735941a2,0x3ee3,0x495a,{0x8d,0xa9,0x97,0x26,0x27,0x00,0x30,0x75}};
struct CacheRelease:IUnknown{virtual HRESULT STDMETHODCALLTYPE Unregister(InstanceHandle)=0;};
HMODULE selfModule{};HANDLE mapping{};glass::Config*shared{};HANDLE ownerHandle{},ownerWait{};DWORD ownerPid{};SRWLOCK configLock=SRWLOCK_INIT;
UINT refreshMsg{},attachMsg{};std::atomic<bool> injecting[2]{};
bool snapshot(glass::Config&c){AcquireSRWLockShared(&configLock);bool valid=false;if(shared)for(int i=0;i<3;++i){LONG s=shared->sequence;if(s&1)continue;MemoryBarrier();c=*shared;MemoryBarrier();if(s==shared->sequence){valid=c.magic==glass::Magic&&c.version==glass::Version;break;}}if(ownerHandle&&WaitForSingleObject(ownerHandle,0)==WAIT_OBJECT_0)c.features=0;ReleaseSRWLockShared(&configLock);return valid;}
void refreshWindows(){HWND h=nullptr;while((h=FindWindowExW(HWND_MESSAGE,h,glass::ThreadClass,nullptr))){DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId())PostMessageW(h,refreshMsg,0,0);}}
void CALLBACK ownerEnded(void*,BOOLEAN){refreshWindows();}
bool connect(DWORD pid){
    AcquireSRWLockExclusive(&configLock);
    if(shared&&ownerPid==pid){ReleaseSRWLockExclusive(&configLock);return true;}
    if(shared&&ownerHandle&&WaitForSingleObject(ownerHandle,0)!=WAIT_OBJECT_0){ReleaseSRWLockExclusive(&configLock);return false;}
    if(ownerWait){UnregisterWaitEx(ownerWait,INVALID_HANDLE_VALUE);ownerWait=nullptr;}if(ownerHandle){CloseHandle(ownerHandle);ownerHandle=nullptr;}if(shared){UnmapViewOfFile(shared);shared=nullptr;}if(mapping){CloseHandle(mapping);mapping=nullptr;}
    auto name=glass::mappingName(pid);mapping=OpenFileMappingW(FILE_MAP_READ,FALSE,name.c_str());if(!mapping){ReleaseSRWLockExclusive(&configLock);return false;}
    shared=(glass::Config*)MapViewOfFile(mapping,FILE_MAP_READ,0,0,sizeof(glass::Config));
    if(!shared||shared->magic!=glass::Magic||shared->version!=glass::Version||shared->owner!=pid){if(shared)UnmapViewOfFile(shared);shared=nullptr;CloseHandle(mapping);mapping=nullptr;ReleaseSRWLockExclusive(&configLock);return false;}
    ownerPid=pid;refreshMsg=RegisterWindowMessageW(glass::RefreshMessage);attachMsg=RegisterWindowMessageW(glass::AttachMessage);
    ownerHandle=OpenProcess(SYNCHRONIZE,FALSE,pid);if(!ownerHandle){ReleaseSRWLockExclusive(&configLock);return false;}
    RegisterWaitForSingleObject(&ownerWait,ownerHandle,ownerEnded,nullptr,INFINITE,WT_EXECUTEONLYONCE);
    // Diagnostics owns COM objects whose callbacks can outlive the hook.
    // Pin this small DLL until the shell process exits; all element references
    // and overrides are released on Off / controller exit.
    HMODULE pinned{};GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&connect),&pinned);
    ReleaseSRWLockExclusive(&configLock);return true;
}
struct Saved {unsigned index{};bool local=false;Ptr<IInspectable> value;};
struct Node {Ptr<IWeakReference> weak;glass::Surface surface=glass::NoSurface;std::map<std::wstring,Saved> saved;};
class Session final:public IObjectWithSite,public IVisualTreeServiceCallback2 {
    std::atomic<ULONG> refs{1};Ptr<IUnknown> site;Ptr<IXamlDiagnostics> diag;Ptr<IVisualTreeService2> service;Ptr<CacheRelease> cache;
    struct Thread {Session*session{};HWND window{};std::map<InstanceHandle,Node> nodes;std::vector<InstanceHandle> release;bool updating=false;};
    std::map<DWORD,Thread*> threads;SRWLOCK lock=SRWLOCK_INIT;bool modern=false;int host=0;std::atomic<HRESULT> lastError{S_OK};std::atomic<unsigned> writes{0};
    static LRESULT CALLBACK windowProc(HWND h,UINT msg,WPARAM w,LPARAM l){
        auto t=(Thread*)GetWindowLongPtrW(h,GWLP_USERDATA);
        if(msg==WM_NCCREATE){t=(Thread*)((CREATESTRUCTW*)l)->lpCreateParams;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)t);}
        if(t){if(msg==WM_APP+1){auto list=std::move(t->release);t->release.clear();for(auto v:list)t->session->cache->Unregister(v);return 0;}
            if(msg==refreshMsg){t->session->update(*t);return 0;}
            if(msg==WM_APP+2)return (LRESULT)t->session->writes.load();
            if(msg==WM_APP+3)return (LRESULT)t->session->lastError.load();
            if(msg==WM_NCDESTROY){auto s=t->session;AcquireSRWLockExclusive(&s->lock);s->threads.erase(GetCurrentThreadId());ReleaseSRWLockExclusive(&s->lock);SetWindowLongPtrW(h,GWLP_USERDATA,0);delete t;}
        }return DefWindowProcW(h,msg,w,l);
    }
    Thread&thread(){DWORD id=GetCurrentThreadId();AcquireSRWLockExclusive(&lock);auto&ptr=threads[id];
        if(!ptr){ptr=new Thread;ptr->session=this;WNDCLASSW c{};c.lpfnWndProc=windowProc;c.hInstance=selfModule;c.lpszClassName=glass::ThreadClass;RegisterClassW(&c);ptr->window=CreateWindowW(c.lpszClassName,L"XAML material session",0,0,0,0,0,HWND_MESSAGE,nullptr,selfModule,ptr);}
        ReleaseSRWLockExclusive(&lock);return *ptr;
    }
    Ptr<IInspectable> from(InstanceHandle h){Ptr<IInspectable> p;diag->GetIInspectableFromHandle(h,p.put());return p;}
    void releaseLater(Thread&t,InstanceHandle h){if(h){bool first=t.release.empty();t.release.push_back(h);if(first)PostMessageW(t.window,WM_APP+1,0,0);}}
    Ptr<IInspectable> brush(const glass::Config&c,bool transparent){
        Ptr<XamlReader> reader;const wchar_t*cls=modern?L"Microsoft.UI.Xaml.Markup.XamlReader":L"Windows.UI.Xaml.Markup.XamlReader";
        HSTRING className{};WindowsCreateString(cls,(UINT32)wcslen(cls),&className);HRESULT hr=RoGetActivationFactory(className,ReaderIds[modern?1:0],reinterpret_cast<void**>(reader.put()));
        if(FAILED(hr)&&modern){auto mod=GetModuleHandleW(L"Microsoft.UI.Xaml.dll");auto fn=mod?reinterpret_cast<HRESULT(WINAPI*)(HSTRING,IActivationFactory**)>(GetProcAddress(mod,"DllGetActivationFactory")):nullptr;Ptr<IActivationFactory> factory;if(fn&&SUCCEEDED(fn(className,factory.put())))hr=factory->QueryInterface(ReaderIds[1],reinterpret_cast<void**>(reader.put()));}
        WindowsDeleteString(className);if(FAILED(hr)||!reader){lastError=hr;return {};}
        bool dark=c.tint==2;if(c.tint==0){DWORD light=1,size=4;RegGetValueW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",L"AppsUseLightTheme",RRF_RT_REG_DWORD,nullptr,&light,&size);dark=!light;}
        auto color=dark?L"#FF18202C":L"#FFF4F7FC";std::wstring xml;
        if(transparent)xml=L"<SolidColorBrush xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Color=\"Transparent\"/>";
        else xml=L"<AcrylicBrush xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" "+std::wstring(modern?L"":L"BackgroundSource=\"HostBackdrop\" ")+L"TintColor=\""+std::wstring(color)+L"\" TintOpacity=\""+std::to_wstring(c.opacity/100.0)+L"\" FallbackColor=\""+color+L"\" AlwaysUseFallback=\""+(c.economy?L"True":L"False")+L"\"/>";
        HSTRING source{};WindowsCreateString(xml.c_str(),(UINT32)xml.size(),&source);Ptr<IInspectable> result;hr=reader->Load(source,result.put());WindowsDeleteString(source);if(FAILED(hr))lastError=hr;return result;
    }
    bool save(Node&n,InstanceHandle h,const std::wstring&prop,unsigned index){
        if(n.saved.count(prop))return true;
        unsigned ns=0,nv=0;PropertyChainSource*s=nullptr;PropertyChainValue*v=nullptr;HRESULT hr=service->GetPropertyValuesChain(h,&ns,&s,&nv,&v);if(FAILED(hr))return false;
        bool local=false,binding=false;
        for(unsigned i=0;i<nv;++i)if(v[i].Index==index&&!v[i].Overridden){local=v[i].PropertyChainIndex<ns&&s[v[i].PropertyChainIndex].Source==BaseValueSourceLocal;binding=(v[i].MetadataBits&IsValueBindingExpression)!=0;break;}
        for(unsigned i=0;i<ns;++i){SysFreeString(s[i].TargetType);SysFreeString(s[i].Name);SysFreeString(s[i].SrcInfo.FileName);SysFreeString(s[i].SrcInfo.Hash);}CoTaskMemFree(s);
        for(unsigned i=0;i<nv;++i){SysFreeString(v[i].Type);SysFreeString(v[i].DeclaringType);SysFreeString(v[i].ValueType);SysFreeString(v[i].ItemType);SysFreeString(v[i].Value);SysFreeString(v[i].PropertyName);}CoTaskMemFree(v);
        if(binding)return false;Saved old;old.index=index;old.local=local;
        if(local){InstanceHandle value=0;hr=service->GetProperty(h,index,&value);if(FAILED(hr))return false;if(value){old.value=from(value);cache->Unregister(value);if(!old.value)return false;}}
        n.saved.emplace(prop,std::move(old));return true;
    }
    void set(Node&n,InstanceHandle h,const std::wstring&prop,IInspectable*value){
        unsigned index=0;if(FAILED(service->GetPropertyIndex(h,prop.c_str(),&index))||!save(n,h,prop,index))return;
        InstanceHandle v=0;if(value&&FAILED(diag->RegisterInstance(value,&v)))return;
        HRESULT hr=service->SetProperty(h,v,index);if(v)cache->Unregister(v);if(FAILED(hr))lastError=hr;else ++writes;
    }
    void restore(Node&n,InstanceHandle h){for(auto i=n.saved.begin();i!=n.saved.end();){auto&v=i->second;HRESULT hr=S_OK;InstanceHandle old=0;
            if(v.local){if(v.value)hr=diag->RegisterInstance(v.value.p,&old);if(SUCCEEDED(hr))hr=service->SetProperty(h,old,v.index);if(old)cache->Unregister(old);}else hr=service->ClearProperty(h,v.index);
            if(SUCCEEDED(hr))i=n.saved.erase(i);else{lastError=hr;++i;}}
    }
    void apply(Node&n,InstanceHandle h,const glass::Config&c){
        if(!glass::active(n.surface,c.features)){restore(n,h);return;}
        auto b=brush(c,n.surface==glass::OsdPlate||n.surface==glass::StartOverlay);if(!b)return;
        set(n,h,n.surface==glass::TaskbarFill?L"Fill":L"Background",b.p);
        // Round only native material plates; never change layout constraints.
        if(n.surface!=glass::TaskbarFill&&n.surface!=glass::ExplorerPlate&&n.surface!=glass::OsdPlate){
            InstanceHandle r=0;BSTR type=SysAllocString(modern?L"Microsoft.UI.Xaml.CornerRadius":L"Windows.UI.Xaml.CornerRadius");auto text=std::to_wstring(c.radius);BSTR value=SysAllocString(text.c_str());
            if(SUCCEEDED(service->CreateInstance(type,value,&r))){auto p=from(r);if(p)set(n,h,L"CornerRadius",p.p);cache->Unregister(r);}SysFreeString(type);SysFreeString(value);
        }
    }
    void update(Thread&t){if(t.updating)return;t.updating=true;glass::Config c;if(!snapshot(c))c.features=0;
        for(auto i=t.nodes.begin();i!=t.nodes.end();){Ptr<IInspectable> p;if(i->second.weak) i->second.weak->Resolve(__uuidof(IInspectable),p.put());if(!p){i=t.nodes.erase(i);continue;}
            InstanceHandle h=0;if(SUCCEEDED(diag->RegisterInstance(p.p,&h))){apply(i->second,h,c);cache->Unregister(h);}++i;}
        t.updating=false;
    }
public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**out)override{if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==IID_IObjectWithSite)*out=static_cast<IObjectWithSite*>(this);else if(id==__uuidof(IVisualTreeServiceCallback)||id==__uuidof(IVisualTreeServiceCallback2))*out=static_cast<IVisualTreeServiceCallback2*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{ULONG n=--refs;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetSite(REFIID id,void**out)override{return site?site->QueryInterface(id,out):E_FAIL;}
    HRESULT STDMETHODCALLTYPE SetSite(IUnknown*p)override{
        if(!p)return S_OK;site=Ptr<IUnknown>(p);p->AddRef();HRESULT hr=p->QueryInterface(__uuidof(IXamlDiagnostics),reinterpret_cast<void**>(diag.put()));if(FAILED(hr))return hr;
        if(FAILED(hr=p->QueryInterface(__uuidof(IVisualTreeService2),reinterpret_cast<void**>(service.put())))||FAILED(hr=p->QueryInterface(ReleaseId,reinterpret_cast<void**>(cache.put()))))return hr;
        BSTR init=nullptr;if(SUCCEEDED(diag->GetInitializationData(&init))&&init){std::wstring v=init;modern=v.find(L"modern")!=std::wstring::npos;DWORD owner=wcstoul(v.c_str(),nullptr,10);SysFreeString(init);if(!connect(owner))return E_FAIL;}
        if(!shared)return E_FAIL;wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);std::wstring path=exe;
        if(path.find(L"StartMenuExperienceHost.exe")!=std::wstring::npos)host=1;else if(path.find(L"ShellExperienceHost.exe")!=std::wstring::npos)host=2;else host=modern?3:0;
        AddRef();auto worker=CreateThread(nullptr,0,[](void*ctx)->DWORD{auto s=(Session*)ctx;CoInitializeEx(nullptr,COINIT_MULTITHREADED);s->lastError=s->service->AdviseVisualTreeChange(s);s->Release();CoUninitialize();return 0;},this,0,nullptr);if(worker)CloseHandle(worker);else Release();return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation relation,VisualElement element,VisualMutationType mutation)override{
        try{auto&t=thread();if(mutation==Add){auto surface=glass::identify(element.Name?element.Name:L"",element.Type?element.Type:L"",host);
                if(surface!=glass::NoSurface){auto p=from(element.Handle);Ptr<IWeakReferenceSource> source;if(p&&SUCCEEDED(p->QueryInterface(__uuidof(IWeakReferenceSource),reinterpret_cast<void**>(source.put())))){
                    Node n;n.surface=surface;source->GetWeakReference(n.weak.put());auto old=t.nodes.find(element.Handle);if(old!=t.nodes.end())restore(old->second,element.Handle);t.nodes[element.Handle]=std::move(n);
                    glass::Config c;if(snapshot(c)){t.updating=true;apply(t.nodes[element.Handle],element.Handle,c);t.updating=false;}
                }}releaseLater(t,element.Handle);releaseLater(t,relation.Parent);
            }else if(mutation==Remove){auto i=t.nodes.find(element.Handle);if(i!=t.nodes.end()){restore(i->second,element.Handle);t.nodes.erase(i);}releaseLater(t,element.Handle);}
        }catch(...){lastError=E_FAIL;}return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnElementStateChanged(InstanceHandle,VisualElementState,LPCWSTR)override{return S_OK;}
};
class Factory final:public IClassFactory {std::atomic<ULONG>refs{1};public:
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**p)override{if(!p)return E_POINTER;*p=nullptr;if(id!=IID_IUnknown&&id!=IID_IClassFactory)return E_NOINTERFACE;*p=this;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{ULONG n=--refs;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown*outer,REFIID id,void**p)override{if(outer)return CLASS_E_NOAGGREGATION;auto s=new Session;auto hr=s->QueryInterface(id,p);s->Release();return hr;}
    HRESULT STDMETHODCALLTYPE LockServer(BOOL)override{return S_OK;}
};
DWORD WINAPI attachFramework(void*arg){int modern=(int)(INT_PTR)arg;CoInitializeEx(nullptr,COINIT_MULTITHREADED);
    HMODULE module=GetModuleHandleW(modern?L"Microsoft.Internal.FrameworkUdk.dll":L"Windows.UI.Xaml.dll");
    auto fn=module?reinterpret_cast<decltype(&InitializeXamlDiagnosticsEx)>(GetProcAddress(module,"InitializeXamlDiagnosticsEx")):nullptr;
    HRESULT result=E_FAIL;if(fn){wchar_t path[32768]{};GetModuleFileNameW(selfModule,path,32768);auto init=std::to_wstring(ownerPid)+(modern?L" modern":L" classic");
        for(int i=1;i<=128;++i){auto endpoint=std::wstring(modern?L"WinUIVisualDiagConnection":L"VisualDiagConnection")+std::to_wstring(i);result=fn(endpoint.c_str(),GetCurrentProcessId(),L"",path,glass::TapId,init.c_str());if(result!=HRESULT_FROM_WIN32(ERROR_NOT_FOUND))break;}
    }if(FAILED(result))injecting[modern]=false;CoUninitialize();return 0;
}
}
extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID id,REFIID iid,void**p){if(id!=glass::TapId)return CLASS_E_CLASSNOTAVAILABLE;auto f=new Factory;auto hr=f->QueryInterface(iid,p);f->Release();return hr;}
extern "C" HRESULT WINAPI DllCanUnloadNow(){return S_FALSE;}
extern "C" __declspec(dllexport) LRESULT CALLBACK GlassHook(int code,WPARAM w,LPARAM l){
    if(code>=0){auto m=(CWPSTRUCT*)l;if(m->message==RegisterWindowMessageW(glass::AttachMessage)&&connect((DWORD)m->wParam)){
        for(int modern=0;modern<2;++modern)if(GetModuleHandleW(modern?L"Microsoft.Internal.FrameworkUdk.dll":L"Windows.UI.Xaml.dll")&&!injecting[modern].exchange(true)){HANDLE t=CreateThread(nullptr,0,attachFramework,(void*)(INT_PTR)modern,0,nullptr);if(t)CloseHandle(t);else injecting[modern]=false;}
        refreshWindows();
    }}return CallNextHookEx(nullptr,code,w,l);
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){selfModule=instance;DisableThreadLibraryCalls(instance);}return TRUE;}
