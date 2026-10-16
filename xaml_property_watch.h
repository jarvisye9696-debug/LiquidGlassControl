#pragma once
#include <functional>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include <winrt/Microsoft.UI.Xaml.Controls.h>
#include <winrt/Microsoft.UI.Xaml.Shapes.h>
namespace liquid {
// Keep the override when Windows changes a visual state. Unregister every
// callback on restore, using weak references so hidden surfaces can be freed.
inline std::function<void()> watch(IInspectable*p,bool modern,std::wstring const&name,std::function<void()> changed){
    winrt::Windows::Foundation::IInspectable obj{nullptr};winrt::copy_from_abi(obj,p);
    if(modern){namespace x=winrt::Microsoft::UI::Xaml;namespace c=x::Controls;x::DependencyProperty dp{nullptr};auto d=obj.try_as<x::DependencyObject>();if(!d)return {};
        if(name==L"Background"){if(obj.try_as<c::Panel>())dp=c::Panel::BackgroundProperty();else if(obj.try_as<c::Border>())dp=c::Border::BackgroundProperty();else if(obj.try_as<c::Control>())dp=c::Control::BackgroundProperty();}
        if(name==L"Fill"&&obj.try_as<x::Shapes::Shape>())dp=x::Shapes::Shape::FillProperty();
        if(!dp)return {};auto token=d.RegisterPropertyChangedCallback(dp,[changed](auto const&,auto const&){changed();});auto weak=winrt::make_weak(d);return [weak,dp,token]{try{if(auto v=weak.get())v.UnregisterPropertyChangedCallback(dp,token);}catch(...){}};
    }else{namespace x=winrt::Windows::UI::Xaml;namespace c=x::Controls;x::DependencyProperty dp{nullptr};auto d=obj.try_as<x::DependencyObject>();if(!d)return {};
        if(name==L"Background"){if(obj.try_as<c::Panel>())dp=c::Panel::BackgroundProperty();else if(obj.try_as<c::Border>())dp=c::Border::BackgroundProperty();else if(obj.try_as<c::Control>())dp=c::Control::BackgroundProperty();}
        if(name==L"Fill"&&obj.try_as<x::Shapes::Shape>())dp=x::Shapes::Shape::FillProperty();
        if(!dp)return {};auto token=d.RegisterPropertyChangedCallback(dp,[changed](auto const&,auto const&){changed();});auto weak=winrt::make_weak(d);return [weak,dp,token]{try{if(auto v=weak.get())v.UnregisterPropertyChangedCallback(dp,token);}catch(...){}};
    }
}
}
