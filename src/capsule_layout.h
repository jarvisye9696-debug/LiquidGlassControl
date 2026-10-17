#pragma once
#include <winrt/Windows.Foundation.Collections.h>
namespace liquid {
struct CapsulePlacement {double left=0,right=0,apps=0,tray=0;bool valid=false;};
inline CapsulePlacement placeCapsule(double width,double appLeft,double appRight,double trayLeft,double trayRight){
    double apps=appRight-appLeft,tray=trayRight-trayLeft,total=apps+8+tray;
    if(apps<=0||tray<=0||total>width-16)return {};
    double left=(width-total)/2;return {left,left+total,left-appLeft,left+apps+8-trayLeft,true};
}
// XAML transforms move both visuals and hit testing. Never change the native
// taskbar window, monitor work area, or the user's taskbar alignment preference.
class CapsuleLayout {
    using Element=winrt::Windows::UI::Xaml::FrameworkElement;
    using Transform=winrt::Windows::UI::Xaml::Media::Transform;
    using Group=winrt::Windows::UI::Xaml::Media::TransformGroup;
    using Translation=winrt::Windows::UI::Xaml::Media::TranslateTransform;
    struct Target {winrt::weak_ref<Element> element;Transform original{nullptr};Group group{nullptr};Translation shift{nullptr};bool used=false;};
    std::vector<Target> targets;
    static void restore(Target&t){try{if(auto e=t.element.get())if(e.RenderTransform()==t.group)e.RenderTransform(t.original);}catch(...){}}
    Target&target(Element const&e){for(auto&t:targets)if(t.element.get()==e){t.used=true;return t;}
        Target t;t.element=winrt::make_weak(e);t.original=e.RenderTransform();t.group=Group();t.shift=Translation();if(t.original)t.group.Children().Append(t.original);t.group.Children().Append(t.shift);t.used=true;targets.push_back(std::move(t));return targets.back();}
    void move(Element const&e,double x){auto&t=target(e);auto current=e.RenderTransform();if(current!=t.group&&current!=t.original)throw winrt::hresult_error(E_ABORT);
        if(std::abs(t.shift.X()-x)>.25)t.shift.X(x);if(current!=t.group)e.RenderTransform(t.group);}
    double offset(Element const&e){for(auto&t:targets)if(t.element.get()==e&&e.RenderTransform()==t.group)return t.shift.X();return 0;}
public:
    ~CapsuleLayout(){stop();}
    void stop(){for(auto&t:targets)restore(t);targets.clear();}
    CapsulePlacement apply(std::vector<Element>const&icons,Element const&tray,Element const&host){try{
        if(icons.empty()||!tray||!host||host.ActualWidth()<host.ActualHeight()){stop();return {};}
        double left=host.ActualWidth(),right=0;for(auto const&e:icons){auto p=e.TransformToVisual(host).TransformPoint({0,0});double x=p.X-offset(e);left=std::min(left,x);right=std::max(right,x+e.ActualWidth());}
        auto p=tray.TransformToVisual(host).TransformPoint({0,0});double trayLeft=p.X-offset(tray);auto plan=placeCapsule(host.ActualWidth(),left,right,trayLeft,trayLeft+tray.ActualWidth());if(!plan.valid){stop();return {};}
        for(auto&t:targets)t.used=false;for(auto const&e:icons)move(e,plan.apps);move(tray,plan.tray);
        for(auto i=targets.begin();i!=targets.end();)if(!i->used){restore(*i);i=targets.erase(i);}else ++i;
        return plan;
    }catch(...){stop();return {};}}
};
inline bool testCapsulePlacement(){auto p=placeCapsule(1920,550,1350,1600,1900);return p.valid&&p.left==406&&p.right==1514&&550+p.apps==406&&1600+p.tray==1214&&1214-(1350+p.apps)==8&&!placeCapsule(900,0,800,800,1000).valid&&!placeCapsule(1920,550,550,1600,1900).valid;}
}
