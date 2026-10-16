#define _WIN32_WINNT 0x0A00
#define WINVER 0x0A00
#ifndef UNICODE
#define UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <objbase.h>
#include <shellapi.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <commctrl.h>
#include <uxtheme.h>
#include <cmath>
#include "explorer_background.h"
#include "native_osd.h"
#include "native_switches.h"

namespace {
constexpr wchar_t ClassName[]=L"LiquidGlass.Control";
enum {Language=101,Refresh,Apply,Restore,Pause,Scope,Material,Corners,List=113,Status,Note,
    Header=130,Pages,Compatibility,OverlayNote=145,SettingsNote=152,ExitApp,
    NativeApply=160,NativeRestore,NativeInfo,MotionEnabled,VolumeHeading=166,BrightnessHeading,
    VolumeOn=180,VolumeOff,BrightnessOn,BrightnessOff,SettingsPages=190,LanguageLabel,StartupAnimation,MotionText,CompatText,AboutText};
HWND mainWindow{}, language{}, scope{}, material{}, corners{}, list{}, status{}, note{};
HFONT font{}; HBRUSH background{}; NOTIFYICONDATAW tray{};
bool english=true,enabled=false,busy=false; UINT dpi=96;
bool motionEnabled=true;HWINEVENTHOOK windowEvents{};unsigned attributeWrites=0;
bool volumeOn=false,brightnessOn=false,startupAnimation=true,startupPlayed=false;int requestedMask=-1;ULONGLONG startupTime=0;RECT startupRect{};
HWND settingsTabs{};int settingsPage=0,buildingSettings=-1;std::vector<HWND> settingsControls[4];std::map<std::wstring,HICON> appIcons;
std::wstring iniPath;
HWND tabs{},compatibility{},nativeInfo{};
HANDLE nativeJob{};bool nativeRestoring=false;
std::vector<HWND> pageControls[3];int buildingPage=-1,currentPage=0;
HANDLE explorerJob{};bool explorerRestoring=false,resumeRules=false,exitAfterRestore=false;
void configureExplorer(bool restore);
void restoreAll(bool exit=false);
void updateCompatibility();
void configureNative(bool restore);
void updateNativeInfo();
void selectSettingsPage(int page);
struct Entry {HWND hwnd; DWORD pid; std::wstring title,exe,cls;HICON icon{};};
struct Snapshot {DWORD pid;std::wstring cls;int backdrop{},corner{};bool hasBackdrop=false,hasCorner=false;int appliedBackdrop=-1,appliedCorner=-1;};
std::vector<Entry> entries;std::map<HWND,Snapshot> originals;
std::wstring selectedExe;int mode=0,backdrop=3,corner=2;
const wchar_t* tr(const wchar_t* zh,const wchar_t* en){return english?en:zh;}
int selection(HWND h){return (int)SendMessageW(h,CB_GETCURSEL,0,0);}
void message(const std::wstring& s){SetWindowTextW(status,s.c_str());}
std::wstring classOf(HWND h){wchar_t c[256]{};GetClassNameW(h,c,256);return c;}
std::wstring executable(DWORD pid){HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);wchar_t path[32768]{};DWORD n=32768;if(p){if(!QueryFullProcessImageNameW(p,0,path,&n))path[0]=0;CloseHandle(p);}std::wstring s=path;auto i=s.find_last_of(L"\\/");return i==std::wstring::npos?s:s.substr(i+1);}
bool same(const std::wstring&a,const std::wstring&b){return _wcsicmp(a.c_str(),b.c_str())==0;}
bool usesExplorer(int selectedMode,const std::wstring&exe){return selectedMode==1||selectedMode==2||(selectedMode==0&&same(exe,L"explorer.exe"));}
bool explorerBackupExists(){auto dir=iniPath.substr(0,iniPath.find_last_of(L"\\")+1);return GetFileAttributesW((dir+L"explorer-background-originals.bin").c_str())!=INVALID_FILE_ATTRIBUTES;}
bool nativeBackupExists(){auto dir=iniPath.substr(0,iniPath.find_last_of(L"\\")+1);return GetFileAttributesW((dir+L"native-osd-originals.bin").c_str())!=INVALID_FILE_ATTRIBUTES||GetFileAttributesW((dir+L"native-osd-switches.bin").c_str())!=INVALID_FILE_ATTRIBUTES;}
void updateNativeInfo(){if(nativeInfo)SetWindowTextW(nativeInfo,nativeJob?tr(L"正在处理原生提示条……",L"Updating native indicators..."):tr(L"On：玻璃背景；Off：普通不透明背景。全部关闭时恢复原配置。",L"On: glass. Off: opaque background. Both Off restores the original settings."));for(int id:{VolumeOn,VolumeOff,BrightnessOn,BrightnessOff})if(GetDlgItem(mainWindow,id))InvalidateRect(GetDlgItem(mainWindow,id),nullptr,FALSE);}
bool shellClass(const std::wstring& c){return c==L"Shell_TrayWnd"||c==L"Shell_SecondaryTrayWnd"||c==L"Progman"||c==L"WorkerW"||c==L"XamlExplorerHostIslandWindow";}
bool valid(HWND h,const Snapshot& s){DWORD pid=0;GetWindowThreadProcessId(h,&pid);return IsWindow(h)&&pid==s.pid&&classOf(h)==s.cls&&GetPropW(h,L"LiquidGlass.Control.Owner")==mainWindow;}
void saveLanguage(){WritePrivateProfileStringW(L"UI",L"Language",english?L"en":L"zh",iniPath.c_str());}
bool readEnglish(const std::wstring& path){wchar_t lang[8]{};GetPrivateProfileStringW(L"UI",L"Language",L"en",lang,8,path.c_str());return _wcsicmp(lang,L"zh")!=0;}
void setRulesActive(bool active){enabled=active;if(active)SetTimer(mainWindow,1,15000,nullptr);else{KillTimer(mainWindow,1);KillTimer(mainWindow,3);}}
void CALLBACK windowChanged(HWINEVENTHOOK,DWORD,HWND h,LONG object,LONG child,DWORD,DWORD){
    if(!enabled||!h||object!=OBJID_WINDOW||child!=CHILDID_SELF||GetAncestor(h,GA_ROOT)!=h)return;
    DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid!=GetCurrentProcessId())SetTimer(mainWindow,3,180,nullptr);
}
void selectPage(int page){
    if(page<0||page>2)return;currentPage=page;
    SendMessageW(mainWindow,WM_SETREDRAW,FALSE,0);
    for(int p=0;p<3;++p)for(HWND h:pageControls[p])ShowWindow(h,p==page?SW_SHOWNA:SW_HIDE);
    if(page==2)selectSettingsPage(settingsPage);
    TabCtrl_SetCurSel(tabs,page);SendMessageW(mainWindow,WM_SETREDRAW,TRUE,0);RedrawWindow(mainWindow,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
}
void selectSettingsPage(int page){if(page<0||page>3)return;settingsPage=page;for(int p=0;p<4;++p)for(HWND h:settingsControls[p])ShowWindow(h,currentPage==2&&p==page?SW_SHOWNA:SW_HIDE);TabCtrl_SetCurSel(settingsTabs,page);RedrawWindow(settingsTabs,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE);InvalidateRect(mainWindow,nullptr,FALSE);}
void restoreWindows(){
    int failures=0;
    for(auto i=originals.begin();i!=originals.end();){auto h=i->first;auto&s=i->second;
        if(!valid(h,s)){i=originals.erase(i);continue;}
        bool ok=true;if(s.hasBackdrop)ok=SUCCEEDED(DwmSetWindowAttribute(h,38,&s.backdrop,sizeof(int)))&&ok;
        if(s.hasCorner)ok=SUCCEEDED(DwmSetWindowAttribute(h,33,&s.corner,sizeof(int)))&&ok;
        if(ok){RemovePropW(h,L"LiquidGlass.Control.Owner");i=originals.erase(i);}else{++failures;++i;}
    }
    message(failures?tr(L"部分窗口恢复失败，可再次尝试恢复。",L"Some windows could not be restored. Try Restore again."):tr(L"已恢复受管理窗口的原始属性。",L"Original attributes restored for managed windows."));
}
bool applyWindow(HWND h){
    // Enforce the app-only boundary even when a caller bypasses enumeration.
    DWORD targetPid=0;GetWindowThreadProcessId(h,&targetPid);
    auto targetClass=classOf(h);
    if(shellClass(targetClass))return false;
    auto targetExe=executable(targetPid);
    if(same(targetExe,L"StartMenuExperienceHost.exe")||same(targetExe,L"SearchHost.exe")||same(targetExe,L"SearchApp.exe")||same(targetExe,L"ShellExperienceHost.exe"))return false;
    auto it=originals.find(h);
    if(it!=originals.end()&&!valid(h,it->second)){originals.erase(it);it=originals.end();}
    if(it==originals.end()){
        Snapshot s;GetWindowThreadProcessId(h,&s.pid);s.cls=classOf(h);
        s.hasBackdrop=SUCCEEDED(DwmGetWindowAttribute(h,38,&s.backdrop,sizeof(int)));
        s.hasCorner=SUCCEEDED(DwmGetWindowAttribute(h,33,&s.corner,sizeof(int)));
        if(!s.hasBackdrop&&!s.hasCorner)return false;
        if(!SetPropW(h,L"LiquidGlass.Control.Owner",mainWindow))return false;
        it=originals.emplace(h,s).first;
    }
    bool requested=false,ok=true;
    if(it->second.hasBackdrop){requested=true;if(it->second.appliedBackdrop!=backdrop){++attributeWrites;bool changed=SUCCEEDED(DwmSetWindowAttribute(h,38,&backdrop,sizeof(int)));if(changed)it->second.appliedBackdrop=backdrop;ok=changed&&ok;}}
    if(it->second.hasCorner){requested=true;if(it->second.appliedCorner!=corner){++attributeWrites;bool changed=SUCCEEDED(DwmSetWindowAttribute(h,33,&corner,sizeof(int)));if(changed)it->second.appliedCorner=corner;ok=changed&&ok;}}
    return requested&&ok&&it->second.hasBackdrop;
}
BOOL CALLBACK enumerate(HWND h,LPARAM data){
    auto& out=*reinterpret_cast<std::vector<Entry>*>(data);DWORD pid=0;GetWindowThreadProcessId(h,&pid);
    if(pid==GetCurrentProcessId()||!IsWindowVisible(h))return TRUE;
    auto cls=classOf(h);if(shellClass(cls))return TRUE;
    if(GetWindow(h,GW_OWNER)||GetWindowLongPtrW(h,GWL_EXSTYLE)&WS_EX_TOOLWINDOW)return TRUE;
    wchar_t title[512]{};GetWindowTextW(h,title,512);if(!title[0])return TRUE;
    std::wstring exe=executable(pid);if(exe.empty())return TRUE;
    // System surfaces are never application targets.
    if(same(exe,L"StartMenuExperienceHost.exe")||same(exe,L"SearchHost.exe")||same(exe,L"SearchApp.exe")||same(exe,L"ShellExperienceHost.exe"))return TRUE;
    out.push_back({h,pid,title,exe,cls});return TRUE;
}
std::vector<Entry> collect(){std::vector<Entry> v;EnumWindows(enumerate,reinterpret_cast<LPARAM>(&v));return v;}
HICON applicationIcon(const Entry&e){
    wchar_t file[32768]{};DWORD length=32768;HANDLE process=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,e.pid);if(process){QueryFullProcessImageNameW(process,0,file,&length);CloseHandle(process);}std::wstring key=file[0]?file:e.exe;
    auto found=appIcons.find(key);if(found!=appIcons.end())return found->second;
    DWORD_PTR supplied=0;HICON icon=nullptr;SendMessageTimeoutW(e.hwnd,WM_GETICON,ICON_SMALL2,0,SMTO_ABORTIFHUNG|SMTO_BLOCK,20,&supplied);if(supplied)icon=CopyIcon((HICON)supplied);
    if(!icon){HICON cls=(HICON)GetClassLongPtrW(e.hwnd,GCLP_HICONSM);if(cls)icon=CopyIcon(cls);}
    if(!icon&&file[0]){HICON extractedIcon=nullptr;ExtractIconExW(file,0,nullptr,&extractedIcon,1);icon=extractedIcon;}
    if(!icon)icon=CopyIcon(LoadIconW(nullptr,IDI_APPLICATION));appIcons[key]=icon;return icon;
}
void refresh(){
    HWND previous=nullptr;int n=(int)SendMessageW(list,LB_GETCURSEL,0,0);if(n>=0&&n<(int)entries.size())previous=entries[n].hwnd;
    SendMessageW(list,WM_SETREDRAW,FALSE,0);entries=collect();SendMessageW(list,LB_RESETCONTENT,0,0);int sel=0;
    for(size_t i=0;i<entries.size();++i){entries[i].icon=applicationIcon(entries[i]);bool explorer=entries[i].cls==L"CabinetWClass"||entries[i].cls==L"ExploreWClass";std::wstring text=entries[i].exe+(explorer?tr(L" [整窗后端]  —  ",L" [whole-window backend]  —  "):tr(L" [仅外框/待验证]  —  ",L" [frame only/unverified]  —  "))+entries[i].title;SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));if(entries[i].hwnd==previous)sel=(int)i;}
    if(!entries.empty())SendMessageW(list,LB_SETCURSEL,sel,0);
    SendMessageW(list,WM_SETREDRAW,TRUE,0);InvalidateRect(list,nullptr,FALSE);
    updateCompatibility();
}
void updateCompatibility(){
    if(!compatibility)return;
    int n=(int)SendMessageW(list,LB_GETCURSEL,0,0);bool explorer=selection(scope)==1||(n>=0&&n<(int)entries.size()&&(entries[n].cls==L"CabinetWClass"||entries[n].cls==L"ExploreWClass"));
    SetWindowTextW(compatibility,explorer?tr(L"资源管理器：有整窗背景后端，需要已安装 File Explorer Styler。",L"Explorer: whole-window backend available; installed File Explorer Styler required."):tr(L"其他应用：仅尝试外框材质；Chrome / Electron 等内容区通常不受影响。",L"Other apps: frame material only; Chrome / Electron content usually stays opaque."));
}
void sync(bool report){
    if(!enabled||busy)return;busy=true;
    for(auto it=originals.begin();it!=originals.end();)if(!valid(it->first,it->second))it=originals.erase(it);else ++it;
    int ok=0,total=0;
    for(const auto&e:collect()){
        bool explorer=e.cls==L"CabinetWClass"||e.cls==L"ExploreWClass";
        if((mode==0&&same(e.exe,selectedExe))||(mode==1&&explorer)||(mode==2)){
            ++total;if(applyWindow(e.hwnd))++ok;
        }
    }
    if(report){std::wstring s=tr(L"应用窗口属性请求成功：",L"App window attribute requests accepted: ");s+=std::to_wstring(ok)+L" / "+std::to_wstring(total);s+=tr(L"。若无可见变化，该应用可能不支持外部背景材质。",L". If unchanged visually, the app may not support external backdrop requests.");message(s);}
    busy=false;
}
void apply(){
    if(explorerJob||nativeJob)return;
    int nextMode=selection(scope);std::wstring exe;
    if(nextMode==0){int n=(int)SendMessageW(list,LB_GETCURSEL,0,0);if(n<0||n>=(int)entries.size()){message(tr(L"请先选择一个应用窗口。",L"Select an application window first."));return;}exe=entries[n].exe;}
    setRulesActive(false);restoreWindows();if(!originals.empty())return;
    mode=nextMode;selectedExe=exe;int idx=selection(material);backdrop=idx==0?3:idx==1?2:idx==2?4:1;
    corner=selection(corners)==0?2:selection(corners)==1?3:1;
    if(usesExplorer(mode,selectedExe)){resumeRules=true;configureExplorer(false);return;}
    setRulesActive(true);sync(true);
}
void restoreAll(bool exit){
    if(explorerJob||nativeJob)return;
    setRulesActive(false);resumeRules=false;restoreWindows();if(!originals.empty())return;
    exitAfterRestore=exit;
    if(explorerBackupExists()){configureExplorer(true);return;}
    if(nativeBackupExists()){configureNative(true);return;}
    message(tr(L"已还原，恢复到应用前的状态。",L"Restored to the state before Apply."));
    if(exitAfterRestore)DestroyWindow(mainWindow);
}
void combo(HWND h,std::initializer_list<const wchar_t*> labels,int sel){SendMessageW(h,CB_RESETCONTENT,0,0);for(auto s:labels)SendMessageW(h,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(s));SendMessageW(h,CB_SETCURSEL,sel<0?0:sel,0);}
#include "ui_style.h"
#include "control_ui.h"
void configureExplorer(bool restore){
    if(explorerJob||nativeJob)return;
    wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
    const wchar_t* commands[]={L"--explorer-acrylic",L"--explorer-mica",L"--explorer-mica-alt",L"--explorer-none"};
    int index=selection(material);if(index<0||index>3)index=0;
    SHELLEXECUTEINFOW info{};info.cbSize=sizeof(info);info.fMask=SEE_MASK_NOCLOSEPROCESS;info.hwnd=mainWindow;info.lpVerb=L"runas";info.lpFile=exe;info.lpParameters=restore?L"--explorer-restore":commands[index];info.nShow=SW_HIDE;
    if(!ShellExecuteExW(&info)){resumeRules=false;exitAfterRestore=false;message(tr(L"授权已取消，操作未完成，可再次点击重试。",L"Approval canceled. Operation incomplete; click again to retry."));return;}
    explorerJob=info.hProcess;explorerRestoring=restore;
    SetTimer(mainWindow,2,200,nullptr);
    for(int id:{Apply,Restore,Scope,Material,Corners,List})EnableWindow(GetDlgItem(mainWindow,id),FALSE);
    message(restore?tr(L"正在还原……",L"Restoring..."):tr(L"正在应用……",L"Applying..."));
}
void pollExplorer(){
    if(!explorerJob||WaitForSingleObject(explorerJob,0)!=WAIT_OBJECT_0)return;
    DWORD code=1;GetExitCodeProcess(explorerJob,&code);CloseHandle(explorerJob);explorerJob=nullptr;
    for(int id:{Apply,Restore,Scope,Material,Corners,List})EnableWindow(GetDlgItem(mainWindow,id),TRUE);
    if(!code){
        if(!explorerRestoring&&resumeRules){setRulesActive(true);sync(false);}
        message(explorerRestoring?tr(L"已还原，恢复到应用前的状态。",L"Restored to the state before Apply."):tr(L"已应用。点击「还原」即可切换回去。",L"Applied. Click Restore to switch back."));
        if(explorerRestoring&&nativeBackupExists()){configureNative(true);return;}
        if(explorerRestoring&&exitAfterRestore){DestroyWindow(mainWindow);return;}
    }
    else if(code==8)message(tr(L"检测到原有主题或自定义样式，已停止，避免覆盖。",L"Existing theme or custom styles detected; stopped to preserve them."));
    else message(std::wstring(tr(L"背景后端失败，错误码：",L"Background backend failed, code: "))+std::to_wstring(code)+tr(L"（3：模块未安装/无权限；7：模块已禁用；6：备份异常）。",L" (3: missing module/access; 7: disabled module; 6: backup error)."));
    resumeRules=false;exitAfterRestore=false;
}
void configureNative(bool restore){
    if(nativeJob||explorerJob)return;
    wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);
    if(restore&&!nativeBackupExists()){updateNativeInfo();return;}
    int mask=restore?0:requestedMask>=0?requestedMask:((volumeOn?1:0)|(brightnessOn?2:0));requestedMask=mask;
    std::wstring command=L"--native-switch-"+std::to_wstring(mask)+(motionEnabled?L"":L"-static");
    SHELLEXECUTEINFOW info{};info.cbSize=sizeof(info);info.fMask=SEE_MASK_NOCLOSEPROCESS;info.hwnd=mainWindow;info.lpVerb=L"runas";info.lpFile=exe;info.lpParameters=command.c_str();info.nShow=SW_HIDE;
    if(!ShellExecuteExW(&info)){requestedMask=-1;exitAfterRestore=false;message(tr(L"授权已取消，原生提示条操作未完成。",L"Approval canceled. Native indicator operation incomplete."));return;}
    nativeJob=info.hProcess;nativeRestoring=restore||mask==0;for(int id:{NativeApply,NativeRestore,Apply,Restore})EnableWindow(GetDlgItem(mainWindow,id),FALSE);updateNativeInfo();
    for(int id:{VolumeOn,VolumeOff,BrightnessOn,BrightnessOff})EnableWindow(GetDlgItem(mainWindow,id),FALSE);
    SetTimer(mainWindow,2,200,nullptr);
}
void pollNative(){
    if(!nativeJob||WaitForSingleObject(nativeJob,0)!=WAIT_OBJECT_0)return;
    DWORD code=1;GetExitCodeProcess(nativeJob,&code);CloseHandle(nativeJob);nativeJob=nullptr;for(int id:{NativeApply,NativeRestore,Apply,Restore,VolumeOn,VolumeOff,BrightnessOn,BrightnessOff})EnableWindow(GetDlgItem(mainWindow,id),TRUE);
    if(!code){volumeOn=(requestedMask&1)!=0;brightnessOn=(requestedMask&2)!=0;WritePrivateProfileStringW(L"Native",L"VolumeOn",volumeOn?L"1":L"0",iniPath.c_str());WritePrivateProfileStringW(L"Native",L"BrightnessOn",brightnessOn?L"1":L"0",iniPath.c_str());}requestedMask=-1;updateNativeInfo();
    if(!code){message(nativeRestoring?tr(L"Windows 原生音量与亮度提示已还原。",L"Native Windows volume and brightness indicators restored."):tr(L"已设置原生提示条背景。按音量键或亮度键查看，不再显示额外浮层。",L"Native indicator background configured. Use volume or brightness keys; no extra overlay is shown."));if(nativeRestoring&&exitAfterRestore){DestroyWindow(mainWindow);return;}}
    else message(std::wstring(tr(L"原生提示条后端失败，错误码：",L"Native indicator backend failed, code: "))+std::to_wstring(code)+tr(L"。3：模块未安装/无权限；7：模块禁用；6：备份异常。",L". 3: module/access unavailable; 7: disabled; 6: backup error."));
    exitAfterRestore=false;
}
int selfTest(){
    bool passed=true;
    // Exercise first launch and saved preferences without touching the user's INI.
    auto userIni=iniPath;bool userEnglish=english;
    iniPath=userIni+L".language-test-"+std::to_wstring(GetCurrentProcessId());
    passed=passed&&readEnglish(iniPath);
    english=false;saveLanguage();passed=passed&&!readEnglish(iniPath);
    english=true;saveLanguage();passed=passed&&readEnglish(iniPath);
    passed=DeleteFileW(iniPath.c_str())&&passed;iniPath=userIni;english=userEnglish;
    // Removed commands must not retain hidden controls or message handlers.
    for(int removedId:{109,110,111,112,116,117,120,121})passed=passed&&!GetDlgItem(mainWindow,removedId);
    passed=passed&&usesExplorer(0,L"explorer.exe")&&usesExplorer(1,L"")&&usesExplorer(2,L"")&&!usesExplorer(0,L"notepad.exe");
    passed=passed&&TabCtrl_GetItemCount(tabs)==3;
    selectPage(1);passed=passed&&((GetWindowLongPtrW(GetDlgItem(mainWindow,VolumeOn),GWL_STYLE)&WS_VISIBLE)!=0)&&((GetWindowLongPtrW(list,GWL_STYLE)&WS_VISIBLE)==0);
    RECT onRect{},offRect{};GetWindowRect(GetDlgItem(mainWindow,VolumeOn),&onRect);GetWindowRect(GetDlgItem(mainWindow,VolumeOff),&offRect);passed=passed&&onRect.left<offRect.left&&onRect.bottom-onRect.top>=MulDiv(80,dpi,96)&&offRect.bottom-offRect.top>=MulDiv(80,dpi,96);
    selectPage(2);passed=passed&&((GetWindowLongPtrW(GetDlgItem(mainWindow,SettingsNote),GWL_STYLE)&WS_VISIBLE)!=0);
    passed=passed&&TabCtrl_GetItemCount(settingsTabs)==4;selectSettingsPage(1);passed=passed&&((GetWindowLongPtrW(language,GWL_STYLE)&WS_VISIBLE)==0);
    selectSettingsPage(2);RECT pending{};passed=passed&&settingsPage==2&&TabCtrl_GetCurSel(settingsTabs)==2&&GetUpdateRect(settingsTabs,&pending,FALSE);
    for(HWND tab:{tabs,settingsTabs}){RECT client{},last{};GetClientRect(tab,&client);TabCtrl_GetItemRect(tab,TabCtrl_GetItemCount(tab)-1,&last);passed=passed&&last.right<=client.right;}
    selectSettingsPage(0);passed=passed&&((GetWindowLongPtrW(language,GWL_STYLE)&WS_VISIBLE)!=0);
    selectPage(0);
    WNDCLASSW shellFixtureClass{};shellFixtureClass.lpfnWndProc=DefWindowProcW;shellFixtureClass.hInstance=GetModuleHandleW(nullptr);shellFixtureClass.lpszClassName=L"Shell_SecondaryTrayWnd";
    bool registered=RegisterClassW(&shellFixtureClass)!=0;
    HWND shellFixture=registered?CreateWindowW(shellFixtureClass.lpszClassName,L"Boundary fixture",WS_POPUP,0,0,20,20,nullptr,nullptr,shellFixtureClass.hInstance,nullptr):nullptr;
    passed=passed&&shellFixture&&!applyWindow(shellFixture)&&originals.empty();
    if(shellFixture)DestroyWindow(shellFixture);
    if(registered)UnregisterClassW(shellFixtureClass.lpszClassName,shellFixtureClass.hInstance);
    // Exercise actual controls without persisting language or changing system preferences.
    english=true;localize();wchar_t text[256]{};
    GetWindowTextW(GetDlgItem(mainWindow,Apply),text,256);passed=passed&&wcscmp(text,L"Apply")==0;
    GetWindowTextW(GetDlgItem(mainWindow,Restore),text,256);passed=passed&&wcscmp(text,L"Restore")==0;
    GetWindowTextW(GetDlgItem(mainWindow,VolumeOn),text,256);passed=passed&&wcscmp(text,L"On")==0;
    passed=passed&&SendMessageW(scope,CB_GETCOUNT,0,0)==3&&SendMessageW(material,CB_GETCOUNT,0,0)==4;
    english=false;localize();GetWindowTextW(GetDlgItem(mainWindow,Apply),text,256);passed=passed&&wcscmp(text,L"应用")==0;
    GetWindowTextW(GetDlgItem(mainWindow,Restore),text,256);passed=passed&&wcscmp(text,L"还原")==0;
    GetWindowTextW(GetDlgItem(mainWindow,VolumeOn),text,256);passed=passed&&wcscmp(text,L"On · 开启")==0;
    for(auto&e:entries)passed=passed&&e.icon!=nullptr;
    auto switches=native_switches::fields(1,2,1);passed=passed&&switches.size()==6&&switches[1].data!=switches[4].data;
    for(int removed:{140,141,142,143,144,150,151,154,155})passed=passed&&!GetDlgItem(mainWindow,removed);
    auto nativeValues=native_osd::desired(0);passed=passed&&nativeValues.size()==4&&wcscmp(native_osd::Target,L"Grid#ConfirmatorMainGrid")==0;
    passed=passed&&font&&headingFont&&actionFont;
    // Check both persisted backup formats without touching registry settings.
    std::wstring testBackup=iniPath.substr(0,iniPath.find_last_of(L"\\")+1)+L"native-backup-test-"+std::to_wstring(GetCurrentProcessId())+L".bin";
    try{
        auto saved=nativeValues;for(auto&v:saved){v.type=0xffffffff;v.data.clear();}
        native_osd::save(testBackup,12,saved);DWORD index=0;auto loaded=native_osd::load(testBackup,index);
        passed=passed&&index==12&&loaded.size()==4;for(auto&v:loaded)passed=passed&&v.type==0xffffffff&&v.data.empty();DeleteFileW(testBackup.c_str());
        HANDLE legacy=CreateFileW(testBackup.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
        if(legacy==INVALID_HANDLE_VALUE)throw std::runtime_error("legacy fixture");DWORD magic=0x4e4f5331,slot=9,missing=0xffffffff,size=0;native_osd::io(legacy,&magic,4,true);native_osd::io(legacy,&slot,4,true);for(int i=0;i<3;++i){native_osd::io(legacy,&missing,4,true);native_osd::io(legacy,&size,4,true);}CloseHandle(legacy);
        loaded=native_osd::load(testBackup,index);passed=passed&&index==9&&loaded.size()==3;DeleteFileW(testBackup.c_str());
    }catch(...){passed=false;DeleteFileW(testBackup.c_str());}
    HWND fixture=CreateWindowExW(0,ClassName,L"Restoration fixture",WS_OVERLAPPEDWINDOW,0,0,300,200,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    int originalBackdrop=0,originalCorner=0,afterBackdrop=0,afterCorner=0;
    bool supported=fixture&&SUCCEEDED(DwmGetWindowAttribute(fixture,38,&originalBackdrop,sizeof(int)))&&SUCCEEDED(DwmGetWindowAttribute(fixture,33,&originalCorner,sizeof(int)));
    if(supported){
        bool accepted=applyWindow(fixture);unsigned writes=attributeWrites;applyWindow(fixture);passed=passed&&attributeWrites==writes;restoreWindows();
        bool restored=SUCCEEDED(DwmGetWindowAttribute(fixture,38,&afterBackdrop,sizeof(int)))&&SUCCEEDED(DwmGetWindowAttribute(fixture,33,&afterCorner,sizeof(int)));
        passed=passed&&accepted&&restored&&originalBackdrop==afterBackdrop&&originalCorner==afterCorner&&originals.empty()&&!GetPropW(fixture,L"LiquidGlass.Control.Owner");
    }
    if(fixture)DestroyWindow(fixture);
    std::wstring report=iniPath.substr(0,iniPath.find_last_of(L"\\")+1)+L"self-test.txt";
    const char* result=passed?(supported?"PASS: bilingual pages/fonts; native-only motion rule; v1/v2 backup compatibility; zero duplicate DWM writes; restore.\r\n":"PASS: bilingual pages/fonts; native-only motion rule; v1/v2 backup compatibility. SKIP: DWM backdrop unavailable.\r\n"):"FAIL: UI, caching, backup compatibility, or restoration check.\r\n";
    HANDLE file=CreateFileW(report.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);DWORD written=0;
    if(file!=INVALID_HANDLE_VALUE){WriteFile(file,result,(DWORD)lstrlenA(result),&written,nullptr);CloseHandle(file);}else passed=false;
    return passed?0:2;
}
void show(){
    BOOL systemMotion=TRUE;SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION,0,&systemMotion,0);
    if(!startupPlayed&&startupAnimation&&systemMotion){startupPlayed=true;GetWindowRect(mainWindow,&startupRect);startupTime=GetTickCount64();SetWindowLongPtrW(mainWindow,GWL_EXSTYLE,GetWindowLongPtrW(mainWindow,GWL_EXSTYLE)|WS_EX_LAYERED);SetLayeredWindowAttributes(mainWindow,0,0,LWA_ALPHA);SetWindowPos(mainWindow,nullptr,startupRect.left,startupRect.top+MulDiv(12,dpi,96),0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);ShowWindow(mainWindow,SW_SHOW);SetTimer(mainWindow,4,16,nullptr);}else{startupPlayed=true;ShowWindow(mainWindow,SW_SHOW);}SetForegroundWindow(mainWindow);
}
void menu(){HMENU m=CreatePopupMenu();AppendMenuW(m,MF_STRING,1,tr(L"打开设置",L"Open settings"));AppendMenuW(m,MF_STRING,2,tr(L"还原",L"Restore"));AppendMenuW(m,MF_STRING,3,tr(L"还原并退出",L"Restore and exit"));POINT p;GetCursorPos(&p);SetForegroundWindow(mainWindow);int n=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,mainWindow,nullptr);DestroyMenu(m);if(n==1)show();if(n==2)restoreAll();if(n==3)restoreAll(true);PostMessageW(mainWindow,WM_NULL,0,0);}
LRESULT CALLBACK proc(HWND h,UINT msg,WPARAM w,LPARAM l){
    static UINT restart=RegisterWindowMessageW(L"TaskbarCreated");if(msg==restart){Shell_NotifyIconW(NIM_ADD,&tray);Shell_NotifyIconW(NIM_SETVERSION,&tray);return 0;}
    switch(msg){
    case WM_PAINT:paintChrome(h);return 0;
    case WM_ERASEBKGND:return 1;
    case WM_DRAWITEM:if(drawModernControl(reinterpret_cast<DRAWITEMSTRUCT*>(l)))return TRUE;break;
    case WM_MEASUREITEM:{auto item=reinterpret_cast<MEASUREITEMSTRUCT*>(l);if(item->CtlID==List){item->itemHeight=MulDiv(48,dpi,96);return TRUE;}}break;
    case WM_NOTIFY:if(reinterpret_cast<NMHDR*>(l)->code==TCN_SELCHANGE){if(reinterpret_cast<NMHDR*>(l)->idFrom==Pages)selectPage(TabCtrl_GetCurSel(tabs));else if(reinterpret_cast<NMHDR*>(l)->idFrom==SettingsPages)selectSettingsPage(TabCtrl_GetCurSel(settingsTabs));}return 0;
    case WM_COMMAND:switch(LOWORD(w)){
        case Language:if(HIWORD(w)==CBN_SELCHANGE){english=selection(language)==1;saveLanguage();localize();}break;
        case List:if(HIWORD(w)==LBN_SELCHANGE)updateCompatibility();break;
        case Scope:if(HIWORD(w)==CBN_SELCHANGE)updateCompatibility();break;
        case MotionEnabled:motionEnabled=SendMessageW(GetDlgItem(h,MotionEnabled),BM_GETCHECK,0,0)==BST_CHECKED;WritePrivateProfileStringW(L"Native",L"Motion",motionEnabled?L"1":L"0",iniPath.c_str());break;
        case StartupAnimation:startupAnimation=SendMessageW(GetDlgItem(h,StartupAnimation),BM_GETCHECK,0,0)==BST_CHECKED;WritePrivateProfileStringW(L"UI",L"StartupAnimation",startupAnimation?L"1":L"0",iniPath.c_str());break;
        case VolumeOn:case VolumeOff:case BrightnessOn:case BrightnessOff:{int mask=(volumeOn?1:0)|(brightnessOn?2:0);if(LOWORD(w)==VolumeOn)mask|=1;if(LOWORD(w)==VolumeOff)mask&=~1;if(LOWORD(w)==BrightnessOn)mask|=2;if(LOWORD(w)==BrightnessOff)mask&=~2;requestedMask=mask;exitAfterRestore=false;configureNative(false);break;}
        case NativeApply:exitAfterRestore=false;configureNative(false);break;
        case NativeRestore:exitAfterRestore=false;configureNative(true);break;
        case ExitApp:restoreAll(true);break;
        case Refresh:refresh();break;case Apply:apply();break;
        case Restore:restoreAll();break;
        case Pause:ShowWindow(h,SW_HIDE);break;
        }return 0;
    case WM_TIMER:if(w==4){float t=std::min(1.f,(GetTickCount64()-startupTime)/180.f),ease=1-powf(1-t,3);SetLayeredWindowAttributes(h,0,(BYTE)(255*ease),LWA_ALPHA);SetWindowPos(h,nullptr,startupRect.left,startupRect.top+(int)((1-ease)*MulDiv(12,dpi,96)),0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);if(t>=1){KillTimer(h,4);SetWindowLongPtrW(h,GWL_EXSTYLE,GetWindowLongPtrW(h,GWL_EXSTYLE)&~WS_EX_LAYERED);RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);}}else if(w==1)sync(false);else if(w==3){KillTimer(h,3);sync(false);}else if(w==2){pollExplorer();pollNative();if(!explorerJob&&!nativeJob)KillTimer(h,2);}return 0;
    case WM_APP+1:if(LOWORD(l)==WM_CONTEXTMENU)menu();else if(LOWORD(l)==NIN_SELECT||LOWORD(l)==NIN_KEYSELECT)show();return 0;
    case WM_APP+2:show();return 0;
    case WM_APP+3:if(explorerJob||nativeJob)return 0;setRulesActive(false);restoreWindows();if(!originals.empty()){show();return 0;}DestroyWindow(h);return 0;
    case WM_CLOSE:ShowWindow(h,SW_HIDE);return 0;
    case WM_QUERYENDSESSION:setRulesActive(false);restoreWindows();return TRUE;
    case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:{int id=GetDlgCtrlID(reinterpret_cast<HWND>(l));bool common=id==Header||id==Status;SetBkMode(reinterpret_cast<HDC>(w),TRANSPARENT);SetTextColor(reinterpret_cast<HDC>(w),id==Header?RGB(24,42,66):RGB(77,94,117));return reinterpret_cast<LRESULT>(common?background:cardBrush);}
    case WM_DESTROY:KillTimer(h,1);KillTimer(h,2);KillTimer(h,3);KillTimer(h,4);Shell_NotifyIconW(NIM_DELETE,&tray);PostQuitMessage(0);return 0;
    }return DefWindowProcW(h,msg,w,l);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR commandLine,int){
    for(int mask=0;mask<4;++mask)for(int still=0;still<2;++still){std::wstring expected=L"--native-switch-"+std::to_wstring(mask)+(still?L"-static":L"");if(expected==commandLine){wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);std::wstring dir=path;dir=dir.substr(0,dir.find_last_of(L"\\")+1);return native_switches::run(mask,!still,dir);}}
    if(wcscmp(commandLine,L"--native-apply")==0||wcscmp(commandLine,L"--native-static")==0||wcscmp(commandLine,L"--native-restore")==0){wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);std::wstring dir=path;dir=dir.substr(0,dir.find_last_of(L"\\")+1);return native_switches::run(wcscmp(commandLine,L"--native-restore")==0?0:3,wcscmp(commandLine,L"--native-static")!=0,dir);}
    if(wcsncmp(commandLine,L"--explorer-",11)==0){const wchar_t*effect=nullptr;bool restore=false;
        if(wcscmp(commandLine,L"--explorer-acrylic")==0)effect=L"acrylic";else if(wcscmp(commandLine,L"--explorer-mica")==0)effect=L"mica";else if(wcscmp(commandLine,L"--explorer-mica-alt")==0)effect=L"micaAlt";else if(wcscmp(commandLine,L"--explorer-none")==0)effect=L"none";else if(wcscmp(commandLine,L"--explorer-restore")==0){restore=true;effect=L"";}else return 10;
        wchar_t exe[32768]{};GetModuleFileNameW(nullptr,exe,32768);std::wstring dir=exe;dir=dir.substr(0,dir.find_last_of(L"\\")+1);return explorer_background::run(effect,restore,dir);}
    if(wcscmp(commandLine,L"--shell-apply")==0||wcscmp(commandLine,L"--shell-restore")==0)return 10;
    if(wcscmp(commandLine,L"--close-running")==0){auto old=FindWindowW(ClassName,nullptr);if(!old)return 0;DWORD_PTR result=0;return SendMessageTimeoutW(old,WM_APP+3,0,0,SMTO_ABORTIFHUNG,5000,&result)?0:1;}
    bool testing=wcscmp(commandLine,L"--self-test")==0;
    HANDLE mutex=CreateMutexW(nullptr,FALSE,testing?L"Local\\LiquidGlass.Control.SelfTest":L"Local\\LiquidGlass.Control.SingleInstance");if(!mutex)return 1;
    if(GetLastError()==ERROR_ALREADY_EXISTS){auto old=FindWindowW(ClassName,nullptr);if(old)PostMessageW(old,WM_APP+2,0,0);CloseHandle(mutex);return 0;}
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_SYSTEM_AWARE);dpi=GetDpiForSystem();
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX common{};common.dwSize=sizeof(common);common.dwICC=ICC_TAB_CLASSES;InitCommonControlsEx(&common);
    wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);iniPath=path;iniPath=iniPath.substr(0,iniPath.find_last_of(L"\\")+1)+L"settings.ini";
    english=readEnglish(iniPath);
    motionEnabled=GetPrivateProfileIntW(L"Native",L"Motion",1,iniPath.c_str())!=0;
    volumeOn=GetPrivateProfileIntW(L"Native",L"VolumeOn",nativeBackupExists()?1:0,iniPath.c_str())!=0;brightnessOn=GetPrivateProfileIntW(L"Native",L"BrightnessOn",nativeBackupExists()?1:0,iniPath.c_str())!=0;startupAnimation=GetPrivateProfileIntW(L"UI",L"StartupAnimation",1,iniPath.c_str())!=0;
    background=CreateSolidBrush(RGB(246,248,252));WNDCLASSEXW c{};c.cbSize=sizeof(c);c.lpfnWndProc=proc;c.hInstance=instance;c.lpszClassName=ClassName;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hIcon=(HICON)LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_SHARED);c.hIconSm=(HICON)LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);c.hbrBackground=background;
    if(!RegisterClassExW(&c)){DeleteObject(background);CloseHandle(mutex);return 1;}
    RECT size={0,0,MulDiv(790,dpi,96),MulDiv(630,dpi,96)};AdjustWindowRectEx(&size,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE,0);
    mainWindow=CreateWindowExW(0,ClassName,L"Liquid Glass",WS_CLIPCHILDREN|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,size.right-size.left,size.bottom-size.top,nullptr,nullptr,instance,nullptr);
    if(!mainWindow){DeleteObject(background);CloseHandle(mutex);return 1;}buildUI();
    if(wcscmp(commandLine,L"--self-test")==0){int result=selfTest();DestroyWindow(mainWindow);for(auto&icon:appIcons)if(icon.second)DestroyIcon(icon.second);destroyFonts();DeleteObject(background);CloseHandle(mutex);CoUninitialize();return result;}
    tray.cbSize=sizeof(tray);tray.hWnd=mainWindow;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=WM_APP+1;tray.hIcon=c.hIconSm?c.hIconSm:c.hIcon;tray.uVersion=NOTIFYICON_VERSION_4;
    if(!Shell_NotifyIconW(NIM_ADD,&tray)){MessageBoxW(mainWindow,tr(L"无法创建托盘图标，程序将退出。",L"Cannot create tray icon; exiting."),L"Liquid Glass",MB_ICONERROR);DestroyWindow(mainWindow);}else{Shell_NotifyIconW(NIM_SETVERSION,&tray);windowEvents=SetWinEventHook(EVENT_OBJECT_CREATE,EVENT_OBJECT_SHOW,nullptr,windowChanged,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);show();}
    MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(mainWindow,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
    if(windowEvents)UnhookWinEvent(windowEvents);if(nativeJob)CloseHandle(nativeJob);if(explorerJob)CloseHandle(explorerJob);for(auto&icon:appIcons)if(icon.second)DestroyIcon(icon.second);destroyFonts();DeleteObject(background);CloseHandle(mutex);CoUninitialize();return 0;
}
