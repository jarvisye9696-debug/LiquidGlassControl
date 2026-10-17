#pragma once
#include <windows.h>
#include <string>
namespace glass {
constexpr DWORD Magic=0x4c474531, Version=2;
inline std::wstring runtimeName(const std::wstring&path){auto at=path.find_last_of(L"\\/");return path.substr(at==std::wstring::npos?0:at+1);}
inline std::wstring threadClassName(const std::wstring&path){return L"LiquidGlass.Engine.Thread.v2."+runtimeName(path);}
constexpr wchar_t AttachMessage[]=L"LiquidGlass.Engine.Attach.v2";
constexpr wchar_t RefreshMessage[]=L"LiquidGlass.Engine.Refresh.v2";
constexpr wchar_t StatusMessage[]=L"LiquidGlass.Engine.Status.v2";
constexpr wchar_t AttachAckMessage[]=L"LiquidGlass.Engine.AttachAck.v2";
constexpr CLSID TapId={0x973361c7,0xcbe9,0x4da4,{0x9c,0x96,0x3b,0x70,0x18,0x3b,0xa1,0x62}};
inline CLSID runtimeTapId(const std::wstring&path){DWORD hash=2166136261u;for(auto c:runtimeName(path)){hash^=(DWORD)c;hash*=16777619u;}auto id=TapId;id.Data1^=hash;return id;}
enum Feature:DWORD {Volume=1,Brightness=2,Taskbar=4,Start=8,Notifications=16,Explorer=32,Dock=64};
struct Config {
    DWORD magic=Magic,version=Version,owner=0;volatile LONG sequence=0;
    DWORD features=0,opacity=35,radius=12,tint=0,blur=12;BOOL economy=FALSE;
};
inline std::wstring mappingName(DWORD owner){return L"Local\\LiquidGlass.Engine.Config.v2."+std::to_wstring(owner);}
inline DWORD bounded(DWORD n,DWORD lo,DWORD hi){return n<lo?lo:n>hi?hi:n;}
inline Config normalize(Config c){c.features&=127;c.blur=bounded(c.blur,0,40);c.opacity=bounded(c.opacity,5,90);c.radius=bounded(c.radius,0,28);c.tint=bounded(c.tint,0,2);return c;}
// Names only identify material surfaces. No width, height, padding, alignment,
// taskbar position or system work-area property is ever written by this engine.
enum Surface {NoSurface,TaskbarFill,OsdPlate,VolumePlate,BrightnessPlate,StartPlate,StartOverlay,NotificationPlate,ExplorerPlate,TaskbarRoot,TaskbarOverlay,StartClear,NotificationClear,ExplorerClear,DockRoot,DockIcon};
inline DWORD feature(Surface s){switch(s){case TaskbarRoot:case TaskbarOverlay:case TaskbarFill:return Taskbar;case VolumePlate:return Volume;case BrightnessPlate:return Brightness;case StartClear:case StartOverlay:case StartPlate:return Start;case NotificationClear:case NotificationPlate:return Notifications;case ExplorerClear:case ExplorerPlate:return Explorer;case DockRoot:case DockIcon:return Dock;default:return 0;}}
inline Surface identify(const std::wstring&name,const std::wstring&type,int host){
    if(host==0){if(type==L"Taskbar.TaskbarFrame")return DockRoot;if(type==L"Taskbar.TaskListButton"||type==L"Taskbar.TaskListLabeledButton")return DockIcon;if(name==L"SystemTrayFrameGrid")return TaskbarRoot;if(name==L"BackgroundStroke"&&type.find(L"Rectangle")!=std::wstring::npos)return TaskbarOverlay;
        if(name==L"BackgroundFill"&&type.find(L"Rectangle")!=std::wstring::npos)return TaskbarFill;
        if(name==L"ConfirmatorMainGrid")return OsdPlate;
        if(name==L"VolumeConfirmator")return VolumePlate;
        if(name==L"BrightnessConfirmator")return BrightnessPlate;
    }
    if(host==1){if(name==L"RootGrid"&&type.find(L"Grid")!=std::wstring::npos)return StartClear;if(name==L"AcrylicOverlay")return StartOverlay;if(name==L"AcrylicBorder"||name==L"AppBorder")return StartPlate;}
    if(host==2&&(name==L"PageContent"||name==L"FullScreenPageRoot"))return NotificationClear;
    if(host==2&&(name==L"NotificationCenterGrid"||name==L"CalendarCenterGrid"||name==L"ControlCenterGrid"||name==L"ControlCenterRegion"||name==L"ToastBackgroundBorder"||name==L"ToastBackgroundBorder2"||name==L"CalendarControlScrollViewer"||name==L"CalendarHeaderMinimizedOverlay"||name==L"NotificationCenterTopBanner"||name==L"ListContent"))return NotificationPlate;
    if(host==3&&(name==L"PART_LayoutRoot"||name==L"RootContainer"||name==L"NavigationBarControlGrid"||name==L"DetailsViewControlRootGrid"||name==L"GalleryRootGrid"))return ExplorerClear;
    if(host==3&&(name==L"FileExplorerAddressBarGrid"||name==L"FileExplorerSearchBox"||name==L"CommandBarControlRootGrid"||name==L"FileExplorerCommandBar"||name==L"NavigationBarControlGrid"||name==L"HomeViewRootGrid"||name==L"GalleryRootGrid"||name==L"DetailsViewControlRootGrid"||name==L"GalleryViewControl"))return ExplorerPlate;
    return NoSurface;
}
inline bool active(Surface s,DWORD f){switch(s){case TaskbarRoot:case TaskbarOverlay:case TaskbarFill:return (f&Taskbar)!=0;case OsdPlate:return (f&(Volume|Brightness))!=0;case VolumePlate:return (f&Volume)!=0;case BrightnessPlate:return (f&Brightness)!=0;case StartClear:case StartPlate:case StartOverlay:return (f&Start)!=0;case NotificationClear:case NotificationPlate:return (f&Notifications)!=0;case ExplorerClear:case ExplorerPlate:return (f&Explorer)!=0;case DockRoot:case DockIcon:return (f&Dock)!=0;default:return false;}}
}
