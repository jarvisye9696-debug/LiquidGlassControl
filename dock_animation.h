#pragma once
#include <chrono>
#include <cmath>
namespace liquid {
// Pointer events schedule short compositor animations. No rendering callback,
// global mouse hook, polling thread, or idle animation timer.
class DockAnimation {
    using Element=winrt::Windows::UI::Xaml::FrameworkElement;
    using Visual=winrt::Windows::UI::Composition::Visual;
    struct Icon {winrt::weak_ref<Element> element;winrt::Windows::Foundation::Numerics::float3 scale,center;bool changed=false;float target=1;};
    winrt::weak_ref<Element> root;std::vector<Icon> icons;winrt::event_token moved{},exited{};bool attached=false;
    static Visual visual(Element const&e){return winrt::Windows::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(e);}
    void animate(Icon&i,Element const&e,float scale){
        if(std::abs(i.target-scale)<.015f)return;i.target=scale;auto v=visual(e);if(!i.changed){i.scale=v.Scale();i.center=v.CenterPoint();i.changed=true;}
        v.CenterPoint({(float)e.ActualWidth()/2,(float)e.ActualHeight(),0});auto a=v.Compositor().CreateVector3KeyFrameAnimation();a.Duration(std::chrono::milliseconds(150));a.InsertKeyFrame(1,{i.scale.x*scale,i.scale.y*scale,i.scale.z});v.StartAnimation(L"Scale",a);
    }
    void point(float x){try{auto r=root.get();if(!r)return;
        // Vertical taskbars stack icons along Y; a horizontal distance model would scale the wrong icons, so they rest at 1.
        bool horizontal=r.ActualWidth()>=r.ActualHeight();
        for(auto i=icons.begin();i!=icons.end();){auto e=i->element.get();if(!e){i=icons.erase(i);continue;}
            float f=0;if(horizontal)try{auto p=e.TransformToVisual(r).TransformPoint({(float)e.ActualWidth()/2,0});float d=std::abs(p.X-x)/110.f;f=d<1?(1-d*d)*(1-d*d):0;}catch(...){}
            animate(*i,e,1+.28f*f);++i;}}catch(...){}}
public:
    ~DockAnimation(){stop();}
    bool ready()const{return attached&&!icons.empty();}
    bool attach(IInspectable*p){try{auto e=winrt::Windows::Foundation::IInspectable{nullptr};winrt::copy_from_abi(e,p);auto r=e.try_as<Element>();if(!r)return false;if(attached&&root.get()==r)return true;if(attached)stop();root=r;
        moved=r.PointerMoved([this](auto const&,auto const&args){if(auto r=root.get())point(args.GetCurrentPoint(r).Position().X);});
        exited=r.PointerExited([this](auto const&,auto const&){for(auto&i:icons)if(auto e=i.element.get())try{animate(i,e,1);}catch(...){}});attached=true;return true;
    }catch(...){return false;}}
    void add(IInspectable*p){try{winrt::Windows::Foundation::IInspectable obj{nullptr};winrt::copy_from_abi(obj,p);auto e=obj.try_as<Element>();if(!e)return;for(auto&i:icons)if(i.element.get()==e)return;if(icons.size()<256)icons.push_back({winrt::make_weak(e),{1,1,1},{0,0,0},false,1});}catch(...){}}
    void stop(){try{if(auto r=root.get()){if(attached){r.PointerMoved(moved);r.PointerExited(exited);}}}catch(...){}attached=false;for(auto&i:icons)if(i.changed)try{if(auto e=i.element.get()){auto v=visual(e);v.StopAnimation(L"Scale");v.Scale(i.scale);v.CenterPoint(i.center);}}catch(...){}icons.clear();root=nullptr;}
};
}
