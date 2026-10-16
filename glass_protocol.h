#pragma once
#include <windows.h>
#include <string>
namespace glass {
constexpr DWORD Magic=0x4c474531, Version=1;
constexpr wchar_t ThreadClass[]=L"LiquidGlass.Engine.Thread.v1";
constexpr wchar_t AttachMessage[]=L"LiquidGlass.Engine.Attach.v1";
constexpr wchar_t RefreshMessage[]=L"LiquidGlass.Engine.Refresh.v1";
constexpr wchar_t StatusMessage[]=L"LiquidGlass.Engine.Status.v1";
constexpr CLSID TapId={0x973361c7,0xcbe9,0x4da4,{0x9c,0x96,0x3b,0x70,0x18,0x3b,0xa1,0x61}};
enum Feature:DWORD {Volume=1,Brightness=2,Taskbar=4,Start=8,Notifications=16,Explorer=32};
struct Config {
    DWORD magic=Magic,version=Version,owner=0;volatile LONG sequence=0;
    DWORD features=0,opacity=35,radius=12,tint=0;BOOL economy=FALSE;
};
inline std::wstring mappingName(DWORD owner){return L"Local\\LiquidGlass.Engine.Config."+std::to_wstring(owner);}
inline DWORD bounded(DWORD n,DWORD lo,DWORD hi){return n<lo?lo:n>hi?hi:n;}
inline Config normalize(Config c){c.features&=63;c.opacity=bounded(c.opacity,5,90);c.radius=bounded(c.radius,0,28);c.tint=bounded(c.tint,0,2);return c;}
// Names only identify material surfaces. No width, height, padding, alignment,
// taskbar position or system work-area property is ever written by this engine.
enum Surface {NoSurface,TaskbarFill,OsdPlate,VolumePlate,BrightnessPlate,StartPlate,StartOverlay,NotificationPlate,ExplorerPlate};
inline Surface identify(const std::wstring&name,const std::wstring&type,int host){
    if(host==0){
        if(name==L"BackgroundFill"&&type.find(L"Rectangle")!=std::wstring::npos)return TaskbarFill;
        if(name==L"ConfirmatorMainGrid")return OsdPlate;
        if(name==L"VolumeConfirmator")return VolumePlate;
        if(name==L"BrightnessConfirmator")return BrightnessPlate;
    }
    if(host==1){if(name==L"AcrylicOverlay")return StartOverlay;if(name==L"AcrylicBorder"||name==L"AppBorder")return StartPlate;}
    if(host==2&&(name==L"NotificationCenterGrid"||name==L"CalendarCenterGrid"||name==L"ControlCenterGrid"))return NotificationPlate;
    if(host==3&&(name==L"CommandBarControlRootGrid"||name==L"FileExplorerCommandBar"||name==L"NavigationBarControlGrid"||name==L"HomeViewRootGrid"||name==L"GalleryRootGrid"||name==L"DetailsViewControlRootGrid"||name==L"GalleryViewControl"))return ExplorerPlate;
    return NoSurface;
}
inline bool active(Surface s,DWORD f){switch(s){case TaskbarFill:return (f&Taskbar)!=0;case OsdPlate:return (f&(Volume|Brightness))!=0;case VolumePlate:return (f&Volume)!=0;case BrightnessPlate:return (f&Brightness)!=0;case StartPlate:case StartOverlay:return (f&Start)!=0;case NotificationPlate:return (f&Notifications)!=0;case ExplorerPlate:return (f&Explorer)!=0;default:return false;}}
}
