#pragma once
#include <windows.h>
#include <vector>
namespace xaml_subscription {
// Opt out of the optional composition-diagnostics walk while subscribing to
// XAML visual mutations. Some shell builds abort that optional walk with
// E_UNEXPECTED. This affects only these registry reads on this worker thread;
// it does not write a registry value or change system/security settings.
inline thread_local bool subscribing=false;
inline SRWLOCK patchLock=SRWLOCK_INIT;
inline LSTATUS WINAPI openKey(HKEY key,LPCWSTR sub,DWORD options,REGSAM access,PHKEY result){
    auto status=RegOpenKeyExW(key,sub,options,access,result);
    if(subscribing&&(status==ERROR_FILE_NOT_FOUND||status==ERROR_PATH_NOT_FOUND)&&key==HKEY_LOCAL_MACHINE&&sub&&_wcsicmp(sub,L"Software\\Microsoft\\XAML\\Debug")==0)
        return RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"Software\\Microsoft",options,access,result);
    return status;
}
inline LSTATUS WINAPI queryValue(HKEY key,LPCWSTR name,LPDWORD reserved,LPDWORD type,LPBYTE data,LPDWORD bytes){
    if(!subscribing||!name||_wcsicmp(name,L"DisableCompositionDiag")!=0)return RegQueryValueExW(key,name,reserved,type,data,bytes);
    if(data&&!bytes)return ERROR_INVALID_PARAMETER;if(type)*type=REG_DWORD;
    if(data&&*bytes<sizeof(DWORD)){*bytes=sizeof(DWORD);return ERROR_MORE_DATA;}
    if(data){DWORD value=1;memcpy(data,&value,sizeof(value));}if(bytes)*bytes=sizeof(DWORD);return ERROR_SUCCESS;
}
class Scope {
    struct Slot {void**address;void*original;void*replacement;};std::vector<Slot> slots;
    void patch(HMODULE module){if(!module)return;auto base=(BYTE*)module;auto dos=(IMAGE_DOS_HEADER*)base;if(dos->e_magic!=IMAGE_DOS_SIGNATURE)return;auto nt=(IMAGE_NT_HEADERS*)(base+dos->e_lfanew);if(nt->Signature!=IMAGE_NT_SIGNATURE)return;
        auto rva=nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;if(!rva)return;
        for(auto entry=(IMAGE_IMPORT_DESCRIPTOR*)(base+rva);entry->Name;++entry){if(!entry->OriginalFirstThunk)continue;auto names=(IMAGE_THUNK_DATA*)(base+entry->OriginalFirstThunk),addresses=(IMAGE_THUNK_DATA*)(base+entry->FirstThunk);
            for(;names->u1.AddressOfData;++names,++addresses){if(IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))continue;auto name=(const char*)((IMAGE_IMPORT_BY_NAME*)(base+names->u1.AddressOfData))->Name;
                void*replacement=strcmp(name,"RegOpenKeyExW")==0?(void*)&openKey:strcmp(name,"RegQueryValueExW")==0?(void*)&queryValue:nullptr;if(!replacement)continue;
                if(slots.size()==slots.capacity())return;auto slot=(void**)&addresses->u1.Function;DWORD old=0;if(VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&old)){auto original=InterlockedExchangePointer(slot,replacement);slots.push_back({slot,original,replacement});DWORD ignored=0;VirtualProtect(slot,sizeof(void*),old,&ignored);}
            }
        }
    }
public:
    explicit Scope(IUnknown*service){slots.reserve(16);AcquireSRWLockExclusive(&patchLock);HMODULE module{};if(service){auto function=(*(void***)service)[3];GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCWSTR)function,&module);}patch(module);auto system=GetModuleHandleW(L"Windows.UI.Xaml.dll");if(system!=module)patch(system);subscribing=true;}
    ~Scope(){subscribing=false;for(auto i=slots.rbegin();i!=slots.rend();++i){DWORD old=0;if(VirtualProtect(i->address,sizeof(void*),PAGE_READWRITE,&old)){InterlockedCompareExchangePointer(i->address,i->original,i->replacement);DWORD ignored=0;VirtualProtect(i->address,sizeof(void*),old,&ignored);}}ReleaseSRWLockExclusive(&patchLock);}
    Scope(Scope const&)=delete;Scope&operator=(Scope const&)=delete;
};
}
