#pragma once
#include <chrono>
#include <cmath>
namespace liquid {
inline float dockWeight(float x,float y,float width,float height){if(width<=0||height<=0||y<0||y>height)return 0;float d=std::abs(x-width/2)/std::max(48.f,width*2);return d<1?(1-d*d)*(1-d*d):0;}
inline bool testDockWeight(){return dockWeight(24,24,48,48)==1&&dockWeight(24,-1,48,48)==0&&dockWeight(24,49,48,48)==0&&dockWeight(120,24,48,48)==0&&dockWeight(72,24,48,48)>0&&dockWeight(72,24,48,48)<1&&dockWeight(0,0,0,0)==0;}
// Pointer events schedule short compositor animations. No rendering callback,
// global mouse hook, polling thread, or idle animation timer.
class DockAnimation {
    using Element=winrt::Windows::UI::Xaml::FrameworkElement;
    using Visual=winrt::Windows::UI::Composition::Visual;
    struct Icon {winrt::weak_ref<Element> element;winrt::Windows::Foundation::Numerics::float3 scale,center;bool changed=false;float target=1;winrt::event_token entered{},moved{},exited{};};
    winrt::weak_ref<Element> root;std::vector<Icon> icons;winrt::event_token moved{},exited{};bool attached=false;
    static Visual visual(Element const&e){return winrt::Windows::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(e);}
    void animate(Icon&i,Element const&e,float scale){
        if(std::abs(i.target-scale)<.015f)return;auto v=visual(e);if(!i.changed){i.scale=v.Scale();i.center=v.CenterPoint();i.changed=true;}
        v.CenterPoint({(float)e.ActualWidth()/2,(float)e.ActualHeight(),0});auto a=v.Compositor().CreateVector3KeyFrameAnimation();a.Duration(std::chrono::milliseconds(150));a.InsertKeyFrame(1,{i.scale.x*scale,i.scale.y*scale,i.scale.z});v.StartAnimation(L"Scale",a);i.target=scale;
    }
    void point(winrt::Windows::UI::Xaml::Input::PointerRoutedEventArgs const&args){try{
        auto r=root.get();bool horizontal=!r||r.ActualWidth()>=r.ActualHeight();
        for(auto i=icons.begin();i!=icons.end();){auto e=i->element.get();if(!e){i=icons.erase(i);continue;}
            // Local pointer coordinates include the capsule's RenderTransform.
            // Do not compare an animated visual's bounds against the old root.
            float f=0;if(horizontal)try{auto p=args.GetCurrentPoint(e).Position();f=dockWeight(p.X,p.Y,(float)e.ActualWidth(),(float)e.ActualHeight());}catch(...){}
            try{animate(*i,e,1+.28f*f);}catch(...){}++i;}}catch(...){}}
public:
    ~DockAnimation(){stop();}
    bool ready()const{return attached&&!icons.empty();}
    bool attach(IInspectable*p){try{auto e=winrt::Windows::Foundation::IInspectable{nullptr};winrt::copy_from_abi(e,p);auto r=e.try_as<Element>();if(!r)return false;if(attached&&root.get()==r)return true;
        // One taskbar thread hosts one root. A second live root is reported rather than silently taking over the first root's icons.
        if(attached&&root.get())return false;
        if(attached)stop();root=r;
        moved=r.PointerMoved([this](auto const&,auto const&args){point(args);});
        exited=r.PointerExited([this](auto const&,auto const&args){point(args);});attached=true;return true;
    }catch(...){return false;}}
    void add(IInspectable*p){try{winrt::Windows::Foundation::IInspectable obj{nullptr};winrt::copy_from_abi(obj,p);auto e=obj.try_as<Element>();if(!e)return;for(auto&i:icons)if(i.element.get()==e)return;if(icons.size()>=256)return;
        icons.push_back({winrt::make_weak(e),{1,1,1},{0,0,0},false,1});auto&i=icons.back();
        // Buttons can handle pointer movement before it reaches the taskbar root.
        try{i.entered=e.PointerEntered([this](auto const&,auto const&args){point(args);});i.moved=e.PointerMoved([this](auto const&,auto const&args){point(args);});i.exited=e.PointerExited([this](auto const&,auto const&args){point(args);});}
        catch(...){try{e.PointerEntered(i.entered);}catch(...){}try{e.PointerMoved(i.moved);}catch(...){}try{e.PointerExited(i.exited);}catch(...){}icons.pop_back();throw;}
    }catch(...){}}
    void stop(){try{if(auto r=root.get()){if(attached){r.PointerMoved(moved);r.PointerExited(exited);}}}catch(...){}attached=false;for(auto&i:icons)if(auto e=i.element.get()){
        try{e.PointerEntered(i.entered);}catch(...){}try{e.PointerMoved(i.moved);}catch(...){}try{e.PointerExited(i.exited);}catch(...){}
        if(i.changed)try{auto v=visual(e);v.StopAnimation(L"Scale");v.Scale(i.scale);v.CenterPoint(i.center);}catch(...){}
    }
        icons.clear();root=nullptr;}
};
}
