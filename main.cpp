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
#include <powrprof.h>
#include "localization.h"
#include "glass_controller.h"
#include "shell_diagnostics.h"
#include "taskbar_autohide.h"
#include "explorer_background.h"
#include "native_osd.h"
#include "native_switches.h"
#include "shell_materials.h"
#include "shell_materials_test.h"
#include "full_themes.h"
#include "full_themes_test.h"

namespace {
constexpr wchar_t ClassName[]=L"LiquidGlass.Control";
enum {Language=101,Refresh,Apply,Restore,Pause,Scope,Material,Corners,List=113,Status,Note,
    Header=130,Pages,Compatibility,SettingsNote=152,ExitApp,
    NativeInfo=162,VolumeHeading=166,BrightnessHeading,
    VolumeOn=180,VolumeOff,BrightnessOn,BrightnessOff,SettingsPages=190,LanguageLabel,StartupAnimation,MotionText,CompatText,AboutText,ColorThemeLabel,ColorTheme,
    TaskbarToggle=210,StartToggle,NotificationToggle,TaskbarHeading,StartHeading,NotificationHeading,TaskbarHint,StartHint,NotificationHint,
    ExplorerToggle=230,ExplorerHeading,OpacityLabel,OpacitySlider,RadiusLabel,RadiusSlider,TintLabel,TintCombo,BatteryFriendly,EffectNote,EffectValue,DockToggle,DockHeading,AutoHideToggle,AutoHideHeading,BlurLabel,BlurSlider,CapsuleToggle,CapsuleHeading};
HWND mainWindow{}, language{}, scope{}, material{}, corners{}, list{}, status{}, note{};
HFONT font{}; HBRUSH background{}; NOTIFYICONDATAW tray{};
bool enabled=false,busy=false; UINT dpi=96;
std::map<HWND,RECT> controlLayout;
HWINEVENTHOOK windowEvents{};unsigned attributeWrites=0;
bool volumeOn=false,brightnessOn=false,startupAnimation=true,startupPlayed=false;ULONGLONG startupTime=0;RECT startupRect{};
HWND settingsTabs{};int settingsPage=0,buildingSettings=-1;std::vector<HWND> settingsControls[3];std::map<std::wstring,HICON> appIcons;
glass::Controller engine;glass::Config glassConfig;bool explorerOn=false,dockOn=false,capsuleOn=false,autoHideOn=false,batteryFriendly=true;taskbar_hide::Controller autoHide;HANDLE migrationJob{};HPOWERNOTIFY powerNotification{},acNotification{};bool engineReady=false;
DWORD confirmedMaterials=0,failedMaterials=0,pausedFeatures=0,fallbackMaterials=0;
constexpr GUID PowerSavingStatus={0xe00958c0,0xc213,0x4ace,{0xac,0x77,0xfe,0xcc,0xed,0x2e,0xee,0xa5}};
void applyGlass(bool discover=true);
void updateEffectLabels();
void observeWindows();
bool migrationNeeded();
void migrateLegacy();
std::wstring iniPath;
HWND tabs{},compatibility{},nativeInfo{};
bool shellOn[3]{};
std::vector<HWND> pageControls[3];int buildingPage=-1,currentPage=0;
bool exitAfterRestore=false;
void restoreAll(bool exit=false);
void updateCompatibility();
void updateNativeInfo();
void selectSettingsPage(int page);
void finishPageMotion();void beginPageMotion();
bool shellBackupExists();
struct Entry {HWND hwnd; DWORD pid; std::wstring title,exe,cls;HICON icon{};};
struct Snapshot {DWORD pid;std::wstring cls;int backdrop{},corner{};bool hasBackdrop=false,hasCorner=false;int appliedBackdrop=-1,appliedCorner=-1;};
std::vector<Entry> entries;std::map<HWND,Snapshot> originals;
std::wstring selectedExe;int mode=0,backdrop=3,corner=2;
const wchar_t* tr(const wchar_t* zh,const wchar_t* en){return l10n::translate(zh,en);}
int selection(HWND h){return (int)SendMessageW(h,CB_GETCURSEL,0,0);}
void message(const std::wstring& s){SetWindowTextW(status,s.c_str());}
std::wstring classOf(HWND h){wchar_t c[256]{};GetClassNameW(h,c,256);return c;}
std::wstring executable(DWORD pid){HANDLE p=OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);wchar_t path[32768]{};DWORD n=32768;if(p){if(!QueryFullProcessImageNameW(p,0,path,&n))path[0]=0;CloseHandle(p);}std::wstring s=path;auto i=s.find_last_of(L"\\/");return i==std::wstring::npos?s:s.substr(i+1);}
bool same(const std::wstring&a,const std::wstring&b){return _wcsicmp(a.c_str(),b.c_str())==0;}
bool systemExecutable(const std::wstring&exe){return same(exe,L"StartMenuExperienceHost.exe")||same(exe,L"SearchHost.exe")||same(exe,L"SearchApp.exe")||same(exe,L"ShellExperienceHost.exe")||same(exe,L"ShellHost.exe");}
bool usesExplorer(int selectedMode,const std::wstring&exe){return selectedMode==1||selectedMode==2||(selectedMode==0&&same(exe,L"explorer.exe"));}
bool explorerBackupExists(){auto dir=iniPath.substr(0,iniPath.find_last_of(L"\\")+1);return GetFileAttributesW((dir+L"explorer-background-originals.bin").c_str())!=INVALID_FILE_ATTRIBUTES;}
bool nativeBackupExists(){auto dir=iniPath.substr(0,iniPath.find_last_of(L"\\")+1);return GetFileAttributesW((dir+L"native-osd-originals.bin").c_str())!=INVALID_FILE_ATTRIBUTES||GetFileAttributesW((dir+L"native-osd-switches.bin").c_str())!=INVALID_FILE_ATTRIBUTES;}
std::wstring settingsDir(){return iniPath.substr(0,iniPath.find_last_of(L"\\")+1);}
bool shellBackupExists(){for(int i=0;i<3;++i)if(full_themes::pending(i,settingsDir())||shell_materials::exists(i,settingsDir()))return true;return false;}
void updateNativeInfo(){
    bool pending=migrationJob!=nullptr;
    if(nativeInfo)SetWindowTextW(nativeInfo,pending?tr(L"正在恢复旧版主题，请稍候……",L"Restoring the previous version's theme..."):tr(L"独立材质引擎 · 不需要 Windhawk · 保留系统布局\r\n设置 → 玻璃效果：调整着色、透明感、圆角与省电。",L"Standalone materials · No Windhawk required · System layout preserved\r\nSettings > Glass: tint, opacity, corners and power saving."));
    int ids[]={VolumeOn,BrightnessOn,TaskbarToggle,StartToggle,NotificationToggle,ExplorerToggle,DockToggle,CapsuleToggle};bool states[]={volumeOn,brightnessOn,shellOn[0],shellOn[1],shellOn[2],explorerOn,dockOn,capsuleOn};
    for(int i=0;i<8;++i){HWND h=GetDlgItem(mainWindow,ids[i]);if(!h)continue;SendMessageW(h,BM_SETCHECK,states[i]?BST_CHECKED:BST_UNCHECKED,0);EnableWindow(h,!pending&&engineReady&&!migrationNeeded());InvalidateRect(h,nullptr,FALSE);}
    int headings[]={VolumeHeading,BrightnessHeading,TaskbarHeading,StartHeading,NotificationHeading,ExplorerHeading,DockHeading,CapsuleHeading};const wchar_t*zh[]={L"音量提示",L"亮度提示",L"任务栏",L"开始菜单",L"通知中心与日历",L"资源管理器",L"Dock 悬停动画",L"任务栏胶囊"};const wchar_t*en[]={L"Volume indicator",L"Brightness indicator",L"Taskbar",L"Start menu",L"Notification center and calendar",L"File Explorer",L"Dock hover animation",L"Taskbar capsule"};
    for(int i=0;i<8;++i){DWORD bit=1u<<i;std::wstring text=tr(zh[i],en[i]);if(states[i])text+=(pausedFeatures&bit)?tr(L" · 已暂停",L" · Paused"):(failedMaterials&bit)?tr(L" · 未生效",L" · Not applied"):(fallbackMaterials&bit)?tr(L" · 亚克力（模糊固定）",L" · Acrylic (fixed blur)"):(confirmedMaterials&bit)?(i>=6?tr(L" · 已应用",L" · Applied"):tr(L" · 已应用（自定义模糊）",L" · Applied (custom blur)")):tr(L" · 等待连接",L" · Connecting");SetWindowTextW(GetDlgItem(mainWindow,headings[i]),text.c_str());}
    SendMessageW(GetDlgItem(mainWindow,AutoHideToggle),BM_SETCHECK,autoHideOn?BST_CHECKED:BST_UNCHECKED,0);
    for(int id:{TaskbarHint,StartHint,NotificationHint})SetWindowTextW(GetDlgItem(mainWindow,id),L"");
}
bool shellClass(const std::wstring& c){return c==L"Shell_TrayWnd"||c==L"Shell_SecondaryTrayWnd"||c==L"Progman"||c==L"WorkerW"||c==L"XamlExplorerHostIslandWindow";}
bool valid(HWND h,const Snapshot& s){DWORD pid=0;GetWindowThreadProcessId(h,&pid);return IsWindow(h)&&pid==s.pid&&classOf(h)==s.cls&&GetPropW(h,L"LiquidGlass.Control.Owner")==mainWindow;}
void saveLanguage(){l10n::save(iniPath);}
void setRulesActive(bool active){enabled=active;if(!active)KillTimer(mainWindow,3);observeWindows();}
void CALLBACK windowChanged(HWINEVENTHOOK,DWORD,HWND h,LONG object,LONG child,DWORD,DWORD){
    if((!enabled&&!glassConfig.features)||!h||object!=OBJID_WINDOW||child!=CHILDID_SELF||!IsWindowVisible(h)||GetAncestor(h,GA_ROOT)!=h)return;
    DWORD pid=0;GetWindowThreadProcessId(h,&pid);if(pid!=GetCurrentProcessId())SetTimer(mainWindow,3,180,nullptr);
}
void observeWindows(){bool need=enabled||glassConfig.features;if(need&&!windowEvents)windowEvents=SetWinEventHook(EVENT_OBJECT_CREATE,EVENT_OBJECT_SHOW,nullptr,windowChanged,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);else if(!need&&windowEvents){UnhookWinEvent(windowEvents);windowEvents=nullptr;KillTimer(mainWindow,3);}}
void selectPage(int page){
    if(page<0||page>2)return;bool changed=currentPage!=page;finishPageMotion();currentPage=page;
    SendMessageW(mainWindow,WM_SETREDRAW,FALSE,0);
    for(int p=0;p<3;++p)for(HWND h:pageControls[p])ShowWindow(h,p==page?SW_SHOWNA:SW_HIDE);
    if(page==2)selectSettingsPage(settingsPage);
    if(page==1)updateNativeInfo();
    TabCtrl_SetCurSel(tabs,page);SendMessageW(mainWindow,WM_SETREDRAW,TRUE,0);RedrawWindow(mainWindow,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);
    if(HWND focused=GetFocus();focused&&IsChild(mainWindow,focused)&&!IsWindowVisible(focused))SetFocus(tabs);
    if(changed)beginPageMotion();
}
void selectSettingsPage(int page){if(page<0||page>2)return;bool changed=settingsPage!=page;finishPageMotion();settingsPage=page;for(int p=0;p<3;++p)for(HWND h:settingsControls[p])ShowWindow(h,currentPage==2&&p==page?SW_SHOWNA:SW_HIDE);TabCtrl_SetCurSel(settingsTabs,page);RedrawWindow(settingsTabs,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE);InvalidateRect(mainWindow,nullptr,FALSE);if(HWND focused=GetFocus();focused&&IsChild(mainWindow,focused)&&!IsWindowVisible(focused))SetFocus(currentPage==2?settingsTabs:tabs);if(changed)beginPageMotion();}
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
    if(systemExecutable(targetExe))return false;
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
    if(systemExecutable(exe))return TRUE;
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
void refresh(bool recollect=true){
    HWND previous=nullptr;int n=(int)SendMessageW(list,LB_GETCURSEL,0,0);if(n>=0&&n<(int)entries.size())previous=entries[n].hwnd;
    SendMessageW(list,WM_SETREDRAW,FALSE,0);if(recollect)entries=collect();SendMessageW(list,LB_RESETCONTENT,0,0);int sel=0;
    for(size_t i=0;i<entries.size();++i){if(!entries[i].icon)entries[i].icon=applicationIcon(entries[i]);bool explorer=entries[i].cls==L"CabinetWClass"||entries[i].cls==L"ExploreWClass";std::wstring text=entries[i].exe+(explorer?tr(L" [整窗后端]  —  ",L" [whole-window backend]  —  "):tr(L" [仅外框/待验证]  —  ",L" [frame only/unverified]  —  "))+entries[i].title;SendMessageW(list,LB_ADDSTRING,0,reinterpret_cast<LPARAM>(text.c_str()));if(entries[i].hwnd==previous)sel=(int)i;}
    if(!entries.empty())SendMessageW(list,LB_SETCURSEL,sel,0);
    SendMessageW(list,WM_SETREDRAW,TRUE,0);InvalidateRect(list,nullptr,FALSE);
    for(auto i=appIcons.begin();i!=appIcons.end();){bool used=false;for(auto&e:entries)if(e.icon==i->second){used=true;break;}if(!used){DestroyIcon(i->second);i=appIcons.erase(i);}else ++i;}
    updateCompatibility();
}
void updateCompatibility(){
    if(!compatibility)return;
    int n=(int)SendMessageW(list,LB_GETCURSEL,0,0);bool explorer=selection(scope)==1||(n>=0&&n<(int)entries.size()&&(entries[n].cls==L"CabinetWClass"||entries[n].cls==L"ExploreWClass"));
    SetWindowTextW(compatibility,explorer?tr(L"资源管理器：独立 XAML 材质；经典内容区可能仍不透明。",L"Explorer: standalone XAML materials; classic content may remain opaque."):tr(L"其他应用：仅尝试外框材质；Chrome / Electron 等内容区通常不受影响。",L"Other apps: frame material only; Chrome / Electron content usually stays opaque."));
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
    if(migrationJob)return;
    int nextMode=selection(scope);std::wstring exe;
    if(nextMode==0){int n=(int)SendMessageW(list,LB_GETCURSEL,0,0);if(n<0||n>=(int)entries.size()){message(tr(L"请先选择一个应用窗口。",L"Select an application window first."));return;}exe=entries[n].exe;}
    setRulesActive(false);restoreWindows();if(!originals.empty())return;
    mode=nextMode;selectedExe=exe;int idx=selection(material);backdrop=idx==0?3:idx==1?2:idx==2?4:1;
    corner=selection(corners)==0?2:selection(corners)==1?3:1;
    if(usesExplorer(mode,selectedExe)){explorerOn=true;applyGlass();}
    setRulesActive(true);sync(true);
}
void restoreAll(bool exit){
    if(migrationJob)return;
    setRulesActive(false);restoreWindows();if(!originals.empty())return;
    if(!autoHide.disable()){message(tr(L"任务栏恢复失败，恢复记录已保留。",L"Taskbar restore failed; recovery record retained."));return;}autoHideOn=dockOn=false;WritePrivateProfileStringW(L"Taskbar",L"AutoHide",L"0",iniPath.c_str());volumeOn=brightnessOn=explorerOn=false;for(bool&on:shellOn)on=false;applyGlass(false);
    if(migrationNeeded()){exitAfterRestore=exit;migrateLegacy();return;}
    message(tr(L"已还原所有效果。",L"All effects restored."));if(exit)DestroyWindow(mainWindow);
}
void combo(HWND h,std::initializer_list<const wchar_t*> labels,int sel){SendMessageW(h,CB_RESETCONTENT,0,0);for(auto s:labels)SendMessageW(h,CB_ADDSTRING,0,reinterpret_cast<LPARAM>(s));SendMessageW(h,CB_SETCURSEL,sel<0?0:sel,0);}
#include "ui_style.h"
#include "control_ui.h"
#include "glass_features.h"
int selfTest(){
    bool passed=FindResourceW(nullptr,MAKEINTRESOURCEW(202),RT_RCDATA)!=nullptr;HRESULT graphicsResult=E_FAIL;
    // Exercise the embedded engine and its COM factory without attaching to
    // Explorer or changing any live shell material. CI needs no Windhawk.
    auto testDir=settingsDir()+L"engine-test-"+std::to_wstring(GetCurrentProcessId())+L"\\";
    bool made=CreateDirectoryW(testDir.c_str(),nullptr)!=FALSE;
    if(made){glass::Controller probe;passed=probe.initialize(testDir)&&probe.available()&&probe.factoryTest()&&passed;graphicsResult=probe.graphicsTest();passed=SUCCEEDED(graphicsResult)&&passed;passed=SUCCEEDED(probe.connectTest())&&passed;auto runtime=probe.runtimePath();probe.shutdown();DeleteFileW(runtime.c_str());RemoveDirectoryW(testDir.c_str());}else passed=false;
    glass::Config invalid;invalid.features=0xffffffff;invalid.opacity=500;invalid.radius=999;invalid.tint=999;auto safe=glass::normalize(invalid);passed=passed&&safe.features==255&&glass::active(glass::TaskbarFill,glass::Capsule)&&glass::active(glass::TaskbarRoot,glass::Capsule)&&!glass::active(glass::TaskbarFill,glass::Dock)&&safe.opacity==90&&safe.radius==28&&safe.tint==2;
    passed=passed&&taskbar_hide::intersects({0,0,100,100},{50,50,120,120})&&!taskbar_hide::intersects({0,0,10,10},{10,10,20,20});
    passed=glass::selfTestSnapshot()&&passed;
    passed=passed&&glass::identify(L"BackgroundFill",L"Windows.UI.Xaml.Shapes.Rectangle",0)==glass::TaskbarFill&&glass::identify(L"BackgroundFill",L"Windows.UI.Xaml.Shapes.Rectangle",3)==glass::NoSurface&&glass::identify(L"MyDocument",L"Grid",3)==glass::NoSurface;
    passed=passed&&!glass::active(glass::VolumePlate,glass::Brightness)&&glass::active(glass::BrightnessPlate,glass::Brightness)&&!glass::active(glass::TaskbarFill,glass::Start);
    // Exercise first launch and saved preferences without touching the user's INI.
    auto userIni=iniPath;auto userLanguage=l10n::current;
    iniPath=userIni+L".language-test-"+std::to_wstring(GetCurrentProcessId());
    passed=passed&&l10n::read(iniPath)==l10n::English;
    for(int i=0;i<l10n::Count;++i){l10n::current=(l10n::Language)i;saveLanguage();passed=passed&&l10n::read(iniPath)==l10n::current;}
    passed=DeleteFileW(iniPath.c_str())&&passed;iniPath=userIni;l10n::current=userLanguage;
    // Removed commands must not retain hidden controls or message handlers.
    for(int removedId:{109,110,111,112,116,117,120,121})passed=passed&&!GetDlgItem(mainWindow,removedId);
    passed=passed&&usesExplorer(0,L"explorer.exe")&&usesExplorer(1,L"")&&usesExplorer(2,L"")&&!usesExplorer(0,L"notepad.exe");
    passed=passed&&TabCtrl_GetItemCount(tabs)==3;
    selectPage(1);passed=passed&&((GetWindowLongPtrW(GetDlgItem(mainWindow,VolumeOn),GWL_STYLE)&WS_VISIBLE)!=0)&&((GetWindowLongPtrW(list,GWL_STYLE)&WS_VISIBLE)==0);
    passed=passed&&!GetDlgItem(mainWindow,VolumeOff)&&!GetDlgItem(mainWindow,BrightnessOff);
    for(int id:{VolumeOn,BrightnessOn,TaskbarToggle,StartToggle,NotificationToggle,ExplorerToggle,DockToggle,AutoHideToggle}){HWND h=GetDlgItem(mainWindow,id);RECT r{};GetWindowRect(h,&r);passed=passed&&h&&((GetWindowLongPtrW(h,GWL_STYLE)&BS_TYPEMASK)==BS_AUTOCHECKBOX)&&r.right-r.left<=MulDiv(110,dpi,96)&&r.bottom-r.top<=MulDiv(40,dpi,96);}
    passed=shell_materials::regressionTest(settingsDir())&&passed;
    passed=full_themes::regressionTest(settingsDir())&&passed;
    passed=taskbar_hide::Controller::regressionTest(iniPath+L".taskbar-test-"+std::to_wstring(GetCurrentProcessId()))&&passed;
    selectPage(2);passed=passed&&((GetWindowLongPtrW(GetDlgItem(mainWindow,SettingsNote),GWL_STYLE)&WS_VISIBLE)!=0);
    passed=passed&&TabCtrl_GetItemCount(settingsTabs)==3;selectSettingsPage(1);passed=passed&&((GetWindowLongPtrW(language,GWL_STYLE)&WS_VISIBLE)==0);
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
    l10n::current=l10n::English;localize();wchar_t text[256]{};
    GetWindowTextW(GetDlgItem(mainWindow,Apply),text,256);passed=passed&&wcscmp(text,L"Apply")==0;
    GetWindowTextW(GetDlgItem(mainWindow,Restore),text,256);passed=passed&&wcscmp(text,L"Restore")==0;
    GetWindowTextW(GetDlgItem(mainWindow,VolumeOn),text,256);passed=passed&&wcscmp(text,L"Volume glass")==0;
    GetWindowTextW(GetDlgItem(mainWindow,NotificationHeading),text,256);passed=passed&&wcsncmp(text,L"Notification center and calendar",wcslen(L"Notification center and calendar"))==0;
    passed=passed&&SendMessageW(scope,CB_GETCOUNT,0,0)==3&&SendMessageW(material,CB_GETCOUNT,0,0)==4;
    l10n::current=l10n::Simplified;localize();GetWindowTextW(GetDlgItem(mainWindow,Apply),text,256);passed=passed&&wcscmp(text,L"应用")==0;
    GetWindowTextW(GetDlgItem(mainWindow,Restore),text,256);passed=passed&&wcscmp(text,L"还原")==0;
    GetWindowTextW(GetDlgItem(mainWindow,VolumeOn),text,256);passed=passed&&wcscmp(text,L"音量玻璃")==0;
    GetWindowTextW(GetDlgItem(mainWindow,StartHeading),text,256);passed=passed&&wcsncmp(text,L"开始菜单",wcslen(L"开始菜单"))==0;
    const wchar_t* expected[]={L"Apply",L"应用",L"應用",L"Zastosuj",L"Áp dụng"}; for(int i=0;i<l10n::Count;++i){l10n::current=(l10n::Language)i;localize();GetWindowTextW(GetDlgItem(mainWindow,Apply),text,256);passed=passed&&wcscmp(text,expected[i])==0&&selection(language)==i&&SendMessageW(language,CB_GETCOUNT,0,0)==l10n::Count;} l10n::current=userLanguage;localize();
    for(auto&e:entries)passed=passed&&e.icon!=nullptr;
    auto switches=native_switches::fields(1,2,1);passed=passed&&switches.size()==6&&switches[1].data!=switches[4].data;
    for(int removed:{140,141,142,143,144,150,151,154,155})passed=passed&&!GetDlgItem(mainWindow,removed);
    auto nativeValues=native_osd::desired(0);passed=passed&&nativeValues.size()==4&&wcscmp(native_osd::Target,L"Grid#ConfirmatorMainGrid")==0;
    passed=passed&&font&&headingFont&&actionFont&&!GetDlgItem(mainWindow,MotionText)&&!GetDlgItem(mainWindow,CompatText);
    // Exercise control geometry and font recreation at common monitor DPIs,
    // including a round trip. This does not change the user's display settings.
    {UINT savedDpi=dpi;for(UINT testDpi:{96u,144u,192u,96u}){dpi=testDpi;resizeControls();for(int languageIndex=0;languageIndex<l10n::Count;++languageIndex){l10n::current=(l10n::Language)languageIndex;localize();for(HWND tab:{tabs,settingsTabs}){RECT client{},last{};GetClientRect(tab,&client);TabCtrl_GetItemRect(tab,TabCtrl_GetItemCount(tab)-1,&last);passed=passed&&last.right<=client.right;}for(int id:{OpacitySlider,BlurSlider,RadiusSlider,CapsuleToggle}){RECT r{};GetWindowRect(GetDlgItem(mainWindow,id),&r);passed=passed&&r.right>r.left&&r.bottom>r.top;}}LOGFONTW lf{};GetObjectW(font,sizeof(lf),&lf);passed=passed&&lf.lfHeight==-MulDiv(15,dpi,96);}dpi=savedDpi;l10n::current=userLanguage;resizeControls();localize();}
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
    const char* result=passed?(supported?"PASS: embedded standalone engine, COM factory and GPU blur graph; smart-hide recovery and conflicts; isolated feature masks; bounded material controls; five-language compact switches; legacy-theme restore/conflicts; isolated shell apply/restore/conflicts; native motion rule; v1/v2 backup compatibility; zero duplicate DWM writes; restore.\r\n":"PASS: embedded standalone engine, COM factory and GPU blur graph; smart-hide recovery and conflicts; isolated feature masks; bounded material controls; five-language compact switches; legacy-theme restore/conflicts; isolated shell apply/restore/conflicts; native motion rule; v1/v2 backup compatibility. SKIP: DWM backdrop unavailable.\r\n"):"FAIL: UI, caching, backup compatibility, or restoration check.\r\n";
    HANDLE file=CreateFileW(report.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);DWORD written=0;
    if(file!=INVALID_HANDLE_VALUE){WriteFile(file,result,(DWORD)lstrlenA(result),&written,nullptr);char details[80]{};int n=snprintf(details,sizeof(details),"GPU composition test: 0x%08lx\r\n",(unsigned long)graphicsResult);WriteFile(file,details,n,&written,nullptr);CloseHandle(file);}else passed=false;
    return passed?0:2;
}
void show(){
    BOOL systemMotion=!reducedMotion();
    if(!startupPlayed&&startupAnimation&&systemMotion){startupPlayed=true;GetWindowRect(mainWindow,&startupRect);startupTime=GetTickCount64();SetWindowLongPtrW(mainWindow,GWL_EXSTYLE,GetWindowLongPtrW(mainWindow,GWL_EXSTYLE)|WS_EX_LAYERED);SetLayeredWindowAttributes(mainWindow,0,0,LWA_ALPHA);SetWindowPos(mainWindow,nullptr,startupRect.left,startupRect.top+MulDiv(12,dpi,96),0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);ShowWindow(mainWindow,SW_SHOW);SetTimer(mainWindow,4,uiFrameInterval(),nullptr);}else{startupPlayed=true;ShowWindow(mainWindow,SW_SHOW);}SetForegroundWindow(mainWindow);
}
void menu(){HMENU m=CreatePopupMenu();AppendMenuW(m,MF_STRING,1,tr(L"打开设置",L"Open settings"));AppendMenuW(m,MF_STRING,2,tr(L"还原",L"Restore"));AppendMenuW(m,MF_STRING,3,tr(L"还原并退出",L"Restore and exit"));POINT p;GetCursorPos(&p);SetForegroundWindow(mainWindow);int n=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,mainWindow,nullptr);DestroyMenu(m);if(n==1)show();if(n==2)restoreAll();if(n==3)restoreAll(true);PostMessageW(mainWindow,WM_NULL,0,0);}
LRESULT CALLBACK proc(HWND h,UINT msg,WPARAM w,LPARAM l){
    static UINT attachAck=RegisterWindowMessageW(glass::AttachAckMessage);if(msg==attachAck){engine.acknowledge((DWORD)w);return 0;}
    static UINT engineStatus=RegisterWindowMessageW(glass::StatusMessage);if(msg==engineStatus){DWORD flags=(DWORD)w&glassConfig.features&~pausedFeatures;if(!flags)return 0;if(FAILED((HRESULT)l)){failedMaterials|=flags;confirmedMaterials&=~flags;message(tr(L"部分系统材质未生效，请查看各项连接状态。",L"Some shell materials were not applied. Check each connection status."));}else{confirmedMaterials|=flags;failedMaterials&=~flags;if((HRESULT)l==S_FALSE)fallbackMaterials|=flags;else fallbackMaterials&=~flags;message(tr(L"系统材质属性已写入，请查看对应区域的外观。",L"Shell material properties written. Check the selected surface appearance."));}updateNativeInfo();return 0;}
    static UINT restart=RegisterWindowMessageW(L"TaskbarCreated");if(msg==restart){Shell_NotifyIconW(NIM_ADD,&tray);Shell_NotifyIconW(NIM_SETVERSION,&tray);if(glassConfig.features)engine.discover();return 0;}
    switch(msg){
    case WM_DPICHANGED:{finishPageMotion();dpi=HIWORD(w);auto suggested=reinterpret_cast<RECT*>(l);SetWindowPos(h,nullptr,suggested->left,suggested->top,suggested->right-suggested->left,suggested->bottom-suggested->top,SWP_NOZORDER|SWP_NOACTIVATE);resizeControls();return 0;}
    case WM_PAINT:paintChrome(h);return 0;
    case WM_ERASEBKGND:return 1;
    case WM_DRAWITEM:if(drawModernControl(reinterpret_cast<DRAWITEMSTRUCT*>(l)))return TRUE;break;
    case WM_MEASUREITEM:{auto item=reinterpret_cast<MEASUREITEMSTRUCT*>(l);if(item->CtlType==ODT_COMBOBOX){item->itemHeight=MulDiv(26,dpi,96);return TRUE;}if(item->CtlID==List){item->itemHeight=MulDiv(48,dpi,96);return TRUE;}}break;
    case WM_NOTIFY:if(reinterpret_cast<NMHDR*>(l)->code==TCN_SELCHANGE){if(reinterpret_cast<NMHDR*>(l)->idFrom==Pages)selectPage(TabCtrl_GetCurSel(tabs));else if(reinterpret_cast<NMHDR*>(l)->idFrom==SettingsPages)selectSettingsPage(TabCtrl_GetCurSel(settingsTabs));}return 0;
    case WM_COMMAND:switch(LOWORD(w)){
        case Language:if(HIWORD(w)==CBN_SELCHANGE){int selected=selection(language);if(selected>=0&&selected<l10n::Count){l10n::current=(l10n::Language)selected;saveLanguage();localize();}}break;
        case ColorTheme:if(HIWORD(w)==CBN_SELCHANGE)applyColorTheme(selection(GetDlgItem(h,ColorTheme)),true);break;
        case List:if(HIWORD(w)==LBN_SELCHANGE)updateCompatibility();break;
        case Scope:if(HIWORD(w)==CBN_SELCHANGE)updateCompatibility();break;
        case TintCombo:if(HIWORD(w)==CBN_SELCHANGE){glassConfig.tint=selection(GetDlgItem(h,TintCombo));applyGlass(false);}break;
        case BatteryFriendly:batteryFriendly=SendMessageW(GetDlgItem(h,BatteryFriendly),BM_GETCHECK,0,0)==BST_CHECKED;applyGlass(false);break;
        case StartupAnimation:startupAnimation=SendMessageW(GetDlgItem(h,StartupAnimation),BM_GETCHECK,0,0)==BST_CHECKED;WritePrivateProfileStringW(L"UI",L"StartupAnimation",startupAnimation?L"1":L"0",iniPath.c_str());break;
        case VolumeOn:volumeOn=!volumeOn;applyGlass();break;
        case BrightnessOn:brightnessOn=!brightnessOn;applyGlass();break;
        case DockToggle:dockOn=!dockOn;applyGlass();break;
        case CapsuleToggle:capsuleOn=!capsuleOn;applyGlass();break;
        case AutoHideToggle:autoHideOn=!autoHideOn;if(autoHideOn){if(!autoHide.enable(mainWindow,iniPath)){autoHideOn=false;autoHide.disable();message(tr(L"自动隐藏无法启用。",L"Auto-hide could not be enabled."));}}else if(!autoHide.disable())message(tr(L"任务栏恢复失败，恢复记录已保留。",L"Taskbar restore failed; recovery record retained."));WritePrivateProfileStringW(L"Taskbar",L"AutoHide",autoHideOn?L"1":L"0",iniPath.c_str());updateNativeInfo();break;
        case ExplorerToggle:explorerOn=!explorerOn;applyGlass();break;
        case TaskbarToggle:case StartToggle:case NotificationToggle:{int kind=LOWORD(w)-TaskbarToggle;shellOn[kind]=!shellOn[kind];applyGlass();break;}
        case ExitApp:restoreAll(true);break;
        case Refresh:refresh();break;case Apply:apply();break;
        case Restore:restoreAll();break;
        case Pause:ShowWindow(h,SW_HIDE);break;
        }return 0;
    case WM_HSCROLL:if((HWND)l==GetDlgItem(h,OpacitySlider)||(HWND)l==GetDlgItem(h,RadiusSlider)||(HWND)l==GetDlgItem(h,BlurSlider)){glassConfig.opacity=(DWORD)SendMessageW(GetDlgItem(h,OpacitySlider),TBM_GETPOS,0,0);glassConfig.radius=(DWORD)SendMessageW(GetDlgItem(h,RadiusSlider),TBM_GETPOS,0,0);glassConfig.blur=(DWORD)SendMessageW(GetDlgItem(h,BlurSlider),TBM_GETPOS,0,0);updateEffectLabels();if(LOWORD(w)==TB_ENDTRACK||LOWORD(w)==TB_THUMBPOSITION||LOWORD(w)==TB_LINEUP||LOWORD(w)==TB_LINEDOWN||LOWORD(w)==TB_PAGEUP||LOWORD(w)==TB_PAGEDOWN)applyGlass(false);}return 0;
    case WM_POWERBROADCAST:if(w==PBT_POWERSETTINGCHANGE||w==PBT_APMPOWERSTATUSCHANGE){refreshUiPreferences();applyGlass(false);}return TRUE;
    // A manual change to the taskbar's auto-hide setting is re-checked here. The timer coalesces broadcasts; our own writes find the state unchanged and do not write again.
    case WM_SETTINGCHANGE:case WM_THEMECHANGED:case WM_SYSCOLORCHANGE:refreshUiPreferences();applyColorTheme(colorTheme,false);if(engineReady)applyGlass(false);if(autoHideOn)SetTimer(h,7,150,nullptr);break;
    case WM_TIMER:if(w==12){tickColorTheme();}else if(w==13){tickPageMotion();}else if(w==7){KillTimer(h,7);if(!autoHide.update()){autoHideOn=false;WritePrivateProfileStringW(L"Taskbar",L"AutoHide",L"0",iniPath.c_str());updateNativeInfo();message(tr(L"自动隐藏已停止，请检查任务栏设置。",L"Auto-hide stopped; check taskbar settings."));}}else if(w==4){float t=std::min(1.f,(GetTickCount64()-startupTime)/180.f),ease=1-powf(1-t,3);SetLayeredWindowAttributes(h,0,(BYTE)(255*ease),LWA_ALPHA);SetWindowPos(h,nullptr,startupRect.left,startupRect.top+(int)((1-ease)*MulDiv(12,dpi,96)),0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);if(t>=1){KillTimer(h,4);SetWindowLongPtrW(h,GWL_EXSTYLE,GetWindowLongPtrW(h,GWL_EXSTYLE)&~WS_EX_LAYERED);RedrawWindow(h,nullptr,nullptr,RDW_INVALIDATE|RDW_ALLCHILDREN);}}else if(w==3){KillTimer(h,3);sync(false);if(glassConfig.features)engine.discover();}else if(w==2)pollMigration();else if(w==5){KillTimer(h,5);engine.expireHooks(true);}else if(w==6){KillTimer(h,6);failedMaterials|=glassConfig.features&~pausedFeatures&~confirmedMaterials;updateNativeInfo();if(failedMaterials)message(tr(L"部分区域尚未连接，已标记为未生效；打开对应区域后可重试。",L"Some surfaces are not connected and are marked Not applied; open them and retry."));}return 0;
    case WM_APP+1:if(LOWORD(l)==WM_CONTEXTMENU)menu();else if(LOWORD(l)==NIN_SELECT||LOWORD(l)==NIN_KEYSELECT)show();return 0;
    case WM_APP+2:show();return 0;
    case WM_APP+3:if(migrationJob)return 0;setRulesActive(false);restoreWindows();if(!originals.empty()){show();return 0;}if(!autoHide.disable()){show();return 0;}autoHideOn=false;WritePrivateProfileStringW(L"Taskbar",L"AutoHide",L"0",iniPath.c_str());engine.shutdown();DestroyWindow(h);return 0;
    case WM_APP+5:migrateLegacy();return 0;
    case WM_CLOSE:ShowWindow(h,SW_HIDE);return 0;
    case WM_QUERYENDSESSION:autoHide.disable();setRulesActive(false);restoreWindows();engine.shutdown();return TRUE;
    case WM_CTLCOLORLISTBOX:{SetBkColor(reinterpret_cast<HDC>(w),cardColor());SetTextColor(reinterpret_cast<HDC>(w),textColor());return reinterpret_cast<LRESULT>(cardBrush);}
    case WM_CTLCOLORSTATIC:case WM_CTLCOLORBTN:{int id=GetDlgCtrlID(reinterpret_cast<HWND>(l));bool common=id==Header||id==Status;SetBkMode(reinterpret_cast<HDC>(w),TRANSPARENT);SetTextColor(reinterpret_cast<HDC>(w),highContrast()?textColor():id==Header?textColor():palette().pressed);return reinterpret_cast<LRESULT>(common?background:cardBrush);}
    case WM_DESTROY:finishPageMotion();KillTimer(h,12);KillTimer(h,5);KillTimer(h,6);KillTimer(h,1);KillTimer(h,2);KillTimer(h,3);KillTimer(h,4);Shell_NotifyIconW(NIM_DELETE,&tray);PostQuitMessage(0);return 0;
    }return DefWindowProcW(h,msg,w,l);
}
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR commandLine,int){
    if(wcscmp(commandLine,L"--diagnose-shell")==0){wchar_t file[32768]{};GetModuleFileNameW(nullptr,file,32768);std::wstring path=file;path.resize(path.find_last_of(L"\\")+1);return glass::diagnoseShell(path+L"shell-diagnostics.txt");}
    if(wcscmp(commandLine,L"--legacy-restore-all")==0){wchar_t file[32768]{};GetModuleFileNameW(nullptr,file,32768);std::wstring dir=file;dir.resize(dir.find_last_of(L"\\")+1);return restoreLegacy(dir);}
    if(wcscmp(commandLine,L"--close-running")==0){auto old=FindWindowW(ClassName,nullptr);if(!old)return 0;DWORD_PTR result=0;return SendMessageTimeoutW(old,WM_APP+3,0,0,SMTO_ABORTIFHUNG,5000,&result)?0:1;}
    bool testing=wcscmp(commandLine,L"--self-test")==0;
    if(*commandLine&&!testing)return 10;
    HANDLE mutex=CreateMutexW(nullptr,FALSE,testing?L"Local\\LiquidGlass.Control.SelfTest":L"Local\\LiquidGlass.Control.SingleInstance");if(!mutex)return 1;
    if(GetLastError()==ERROR_ALREADY_EXISTS){auto old=FindWindowW(ClassName,nullptr);if(old)PostMessageW(old,WM_APP+2,0,0);CloseHandle(mutex);return 0;}
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);dpi=GetDpiForSystem();
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    INITCOMMONCONTROLSEX common{};common.dwSize=sizeof(common);common.dwICC=ICC_TAB_CLASSES|ICC_BAR_CLASSES;InitCommonControlsEx(&common);
    wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);iniPath=path;iniPath=iniPath.substr(0,iniPath.find_last_of(L"\\")+1)+L"settings.ini";
    l10n::current=l10n::read(iniPath);
    glassConfig.features=GetPrivateProfileIntW(L"Glass",L"Features",0,iniPath.c_str());glassConfig.opacity=GetPrivateProfileIntW(L"Glass",L"Opacity",35,iniPath.c_str());glassConfig.radius=GetPrivateProfileIntW(L"Glass",L"Radius",12,iniPath.c_str());glassConfig.tint=GetPrivateProfileIntW(L"Glass",L"Tint",0,iniPath.c_str());glassConfig.blur=GetPrivateProfileIntW(L"Glass",L"Blur",12,iniPath.c_str());glassConfig=glass::normalize(glassConfig);
    if(migrationNeeded())glassConfig.features=0;
    dockOn=(glassConfig.features&glass::Dock)!=0;capsuleOn=(glassConfig.features&glass::Capsule)!=0;autoHideOn=GetPrivateProfileIntW(L"Taskbar",L"AutoHide",0,iniPath.c_str())!=0;
    volumeOn=(glassConfig.features&glass::Volume)!=0;brightnessOn=(glassConfig.features&glass::Brightness)!=0;explorerOn=(glassConfig.features&glass::Explorer)!=0;for(int i=0;i<3;++i)shellOn[i]=(glassConfig.features&(glass::Taskbar<<i))!=0;
    batteryFriendly=GetPrivateProfileIntW(L"Glass",L"BatteryFriendly",1,iniPath.c_str())!=0;startupAnimation=GetPrivateProfileIntW(L"UI",L"StartupAnimation",1,iniPath.c_str())!=0;
    applyColorTheme(GetPrivateProfileIntW(L"UI",L"ColorTheme",0,iniPath.c_str()),false);WNDCLASSEXW c{};c.cbSize=sizeof(c);c.lpfnWndProc=proc;c.hInstance=instance;c.lpszClassName=ClassName;c.hCursor=LoadCursorW(nullptr,IDC_ARROW);c.hIcon=(HICON)LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,GetSystemMetrics(SM_CXICON),GetSystemMetrics(SM_CYICON),LR_SHARED);c.hIconSm=(HICON)LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);c.hbrBackground=background;
    if(!RegisterClassExW(&c)){DeleteObject(background);CloseHandle(mutex);return 1;}
    RECT size={0,0,MulDiv(790,dpi,96),MulDiv(630,dpi,96)};AdjustWindowRectEx(&size,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE,0);
    mainWindow=CreateWindowExW(0,ClassName,L"Liquid Glass",WS_CLIPCHILDREN|WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,size.right-size.left,size.bottom-size.top,nullptr,nullptr,instance,nullptr);
    if(!mainWindow){DeleteObject(background);CloseHandle(mutex);return 1;}
    // Keep the window inside the monitor work area. Default placement could put its bottom under the taskbar, hiding the lower controls; only the position changes, never the size or the taskbar layout.
    {MONITORINFO mi{};mi.cbSize=sizeof(mi);RECT w{};GetWindowRect(mainWindow,&w);if(GetMonitorInfoW(MonitorFromWindow(mainWindow,MONITOR_DEFAULTTONEAREST),&mi)){int width=w.right-w.left,height=w.bottom-w.top,x=w.left,y=w.top;if(y+height>mi.rcWork.bottom)y=mi.rcWork.bottom-height;if(y<mi.rcWork.top)y=mi.rcWork.top;if(x+width>mi.rcWork.right)x=mi.rcWork.right-width;if(x<mi.rcWork.left)x=mi.rcWork.left;SetWindowPos(mainWindow,nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);}}
    buildUI();themeWindow(mainWindow);
    if(wcscmp(commandLine,L"--self-test")==0){int result=selfTest();DestroyWindow(mainWindow);for(auto&icon:appIcons)if(icon.second)DestroyIcon(icon.second);destroyFonts();DeleteObject(background);CloseHandle(mutex);CoUninitialize();return result;}
    if(!autoHide.recover(iniPath)){autoHideOn=false;message(tr(L"任务栏恢复失败，恢复记录已保留。",L"Taskbar restore failed; recovery record retained."));}else if(autoHideOn&&!autoHide.enable(mainWindow,iniPath))autoHideOn=false;
    engineReady=engine.initialize(settingsDir());updateNativeInfo();
    if(!engineReady)message(engine.policyBlocked()?tr(L"Windows 应用程序控制策略阻止了引擎，请使用受信任签名或管理员批准的版本。",L"Windows application-control policy blocked the engine. A trusted signed or administrator-approved build is required."):tr(L"独立引擎无法加载，请检查程序所在目录是否可写。",L"Standalone engine could not load. Check that the app folder is writable."));
    powerNotification=RegisterPowerSettingNotification(mainWindow,&PowerSavingStatus,DEVICE_NOTIFY_WINDOW_HANDLE);acNotification=RegisterPowerSettingNotification(mainWindow,&GUID_ACDC_POWER_SOURCE,DEVICE_NOTIFY_WINDOW_HANDLE);
    tray.cbSize=sizeof(tray);tray.hWnd=mainWindow;tray.uID=1;tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;tray.uCallbackMessage=WM_APP+1;tray.hIcon=c.hIconSm?c.hIconSm:c.hIcon;tray.uVersion=NOTIFYICON_VERSION_4;
    if(!Shell_NotifyIconW(NIM_ADD,&tray)){MessageBoxW(mainWindow,tr(L"无法创建托盘图标，程序将退出。",L"Cannot create tray icon; exiting."),L"Liquid Glass",MB_ICONERROR);DestroyWindow(mainWindow);}else{Shell_NotifyIconW(NIM_SETVERSION,&tray);show();if(migrationNeeded())PostMessageW(mainWindow,WM_APP+5,0,0);else if(glassConfig.features)applyGlass();}
    MSG msg;while(GetMessageW(&msg,nullptr,0,0)>0){if(!IsDialogMessageW(mainWindow,&msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}
    if(windowEvents)UnhookWinEvent(windowEvents);if(powerNotification)UnregisterPowerSettingNotification(powerNotification);if(acNotification)UnregisterPowerSettingNotification(acNotification);engine.shutdown();if(migrationJob)CloseHandle(migrationJob);for(auto&icon:appIcons)if(icon.second)DestroyIcon(icon.second);destroyFonts();DeleteObject(background);CloseHandle(mutex);CoUninitialize();return 0;
}
