#pragma once
// SPDX-License-Identifier: GPL-3.0-only
// Theme data is adapted from the four m417z Styler LiquidGlass presets.
// Selector matching below is a standalone adapter, not the Windhawk runtime.
#include <string>
#include <vector>
namespace imported_glass {
struct ThemeTargetStyles{const wchar_t*target;std::vector<const wchar_t*>styles;};
struct Theme{std::vector<ThemeTargetStyles>targets;std::vector<const wchar_t*>constants;};
#include "third_party/windhawk/Taskbar-LiquidGlass.h"
#include "third_party/windhawk/Start-LiquidGlass.h"
#include "third_party/windhawk/Notifications-LiquidGlass.h"
#include "third_party/windhawk/Explorer-LiquidGlass.h"
inline Theme const&theme(int host){return host==1?Start:host==2?Notifications:host==3?Explorer:Taskbar;}
inline std::wstring trim(std::wstring value){auto a=value.find_first_not_of(L" \t\r\n"),b=value.find_last_not_of(L" \t\r\n");return a==value.npos?L"":value.substr(a,b-a+1);}
inline std::wstring shortType(std::wstring value){auto i=value.find_last_of(L'.');return i==value.npos?value:value.substr(i+1);}
struct Element{std::wstring type,name;};
inline bool token(std::wstring selector,Element const&e){selector=trim(selector);if(selector.find(L'[')!=selector.npos)return false;auto state=selector.find(L'@');if(state!=selector.npos)selector.resize(state);auto hash=selector.find(L'#');auto type=hash==selector.npos?selector:selector.substr(0,hash);auto name=hash==selector.npos?L"":selector.substr(hash+1);return !type.empty()&&(type==e.type||shortType(type)==shortType(e.type))&&(hash==selector.npos||name==e.name);}
inline bool matches(std::wstring selector,std::vector<Element>const&ancestry){
    size_t start=0;do{auto end=selector.find(L',',start);auto branch=selector.substr(start,end==selector.npos?end:end-start);bool match=true;size_t depth=0;
        while(true){auto separator=branch.find_last_of(L'>');auto part=separator==branch.npos?branch:branch.substr(separator+1);if(depth>=ancestry.size()||!token(part,ancestry[depth++])){match=false;break;}if(separator==branch.npos)break;branch.resize(separator);}
        if(match)return true;if(end==selector.npos)break;start=end+1;
    }while(start<selector.size());return false;
}
inline bool candidate(Theme const&t,Element const&e){for(auto const&r:t.targets){std::wstring selector=r.target;size_t start=0;do{auto end=selector.find(L',',start);auto branch=selector.substr(start,end==selector.npos?end:end-start);auto parent=branch.find_last_of(L'>');if(token(parent==branch.npos?branch:branch.substr(parent+1),e))return true;if(end==selector.npos)break;start=end+1;}while(start<selector.size());}return false;}
inline std::vector<const wchar_t*>styles(Theme const&t,std::vector<Element>const&ancestry){std::vector<const wchar_t*>result;for(auto const&r:t.targets)if(matches(r.target,ancestry))for(auto s:r.styles)result.push_back(s);return result;}
inline bool assignment(std::wstring text,std::wstring&property,std::wstring&value){auto equal=text.find(L'=');if(equal==text.npos)return false;property=trim(text.substr(0,equal));if(!property.empty()&&property.back()==L':')property.pop_back();property=trim(property);value=trim(text.substr(equal+1));return !property.empty();}
inline std::wstring resolve(Theme const&t,std::wstring value){if(value.empty()||value[0]!=L'$')return value;auto key=value.substr(1);for(auto c:t.constants){std::wstring name,text;if(assignment(c,name,text)&&name==key)return text;}return value;}
inline bool test(){
    std::vector<Element>path={{L"Windows.UI.Xaml.Controls.Grid",L"RootGrid"},{L"Taskbar.TaskbarFrame",L""}};
    if(!matches(L"Taskbar.TaskbarFrame > Grid#RootGrid",path)||matches(L"Border#RootGrid",path)||matches(L"Grid#Other",path))return false;
    auto found=styles(Taskbar,path);bool background=false;
    for(auto s:found){std::wstring p,v;if(assignment(s,p,v)&&p==L"Background")background=resolve(Taskbar,v).find(L"<WindhawkBlur")==0;}
    if(!background||resolve(Explorer,L"$ElementBackground").find(L"<WindhawkBlur")!=0)return false;
    // Imported material presets must never alter shell geometry or visibility.
    for(int host=0;host<4;++host)for(auto const&r:theme(host).targets)for(auto s:r.styles){std::wstring p,v;if(!assignment(s,p,v))return false;if(p!=L"Background"&&p!=L"Fill"&&p!=L"BorderBrush"&&p!=L"BorderThickness"&&p!=L"CornerRadius"&&p!=L"Stroke"&&p!=L"StrokeThickness")return false;}
    return true;
}
}
