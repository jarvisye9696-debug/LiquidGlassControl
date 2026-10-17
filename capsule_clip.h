#pragma once
namespace liquid {
// StackPanel has no CornerRadius property. Clip its composition visual without
// changing layout, hit targets, or taskbar window bounds. Restore the exact clip.
inline std::function<void()> clipCapsule(IInspectable*p){
    using Element=winrt::Windows::UI::Xaml::FrameworkElement;
    winrt::Windows::Foundation::IInspectable obj{nullptr};winrt::copy_from_abi(obj,p);auto e=obj.try_as<Element>();if(!e)return {};
    auto visual=winrt::Windows::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(e);auto original=visual.Clip();auto geometry=visual.Compositor().CreateRoundedRectangleGeometry();auto clip=visual.Compositor().CreateGeometricClip(geometry);
    auto update=[geometry](Element const&element){float w=(float)element.ActualWidth(),h=(float)element.ActualHeight(),vertical=std::min(h/4,std::max(6.f,(h-36.f)/2)),horizontal=std::min(4.f,w/4);geometry.Offset({horizontal,vertical});geometry.Size({std::max(0.f,w-2*horizontal),std::max(0.f,h-2*vertical)});float r=std::min(std::max(0.f,w-2*horizontal),std::max(0.f,h-2*vertical))/2;geometry.CornerRadius({r,r});};update(e);
    auto token=e.SizeChanged([update](auto const&sender,auto const&){try{update(sender.template as<Element>());}catch(...){}});
    try{visual.Clip(clip);}catch(...){e.SizeChanged(token);throw;}
    auto weak=winrt::make_weak(e);
    return [weak,original,clip,token]{try{if(auto element=weak.get()){element.SizeChanged(token);auto v=winrt::Windows::UI::Xaml::Hosting::ElementCompositionPreview::GetElementVisual(element);if(v.Clip()==clip)v.Clip(original);}}catch(...){}};
}
}
