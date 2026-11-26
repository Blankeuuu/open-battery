#include "../src/hid.hpp"
#include <cstdio>
int main(){
    GUID guid;HidD_GetHidGuid(&guid);HDEVINFO list=SetupDiGetClassDevsW(&guid,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if(list==INVALID_HANDLE_VALUE)return 2;
    for(DWORD i=0;;++i){SP_DEVICE_INTERFACE_DATA iface{};iface.cbSize=sizeof(iface);if(!SetupDiEnumDeviceInterfaces(list,nullptr,&guid,i,&iface))break;
        DWORD n=0;SetupDiGetDeviceInterfaceDetailW(list,&iface,nullptr,0,&n,nullptr);if(!n)continue;std::vector<BYTE> storage(n);auto detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());detail->cbSize=sizeof(*detail);
        if(!SetupDiGetDeviceInterfaceDetailW(list,&iface,detail,n,nullptr,nullptr))continue;
        if(std::wstring(detail->DevicePath).find(L"vid_3151&pid_402d")==std::wstring::npos)continue;
        HANDLE h=CreateFileW(detail->DevicePath,0,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);if(h==INVALID_HANDLE_VALUE)continue;
        PHIDP_PREPARSED_DATA pp=nullptr;HIDP_CAPS caps{};if(HidD_GetPreparsedData(h,&pp)){HidP_GetCaps(pp,&caps);HidD_FreePreparsedData(pp);}CloseHandle(h);
        printf("collection page=%04x usage=%04x input=%u output=%u feature=%u\n",caps.UsagePage,caps.Usage,caps.InputReportByteLength,caps.OutputReportByteLength,caps.FeatureReportByteLength);
        if(caps.UsagePage!=0xffff || caps.Usage!=2 || caps.FeatureReportByteLength!=65)continue;
        h=CreateFileW(detail->DevicePath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);if(h==INVALID_HANDLE_VALUE)continue;
        BYTE request[65]{};request[1]=0xf7;if(HidD_SetFeature(h,request,65)){Sleep(50);
            for(BYTE id:{BYTE(0),BYTE(5)}){BYTE reply[65]{};reply[0]=id;BOOL ok=HidD_GetFeature(h,reply,65);
                printf("GET feature id=%u ok=%d error=%lu status=",id,ok,ok?0:GetLastError());if(ok)for(int j=0;j<8;++j)printf("%02x ",reply[j]);puts("");SecureZeroMemory(reply,sizeof(reply));}
        }CloseHandle(h);
    }SetupDiDestroyDeviceInfoList(list);
}
