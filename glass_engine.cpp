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
#include <cstdio>
#include <set>
#include "glass_protocol.h"
#include "composition_glass.h"
#include "dock_animation.h"
#include "xaml_property_watch.h"
#include "xaml_subscription.h"
#include "windhawk_materials.h"
namespace {
template<class T> struct Ptr {
    T*p=nullptr;Ptr()=default;explicit Ptr(T*v):p(v){};~Ptr(){if(p)p->Release();}
    Ptr(const Ptr&o):p(o.p){if(p)p->AddRef();}Ptr(Ptr&&o)noexcept:p(o.p){o.p=nullptr;}
    Ptr&operator=(Ptr o){std::swap(p,o.p);return *this;}T*operator->()const{return p;}explicit operator bool()const{return p!=nullptr;}
    T**put(){if(p){p->Release();p=nullptr;}return &p;}
};
struct XamlReader: IInspectable {virtual HRESULT STDMETHODCALLTYPE Load(HSTRING,IInspectable**)=0;virtual HRESULT STDMETHODCALLTYPE LoadValidated(HSTRING,IInspectable**)=0;};
constexpr GUID ReaderIds[]={{0x9891c6bd,0x534f,0x4955,{0xb8,0x5a,0x8a,0x8d,0xc0,0xdc,0xa6,0x02}},{0x82a4cd9e,0x435e,0x5aeb,{0x8c,0x4f,0x30,0x0c,0xec,0xe4,0x5c,0xae}}};
// Modern WinUI exposes cache release; system XAML on some Windows builds does
// not. Limit observations on that compatibility path instead of refusing all
// system materials merely because this optional interface is missing.
constexpr GUID ReleaseId={0x735941a2,0x3ee3,0x495a,{0x8d,0xa9,0x97,0x26,0x27,0x00,0x30,0x75}};
struct CacheRelease:IUnknown{virtual HRESULT STDMETHODCALLTYPE Unregister(InstanceHandle)=0;};
HMODULE selfModule{};HANDLE mapping{};glass::Config*shared{};HANDLE ownerHandle{},ownerWait{};DWORD ownerPid{};SRWLOCK configLock=SRWLOCK_INIT;
void trace(const char*stage,HRESULT hr){static volatile LONG errors=0;if(SUCCEEDED(hr)||InterlockedIncrement(&errors)>32)return;wchar_t path[32768]{};GetModuleFileNameW(selfModule,path,32768);std::wstring file=path;file.resize(file.find_last_of(L"\\")+1);file+=L"glass-engine-"+std::to_wstring(GetCurrentProcessId())+L".log";HANDLE h=CreateFileW(file.c_str(),FILE_APPEND_DATA,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(h!=INVALID_HANDLE_VALUE){char text[256]{};int n=snprintf(text,sizeof(text),"%s: 0x%08lx\r\n",stage,(unsigned long)hr);DWORD written=0;WriteFile(h,text,n,&written,nullptr);CloseHandle(h);}}
UINT refreshMsg{};std::atomic<bool> injecting[2]{};
const std::wstring&runtimePath(){static std::wstring path=[](){wchar_t value[32768]{};GetModuleFileNameW(selfModule,value,32768);return std::wstring(value);}();return path;}
const std::wstring&threadClass(){static auto name=glass::threadClassName(runtimePath());return name;}
class Session;SRWLOCK sessionsLock=SRWLOCK_INIT;std::vector<Session*> sessions;
// Ok: consistent config. Transient: the controller is mid-publish or the read tore; keep current surfaces and retry. Invalid: no usable config.
using Read=glass::ReadState;
Read snapshot(glass::Config&c){
    AcquireSRWLockShared(&configLock);Read result=Read::Invalid;
    if(shared)result=glass::readSnapshot(shared,c);
    // The controller exited: all effects must be restored regardless of what the mapping still holds.
    if(ownerHandle&&WaitForSingleObject(ownerHandle,0)==WAIT_OBJECT_0){c.features=0;result=Read::Ok;}
    ReleaseSRWLockShared(&configLock);return result;
}
void refreshWindows(){HWND h=nullptr;while((h=FindWindowExW(HWND_MESSAGE,h,threadClass().c_str(),nullptr))){DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid==GetCurrentProcessId())PostMessageW(h,refreshMsg,0,0);}}
void CALLBACK ownerEnded(void*,BOOLEAN){refreshWindows();}
void report(HRESULT hr,DWORD feature=0){auto h=FindWindowW(L"LiquidGlass.Control",nullptr);DWORD pid=0;if(h)GetWindowThreadProcessId(h,&pid);if(pid==ownerPid)PostMessageW(h,RegisterWindowMessageW(glass::StatusMessage),feature,(LPARAM)hr);}
// Caller holds configLock exclusively. Releases every owner-scoped resource; safe on a partially connected state.
void releaseOwnerLocked(){
    if(ownerWait){UnregisterWaitEx(ownerWait,INVALID_HANDLE_VALUE);ownerWait=nullptr;}
    if(ownerHandle){CloseHandle(ownerHandle);ownerHandle=nullptr;}
    if(shared){UnmapViewOfFile(shared);shared=nullptr;}
    if(mapping){CloseHandle(mapping);mapping=nullptr;}
    ownerPid=0;
}
bool connect(DWORD pid){
    AcquireSRWLockExclusive(&configLock);
    // A live owner keeps its configuration; an ended owner must be replaced.
    if(shared&&ownerHandle&&WaitForSingleObject(ownerHandle,0)==WAIT_TIMEOUT){bool same=ownerPid==pid;ReleaseSRWLockExclusive(&configLock);return same;}
    releaseOwnerLocked();
    // Acquire every resource first; publish the connection only after all succeed, so a failure leaves no half-connected state and a later attempt can retry.
    auto name=glass::mappingName(pid);HANDLE m=OpenFileMappingW(FILE_MAP_READ,FALSE,name.c_str());
    if(!m){ReleaseSRWLockExclusive(&configLock);return false;}
    auto view=(glass::Config*)MapViewOfFile(m,FILE_MAP_READ,0,0,sizeof(glass::Config));
    if(!view||view->magic!=glass::Magic||view->version!=glass::Version||view->owner!=pid){if(view)UnmapViewOfFile(view);CloseHandle(m);ReleaseSRWLockExclusive(&configLock);return false;}
    HANDLE owner=OpenProcess(SYNCHRONIZE,FALSE,pid);HANDLE wait=nullptr;
    if(!owner||!RegisterWaitForSingleObject(&wait,owner,ownerEnded,nullptr,INFINITE,WT_EXECUTEONLYONCE)){if(owner)CloseHandle(owner);UnmapViewOfFile(view);CloseHandle(m);ReleaseSRWLockExclusive(&configLock);return false;}
    mapping=m;shared=view;ownerHandle=owner;ownerWait=wait;ownerPid=pid;
    refreshMsg=RegisterWindowMessageW(glass::RefreshMessage);
    // Diagnostics owns COM objects whose callbacks can outlive the hook.
    // Pin this small DLL until the shell process exits; all element references
    // and overrides are released on Off / controller exit.
    HMODULE pinned{};GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&connect),&pinned);
    ReleaseSRWLockExclusive(&configLock);return true;
}
struct Saved {unsigned index{};bool local=false;Ptr<IInspectable> value;};
struct Node {std::vector<const wchar_t*> imported;Ptr<IWeakReference> weak;glass::Surface surface=glass::NoSurface;std::map<std::wstring,Saved> saved;std::map<std::wstring,std::function<void()>> watchers;DWORD appliedSignature=0;bool composition=false,fallback=false;};
class Session final:public IObjectWithSite,public IVisualTreeServiceCallback2 {
    std::atomic<ULONG> refs{1};Ptr<IUnknown> site;Ptr<IXamlDiagnostics> diag;Ptr<IVisualTreeService2> service;Ptr<CacheRelease> cache;
    struct Thread {Session*session{};HWND window{};std::map<InstanceHandle,Node> nodes;std::set<InstanceHandle> release;ULONGLONG lastMutation=0;bool updating=false;bool taskbarPanel=false;unsigned retries{};liquid::DockAnimation dock;~Thread(){for(auto&n:nodes)for(auto&w:n.second.watchers)w.second();}};
    std::map<DWORD,Thread*> threads;SRWLOCK lock=SRWLOCK_INIT;bool modern=false;int host=0;std::atomic<HRESULT> lastError{S_OK};std::atomic<unsigned> writes{0};
    std::atomic<bool> subscribed{false},subscriptionWorker{false};
    std::atomic<unsigned> compatibilityReports{0};
    std::atomic<LONG> notification{S_FALSE};
    void notify(HRESULT hr){if(notification.exchange(hr)!=hr)report(hr);}
    void unregister(InstanceHandle h){if(cache&&h)cache->Unregister(h);}
    bool wantsSubscription(){if(!cache&&compatibilityReports.load()>=8192)return false;glass::Config c;auto state=snapshot(c);if(state==Read::Transient)return subscribed.load();if(state!=Read::Ok)return false;DWORD mask=host==0?(glass::Taskbar|glass::Volume|glass::Brightness|glass::Dock|glass::Explorer):host==1?glass::Start:host==2?glass::Notifications:glass::Explorer;return (c.features&mask)!=0;}
    static LRESULT CALLBACK windowProc(HWND h,UINT msg,WPARAM w,LPARAM l){
        auto t=(Thread*)GetWindowLongPtrW(h,GWLP_USERDATA);
        if(msg==WM_NCCREATE){t=(Thread*)((CREATESTRUCTW*)l)->lpCreateParams;SetWindowLongPtrW(h,GWLP_USERDATA,(LONG_PTR)t);}
        if(t){if(msg==WM_TIMER&&w==9){if(t->session->subscriptionWorker||GetTickCount64()-t->lastMutation<200)return 0;unsigned budget=256;while(!t->release.empty()&&budget--){auto i=t->release.begin();auto value=*i;t->release.erase(i);t->session->unregister(value);}if(t->release.empty())KillTimer(h,9);return 0;}
            if(msg==WM_APP+4){SetTimer(h,8,30,nullptr);return 0;}if(msg==WM_TIMER&&w==8){if(t->session->subscriptionWorker)return 0;KillTimer(h,8);t->session->update(*t);return 0;}
            if(msg==refreshMsg){if(t->session->subscriptionWorker)SetTimer(h,10,100,nullptr);else t->session->update(*t);return 0;}
            if(msg==WM_TIMER&&w==10){if(!t->session->subscriptionWorker){KillTimer(h,10);t->session->update(*t);}return 0;}
            if(msg==WM_TIMER&&w==11){KillTimer(h,11);t->session->update(*t);return 0;}
            if(msg==WM_APP+2)return (LRESULT)t->session->writes.load();
            if(msg==WM_APP+3)return (LRESULT)t->session->lastError.load();
            if(msg==WM_NCDESTROY){auto s=t->session;AcquireSRWLockExclusive(&s->lock);s->threads.erase(GetCurrentThreadId());ReleaseSRWLockExclusive(&s->lock);SetWindowLongPtrW(h,GWLP_USERDATA,0);delete t;}
        }return DefWindowProcW(h,msg,w,l);
    }
    Thread&thread(){DWORD id=GetCurrentThreadId();AcquireSRWLockExclusive(&lock);auto&ptr=threads[id];
        if(!ptr){ptr=new Thread;ptr->session=this;WNDCLASSW c{};c.lpfnWndProc=windowProc;c.hInstance=selfModule;c.lpszClassName=threadClass().c_str();RegisterClassW(&c);ptr->window=CreateWindowW(c.lpszClassName,L"XAML material session",0,0,0,0,0,HWND_MESSAGE,nullptr,selfModule,ptr);}
        ReleaseSRWLockExclusive(&lock);return *ptr;
    }
    Ptr<IInspectable> from(InstanceHandle h){Ptr<IInspectable> p;diag->GetIInspectableFromHandle(h,p.put());return p;}
    void releaseLater(Thread&t,InstanceHandle h){if(cache&&h){bool first=t.release.empty();t.release.insert(h);t.lastMutation=GetTickCount64();if(first)SetTimer(t.window,9,250,nullptr);}}
    Ptr<IInspectable> brush(const glass::Config&c,bool transparent,InstanceHandle handle,DWORD feature,bool&custom,bool&fallback){custom=false;fallback=false;
        if(!transparent)try{auto target=from(handle);if(target){winrt::Windows::Foundation::IInspectable obj{nullptr};winrt::copy_from_abi(obj,target.p);Ptr<IInspectable> result;if(modern){auto element=obj.try_as<winrt::Microsoft::UI::Xaml::UIElement>();if(element){auto value=winrt::make<liquid::ModernBrush>(element,c,[feature](HRESULT hr){trace("composition material",hr);report(hr,feature);});winrt::get_unknown(value)->QueryInterface(__uuidof(IInspectable),(void**)result.put());}}else{auto element=obj.try_as<winrt::Windows::UI::Xaml::UIElement>();if(element){auto value=winrt::make<liquid::SystemBrush>(element,c,[feature](HRESULT hr){trace("composition material",hr);report(hr,feature);});winrt::get_unknown(value)->QueryInterface(__uuidof(IInspectable),(void**)result.put());}}if(result){custom=true;return result;}}}catch(...){fallback=true;trace("composition brush fallback",winrt::to_hresult());}
        Ptr<XamlReader> reader;const wchar_t*cls=modern?L"Microsoft.UI.Xaml.Markup.XamlReader":L"Windows.UI.Xaml.Markup.XamlReader";
        HSTRING className{};WindowsCreateString(cls,(UINT32)wcslen(cls),&className);HRESULT hr=RoGetActivationFactory(className,ReaderIds[modern?1:0],reinterpret_cast<void**>(reader.put()));
        if(FAILED(hr)&&modern){auto mod=GetModuleHandleW(L"Microsoft.UI.Xaml.dll");auto fn=mod?reinterpret_cast<HRESULT(WINAPI*)(HSTRING,IActivationFactory**)>(GetProcAddress(mod,"DllGetActivationFactory")):nullptr;Ptr<IActivationFactory> factory;if(fn&&SUCCEEDED(fn(className,factory.put())))hr=factory->QueryInterface(ReaderIds[1],reinterpret_cast<void**>(reader.put()));}
        WindowsDeleteString(className);if(FAILED(hr)||!reader){lastError=hr;notify(hr);return {};}
        bool dark=c.tint==2;if(c.tint==0){DWORD light=1,size=4;RegGetValueW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",L"AppsUseLightTheme",RRF_RT_REG_DWORD,nullptr,&light,&size);dark=!light;}
        auto color=dark?L"#FF18202C":L"#FFF4F7FC";std::wstring xml;
        if(transparent)xml=L"<SolidColorBrush xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" Color=\"Transparent\"/>";
        else xml=L"<AcrylicBrush xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" "+std::wstring(modern?L"":L"BackgroundSource=\"HostBackdrop\" ")+L"TintColor=\""+std::wstring(color)+L"\" TintOpacity=\""+std::to_wstring(c.opacity/100.0)+L"\" FallbackColor=\""+color+L"\" AlwaysUseFallback=\""+(c.economy?L"True":L"False")+L"\"/>";
        HSTRING source{};WindowsCreateString(xml.c_str(),(UINT32)xml.size(),&source);Ptr<IInspectable> result;hr=reader->Load(source,result.put());WindowsDeleteString(source);if(FAILED(hr)){lastError=hr;notify(hr);}return result;
    }
    bool save(Node&n,InstanceHandle h,const std::wstring&prop,unsigned&index){
        auto existing=n.saved.find(prop);if(existing!=n.saved.end()){index=existing->second.index;return true;}
        unsigned ns=0,nv=0;PropertyChainSource*s=nullptr;PropertyChainValue*v=nullptr;HRESULT hr=service->GetPropertyValuesChain(h,&ns,&s,&nv,&v);if(FAILED(hr)){trace("snapshot property",hr);return false;}
        bool local=false,binding=false,found=false;
        for(unsigned i=0;i<nv;++i)if(v[i].PropertyName&&prop==v[i].PropertyName&&!v[i].Overridden){index=v[i].Index;found=true;local=v[i].PropertyChainIndex<ns&&s[v[i].PropertyChainIndex].Source==BaseValueSourceLocal;binding=(v[i].MetadataBits&IsValueBindingExpression)!=0;break;}
        for(unsigned i=0;i<ns;++i){SysFreeString(s[i].TargetType);SysFreeString(s[i].Name);SysFreeString(s[i].SrcInfo.FileName);SysFreeString(s[i].SrcInfo.Hash);}CoTaskMemFree(s);
        for(unsigned i=0;i<nv;++i){SysFreeString(v[i].Type);SysFreeString(v[i].DeclaringType);SysFreeString(v[i].ValueType);SysFreeString(v[i].ItemType);SysFreeString(v[i].Value);SysFreeString(v[i].PropertyName);}CoTaskMemFree(v);
        if(!found){trace("property missing from chain",(HRESULT)nv);return false;}if(binding){trace("binding skipped",S_FALSE);return false;}Saved old;old.index=index;old.local=local;
        if(local){InstanceHandle value=0;hr=service->GetProperty(h,index,&value);if(FAILED(hr))return false;if(value){old.value=from(value);unregister(value);if(!old.value)return false;}}
        n.saved.emplace(prop,std::move(old));return true;
    }
    bool set(Node&n,InstanceHandle h,const std::wstring&prop,IInspectable*value,bool required=true){
        unsigned index=0;if(!save(n,h,prop,index)){if(required)report(E_INVALIDARG,glass::feature(n.surface));return false;}
        InstanceHandle v=0;if(value&&FAILED(diag->RegisterInstance(value,&v)))return false;
        HRESULT hr=service->SetProperty(h,v,index);trace("set property",hr);if(v)unregister(v);if(FAILED(hr)){lastError=hr;if(required)report(hr,glass::feature(n.surface));}else{++writes;if(required&&!n.composition&&!(n.surface==glass::TaskbarFill&&thread().taskbarPanel)&&glass::feature(n.surface)&&n.surface!=glass::StartOverlay&&n.surface!=glass::TaskbarOverlay&&n.surface!=glass::StartClear&&n.surface!=glass::NotificationClear&&n.surface!=glass::ExplorerClear)report(n.fallback?S_FALSE:S_OK,glass::feature(n.surface));}return SUCCEEDED(hr);
    }
    void restore(Node&n,InstanceHandle h){for(auto i=n.saved.begin();i!=n.saved.end();){auto&v=i->second;HRESULT hr=S_OK;InstanceHandle old=0;
            if(v.local){if(v.value)hr=diag->RegisterInstance(v.value.p,&old);if(SUCCEEDED(hr))hr=service->SetProperty(h,old,v.index);if(old)unregister(old);}else hr=service->ClearProperty(h,v.index);
            trace("restore property",hr);if(SUCCEEDED(hr))i=n.saved.erase(i);else{lastError=hr;report(hr,glass::feature(n.surface));++i;}}if(n.saved.empty()){for(auto&w:n.watchers)w.second();n.watchers.clear();n.appliedSignature=0;}
    }
    Ptr<IInspectable> markup(const std::wstring&xml){
        Ptr<XamlReader> reader;auto cls=modern?L"Microsoft.UI.Xaml.Markup.XamlReader":L"Windows.UI.Xaml.Markup.XamlReader";HSTRING name{};WindowsCreateString(cls,(UINT32)wcslen(cls),&name);
        HRESULT hr=RoGetActivationFactory(name,ReaderIds[modern?1:0],(void**)reader.put());
        if(FAILED(hr)&&modern){auto module=GetModuleHandleW(L"Microsoft.UI.Xaml.dll");auto fn=module?(HRESULT(WINAPI*)(HSTRING,IActivationFactory**))GetProcAddress(module,"DllGetActivationFactory"):nullptr;Ptr<IActivationFactory> factory;if(fn&&SUCCEEDED(fn(name,factory.put())))hr=factory->QueryInterface(ReaderIds[1],(void**)reader.put());}
        WindowsDeleteString(name);if(FAILED(hr)||!reader)return {};HSTRING text{};WindowsCreateString(xml.c_str(),(UINT32)xml.size(),&text);Ptr<IInspectable> result;reader->Load(text,result.put());WindowsDeleteString(text);return result;
    }
    void typed(Node&n,InstanceHandle h,const wchar_t*property,const wchar_t*type,const std::wstring&value){
        auto prefix=modern?L"Microsoft.UI.Xaml.":L"Windows.UI.Xaml.";std::wstring full=wcscmp(type,L"Double")==0?L"Double":std::wstring(prefix)+type;
        BSTR t=SysAllocString(full.c_str()),v=SysAllocString(value.c_str());InstanceHandle instance=0;
        if(SUCCEEDED(service->CreateInstance(t,v,&instance))){auto p=from(instance);if(p)set(n,h,property,p.p,false);unregister(instance);}SysFreeString(t);SysFreeString(v);
    }
    void edge(Node&n,InstanceHandle h,const glass::Config&c){
        bool rectangle=n.surface==glass::TaskbarFill;
        auto rim=markup(L"<LinearGradientBrush xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\" StartPoint=\"0,0\" EndPoint=\"0,1\"><GradientStop Color=\"#80FFFFFF\" Offset=\"0\"/><GradientStop Color=\"#18303030\" Offset=\"0.5\"/><GradientStop Color=\"#60FFFFFF\" Offset=\"1\"/></LinearGradientBrush>");
        if(rim)set(n,h,rectangle?L"Stroke":L"BorderBrush",rim.p,false);
        typed(n,h,rectangle?L"StrokeThickness":L"BorderThickness",rectangle?L"Double":L"Thickness",L"1");
        if(rectangle){typed(n,h,L"RadiusX",L"Double",std::to_wstring(c.radius));typed(n,h,L"RadiusY",L"Double",std::to_wstring(c.radius));}
    }
    std::vector<const wchar_t*> importedStyles(IInspectable*p){
        std::vector<imported_glass::Element> ancestry;winrt::Windows::Foundation::IInspectable object{nullptr};winrt::copy_from_abi(object,p);
        try{for(unsigned i=0;object&&i<24;++i){std::wstring name;winrt::Windows::Foundation::IInspectable parent{nullptr};
            if(modern){auto e=object.try_as<winrt::Microsoft::UI::Xaml::FrameworkElement>();if(e)name=e.Name();auto d=object.try_as<winrt::Microsoft::UI::Xaml::DependencyObject>();if(d)parent=winrt::Microsoft::UI::Xaml::Media::VisualTreeHelper::GetParent(d);}
            else{auto e=object.try_as<winrt::Windows::UI::Xaml::FrameworkElement>();if(e)name=e.Name();auto d=object.try_as<winrt::Windows::UI::Xaml::DependencyObject>();if(d)parent=winrt::Windows::UI::Xaml::Media::VisualTreeHelper::GetParent(d);}
            ancestry.push_back({winrt::get_class_name(object).c_str(),name});object=parent;}
        }catch(...){trace("theme ancestry",winrt::to_hresult());}
        return imported_glass::styles(imported_glass::theme(host),ancestry);
    }
    bool applyImported(Node&n,InstanceHandle h,const glass::Config&c){bool applied=false;
        for(auto rule:n.imported){std::wstring property,value;if(!imported_glass::assignment(rule,property,value))continue;value=imported_glass::resolve(imported_glass::theme(host),value);
            if(property==L"Background"||property==L"Fill"||property==L"BorderBrush"||property==L"Stroke"){
                bool background=property==L"Background"||property==L"Fill";Ptr<IInspectable> result;
                if(value.find(L"<WindhawkBlur")==0){bool custom=false,fallback=false;result=brush(c,false,h,background?glass::feature(n.surface):0,custom,fallback);if(background){n.composition=custom;n.fallback=fallback;}}
                else if(value==L"Transparent"||value.empty()){bool custom=false,fallback=false;result=brush(c,true,h,glass::feature(n.surface),custom,fallback);}
                else if(!value.empty()&&value.front()==L'<'){auto at=value.find(L' ');if(at!=value.npos)value.insert(at,L" xmlns=\"http://schemas.microsoft.com/winfx/2006/xaml/presentation\"");result=markup(value);}
                if(result){bool ok=set(n,h,property,result.p,background);applied=ok||applied;}
            }else if(property==L"CornerRadius")typed(n,h,property.c_str(),L"CornerRadius",std::to_wstring(c.radius));
            else if(property==L"BorderThickness")typed(n,h,property.c_str(),L"Thickness",value);
            else if(property==L"StrokeThickness")typed(n,h,property.c_str(),L"Double",value);
        }return applied;
    }
    void apply(Node&n,InstanceHandle h,const glass::Config&c){
        if(n.surface==glass::DockRoot||n.surface==glass::DockIcon){auto&t=thread();if(!(c.features&glass::Dock)||c.economy){t.dock.stop();return;}auto p=from(h);if(p){if(n.surface==glass::DockRoot){if(!t.dock.attach(p.p))report(E_NOINTERFACE,glass::Dock);}else t.dock.add(p.p);if(t.dock.ready())report(S_OK,glass::Dock);}return;}
        if(!glass::active(n.surface,c.features)){restore(n,h);return;}DWORD signature=1|(c.opacity<<1)|(c.radius<<8)|(c.tint<<13)|(c.economy?1u<<15:0)|(c.blur<<16);if(n.appliedSignature==signature)return;
        bool applied=false;if(!n.imported.empty())applied=applyImported(n,h,c);else{auto b=brush(c,(n.surface==glass::TaskbarFill&&thread().taskbarPanel)||n.surface==glass::OsdPlate||n.surface==glass::StartOverlay||n.surface==glass::TaskbarOverlay||n.surface==glass::StartClear||n.surface==glass::NotificationClear||n.surface==glass::ExplorerClear,h,glass::feature(n.surface),n.composition,n.fallback);if(!b)return;
        applied=set(n,h,(n.surface==glass::TaskbarFill||n.surface==glass::TaskbarOverlay)?L"Fill":L"Background",b.p);
        if(applied&&(n.surface==glass::TaskbarRoot||n.surface==glass::TaskbarFill||n.surface==glass::StartPlate||n.surface==glass::NotificationPlate||n.surface==glass::ExplorerPlate))edge(n,h,c);
        // Round only native material plates; never change layout constraints.
        if(n.surface==glass::StartPlate||n.surface==glass::NotificationPlate||n.surface==glass::TaskbarRoot||n.surface==glass::VolumePlate||n.surface==glass::BrightnessPlate){
            InstanceHandle r=0;BSTR type=SysAllocString(modern?L"Microsoft.UI.Xaml.CornerRadius":L"Windows.UI.Xaml.CornerRadius");auto text=std::to_wstring(c.radius);BSTR value=SysAllocString(text.c_str());
            if(SUCCEEDED(service->CreateInstance(type,value,&r))){auto p=from(r);if(p)set(n,h,L"CornerRadius",p.p,false);unregister(r);}SysFreeString(type);SysFreeString(value);
        }
        }
        if(applied){n.appliedSignature=signature;for(auto property:{L"Background",L"Fill",L"BorderBrush",L"Stroke"})if(n.saved.count(property)&&!n.watchers.count(property))try{auto p=from(h);auto&t=thread();auto callback=liquid::watch(p.p,modern,property,[&t,h]{if(t.updating)return;auto node=t.nodes.find(h);if(node!=t.nodes.end()){node->second.appliedSignature=0;PostMessageW(t.window,WM_APP+4,0,0);}});if(callback)n.watchers.emplace(property,std::move(callback));}catch(...){trace("watch property",winrt::to_hresult());}}
    }
    void update(Thread&t){if(t.updating)return;glass::Config c;Read state=snapshot(c);
        // A torn read keeps the current surfaces (no flicker while a slider is dragged). Retries are bounded; a controller that stays unreadable is treated as gone and effects are restored.
        if(state==Read::Transient){if(t.retries<4){++t.retries;SetTimer(t.window,11,25,nullptr);return;}state=Read::Invalid;}
        t.retries=0;if(state!=Read::Ok)c.features=0;
        t.updating=true;if(!(c.features&glass::Dock)||c.economy)t.dock.stop();
        for(auto i=t.nodes.begin();i!=t.nodes.end();){Ptr<IInspectable> p;if(i->second.weak) i->second.weak->Resolve(__uuidof(IInspectable),p.put());if(!p){for(auto&w:i->second.watchers)w.second();i=t.nodes.erase(i);continue;}
            InstanceHandle h=0;if(SUCCEEDED(diag->RegisterInstance(p.p,&h))){apply(i->second,h,c);releaseLater(t,h);}
            // Keep only weak descriptors for inactive surfaces. Another switch
            // can be enabled while this diagnostics subscription is already on;
            // Windows does not enumerate the existing tree a second time then.
            ++i;}
        t.updating=false;ensureSubscription();
    }
public:
    void ensureSubscription(){
        if(!service||wantsSubscription()==subscribed.load()||subscriptionWorker.exchange(true))return;
        AddRef();auto worker=CreateThread(nullptr,0,[](void*ctx)->DWORD{auto s=(Session*)ctx;CoInitializeEx(nullptr,COINIT_MULTITHREADED);
            for(int attempts=0;attempts<4;++attempts){bool wanted=s->wantsSubscription();if(wanted==s->subscribed.load())break;
                HRESULT hr=S_OK;if(wanted){xaml_subscription::Scope scope(s->service.p);hr=scope.status();if(SUCCEEDED(hr))hr=s->service->AdviseVisualTreeChange(s);}else hr=s->service->UnadviseVisualTreeChange(s);trace("subscription result",hr);s->lastError=hr;if(FAILED(hr)){s->notify(hr);break;}s->subscribed=wanted;}
            s->subscriptionWorker=false;s->Release();CoUninitialize();return 0;
        },this,0,nullptr);if(worker)CloseHandle(worker);else{subscriptionWorker=false;Release();}
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID id,void**out)override{if(!out)return E_POINTER;*out=nullptr;if(id==IID_IUnknown||id==IID_IObjectWithSite)*out=static_cast<IObjectWithSite*>(this);else if(id==__uuidof(IVisualTreeServiceCallback)||id==__uuidof(IVisualTreeServiceCallback2))*out=static_cast<IVisualTreeServiceCallback2*>(this);else return E_NOINTERFACE;AddRef();return S_OK;}
    ULONG STDMETHODCALLTYPE AddRef()override{return ++refs;}ULONG STDMETHODCALLTYPE Release()override{ULONG n=--refs;if(!n)delete this;return n;}
    HRESULT STDMETHODCALLTYPE GetSite(REFIID id,void**out)override{return site?site->QueryInterface(id,out):E_FAIL;}
    HRESULT STDMETHODCALLTYPE SetSite(IUnknown*p)override{
        if(!p)return S_OK;site=Ptr<IUnknown>(p);p->AddRef();HRESULT hr=p->QueryInterface(__uuidof(IXamlDiagnostics),reinterpret_cast<void**>(diag.put()));if(FAILED(hr)){trace("IXamlDiagnostics",hr);return hr;}
        hr=diag->QueryInterface(__uuidof(IVisualTreeService3),reinterpret_cast<void**>(service.put()));trace("visual tree service 3",hr);if(FAILED(hr))return hr;
        hr=diag->QueryInterface(ReleaseId,reinterpret_cast<void**>(cache.put()));trace("cache release (optional)",hr);
        BSTR init=nullptr;if(SUCCEEDED(diag->GetInitializationData(&init))&&init){std::wstring v=init;modern=v.find(L"modern")!=std::wstring::npos;DWORD owner=wcstoul(v.c_str(),nullptr,10);SysFreeString(init);if(!connect(owner))return E_FAIL;}
        if(!shared)return E_FAIL;wchar_t exe[MAX_PATH]{};GetModuleFileNameW(nullptr,exe,MAX_PATH);std::wstring path=exe;
        if(path.find(L"StartMenuExperienceHost.exe")!=std::wstring::npos)host=1;else if(path.find(L"ShellExperienceHost.exe")!=std::wstring::npos||path.find(L"ShellHost.exe")!=std::wstring::npos)host=2;else host=modern?3:0;
        // Keep one reusable session per framework, rather than creating a new
        // diagnostics tree on every On/Off cycle. Off unsubscribes callbacks.
        AddRef();AcquireSRWLockExclusive(&sessionsLock);sessions.push_back(this);ReleaseSRWLockExclusive(&sessionsLock);ensureSubscription();return S_OK;
    }
    HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation relation,VisualElement element,VisualMutationType mutation)override{
        try{auto&t=thread();if(!cache&&++compatibilityReports>=8192)ensureSubscription();if(mutation==Add){auto surface=glass::identify(element.Name?element.Name:L"",element.Type?element.Type:L"",host);
                if(host==0&&surface==glass::NoSurface)surface=glass::identify(element.Name?element.Name:L"",element.Type?element.Type:L"",3);
                if(host==0&&surface==glass::NoSurface&&element.Name&&wcscmp(element.Name,L"RootGrid")==0&&relation.Parent){auto parent=from(relation.Parent);HSTRING type{};if(parent&&SUCCEEDED(parent->GetRuntimeClassName(&type))){auto name=WindowsGetStringRawBuffer(type,nullptr);if(name&&wcscmp(name,L"Taskbar.TaskbarFrame")==0){surface=glass::TaskbarRoot;t.taskbarPanel=true;}WindowsDeleteString(type);}}

                auto target=from(element.Handle);auto imported=target&&imported_glass::candidate(imported_glass::theme(host),{element.Type?element.Type:L"",element.Name?element.Name:L""})?importedStyles(target.p):std::vector<const wchar_t*>{};
                if(!imported.empty()&&surface==glass::NoSurface)surface=host==1?glass::StartPlate:host==2?glass::NotificationPlate:host==3?glass::ExplorerPlate:glass::TaskbarRoot;
                if(surface!=glass::NoSurface){auto p=std::move(target);Ptr<IWeakReferenceSource> source;if(p&&SUCCEEDED(p->QueryInterface(__uuidof(IWeakReferenceSource),reinterpret_cast<void**>(source.put())))){
                    Node n;n.imported=std::move(imported);n.surface=surface;source->GetWeakReference(n.weak.put());auto old=t.nodes.find(element.Handle);if(old!=t.nodes.end()){t.updating=true;restore(old->second,element.Handle);t.updating=false;if(!old->second.saved.empty()){releaseLater(t,element.Handle);return S_OK;}}t.nodes[element.Handle]=std::move(n);
                    glass::Config c;if(snapshot(c)==Read::Ok){t.updating=true;apply(t.nodes[element.Handle],element.Handle,c);t.updating=false;}
                }}releaseLater(t,element.Handle);releaseLater(t,relation.Parent);
            }else if(mutation==Remove){auto i=t.nodes.find(element.Handle);if(i!=t.nodes.end()){t.updating=true;restore(i->second,element.Handle);t.updating=false;for(auto&w:i->second.watchers)w.second();t.nodes.erase(i);}releaseLater(t,element.Handle);}
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
        for(int i=1;i<=128;++i){auto endpoint=std::wstring(modern?L"WinUIVisualDiagConnection":L"VisualDiagConnection")+std::to_wstring(i);result=fn(endpoint.c_str(),GetCurrentProcessId(),L"",path,glass::runtimeTapId(runtimePath()),init.c_str());if(result!=HRESULT_FROM_WIN32(ERROR_NOT_FOUND))break;}
    }trace(modern?"initialize modern":"initialize system",result);if(FAILED(result)){injecting[modern]=false;report(result);}CoUninitialize();return 0;
}
}
extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID id,REFIID iid,void**p){if(id!=glass::TapId&&id!=glass::runtimeTapId(runtimePath()))return CLASS_E_CLASSNOTAVAILABLE;auto f=new Factory;auto hr=f->QueryInterface(iid,p);f->Release();return hr;}
extern "C" HRESULT WINAPI DllCanUnloadNow(){return S_FALSE;}
extern "C" __declspec(dllexport) HRESULT WINAPI GlassGraphicsTest(){
    if(!imported_glass::test())return E_FAIL;
    auto subscriptionTest=xaml_subscription::test();if(FAILED(subscriptionTest))return subscriptionTest;
    struct QueueOptions{DWORD size;int thread,apartment;};
    HMODULE messaging=LoadLibraryExW(L"CoreMessaging.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!messaging)return HRESULT_FROM_WIN32(GetLastError());
    auto create=(HRESULT(WINAPI*)(QueueOptions,void**))GetProcAddress(messaging,"CreateDispatcherQueueController");Ptr<IUnknown> queue;
    HRESULT hr=create?create({sizeof(QueueOptions),2,2},(void**)queue.put()):E_NOINTERFACE;if(FAILED(hr))return hr;
    try{auto gaussian=winrt::make_self<liquid::Effect>();GUID id{};UINT count=0;
        if(FAILED(gaussian->GetEffectId(&id))||id!=CLSID_D2D1GaussianBlur||FAILED(gaussian->GetPropertyCount(&count))||count!=3)return E_FAIL;
        for(UINT i=0;i<3;++i){Ptr<IUnknown> value;if(FAILED(gaussian->GetProperty(i,value.put()))||!value)return E_FAIL;}
        auto compositor=winrt::Windows::UI::Composition::Compositor();glass::Config config;
        auto brush=liquid::makeGlass(compositor,config);if(!brush)return E_FAIL;
        config.blur=0;config.opacity=5;auto clear=liquid::makeGlass(compositor,config);if(!clear)return E_FAIL;
        config.blur=40;config.opacity=90;config.tint=2;auto dark=liquid::makeGlass(compositor,config);return dark?S_OK:E_FAIL;
    }catch(...){return winrt::to_hresult();}
}
void bootstrap(DWORD owner){if(connect(owner)){
        AcquireSRWLockShared(&sessionsLock);for(auto s:sessions)s->ensureSubscription();ReleaseSRWLockShared(&sessionsLock);
        for(int modern=0;modern<2;++modern)if(GetModuleHandleW(modern?L"Microsoft.Internal.FrameworkUdk.dll":L"Windows.UI.Xaml.dll")&&!injecting[modern].exchange(true)){HANDLE t=CreateThread(nullptr,0,attachFramework,(void*)(INT_PTR)modern,0,nullptr);if(t)CloseHandle(t);else injecting[modern]=false;}
        refreshWindows();
    auto controller=FindWindowW(L"LiquidGlass.Control",nullptr);if(controller)PostMessageW(controller,RegisterWindowMessageW(glass::AttachAckMessage),GetCurrentThreadId(),0);
}}
extern "C" __declspec(dllexport) LRESULT CALLBACK GlassHook(int code,WPARAM w,LPARAM l){
    if(code>=0){auto m=(CWPSTRUCT*)l;if(m->message==RegisterWindowMessageW(glass::AttachMessage))bootstrap((DWORD)m->wParam);}return CallNextHookEx(nullptr,code,w,l);
}
extern "C" __declspec(dllexport) LRESULT CALLBACK GlassMessageHook(int code,WPARAM w,LPARAM l){
    if(code>=0&&w==PM_REMOVE){auto m=(MSG*)l;if(m->message==RegisterWindowMessageW(glass::AttachMessage)){bootstrap((DWORD)m->wParam);m->message=WM_NULL;}}return CallNextHookEx(nullptr,code,w,l);
}
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID){if(reason==DLL_PROCESS_ATTACH){selfModule=instance;DisableThreadLibraryCalls(instance);}return TRUE;}
