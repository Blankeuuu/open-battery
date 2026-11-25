#pragma once
#include "number-icon.hpp"
#include "devices.hpp"
namespace ob {
inline HICON deviceIcon(const Device& device,int size,bool dark,bool symbolOnly=false){
    size=std::max(16,size);const int scale=4,w=size*scale;
    std::vector<DWORD> pixels(w*w,0);
    COLORREF gray=dark?RGB(212,212,212):RGB(82,82,82);
    auto colorPixel=[](COLORREF c,int a){return DWORD((a<<24)|((GetRValue(c)*a/255)<<16)|((GetGValue(c)*a/255)<<8)|(GetBValue(c)*a/255));};
    auto cell=[&](double x,double y,double width,double height,COLORREF c){
        int left=int(x*w/16),top=int(y*w/16),right=int((x+width)*w/16),bottom=int((y+height)*w/16);
        for(int yy=std::max(0,top);yy<std::min(w,bottom);++yy)for(int xx=std::max(0,left);xx<std::min(w,right);++xx)pixels[yy*w+xx]=colorPixel(c,255);
    };
    const char* mouse[]={"0011100","0101010","0101010","0100010","0100010","0011100"};
    const char* pad[]={"001111100","011111110","110101011","111111111","110000011","100000001"};
    const char* headphones[]={"001111100","010000010","100000001","110000011","110000011","110000011"};
    const char* generic[]={"0111110","0100010","0100010","0100010","0100010","0111110"};
    const char** glyph=device.kind==DeviceKind::mouse?mouse:device.kind==DeviceKind::controller?pad:device.kind==DeviceKind::headphones?headphones:generic;
    int gw=device.kind==DeviceKind::mouse || device.kind==DeviceKind::other?7:9;
    const double unit=symbolOnly?1.5:1.0,yUnit=symbolOnly?1.8:1.0;
    COLORREF glyphColor=device.reading.chargingKnown && device.reading.charging?batteryColor(100,dark):gray;
    for(int y=0;y<6;++y)for(int x=0;x<gw;++x)if(glyph[y][x]=='1')cell((16-gw*unit)/2.0+x*unit,(symbolOnly?3.0:0)+y*yUnit,unit,yUnit,glyphColor);
    if(device.badge){
        static const char* digits[]={"111101101101111","010110010010111","111001111100111","111001111001111","101101111001001","111100111001111","111100111101111","111001001001001","111101111101111","111101111001111"};
        const char* d=digits[std::min(device.badge,9u)];for(int y=0;y<5;++y)for(int x=0;x<3;++x)if(d[y*3+x]=='1')cell(x,y,1,1,gray);
    }
    if(device.reading.chargingKnown && device.reading.charging){cell(14,0,2,1,gray);cell(13,1,2,2,gray);cell(14,3,2,1,gray);cell(13,4,2,1,gray);cell(13,5,1,1,gray);}
    std::wstring text;
    switch(device.reading.state){
    case State::reading:text=std::to_wstring(device.reading.percent);break;
    case State::inactive:text=L"z";break;
    case State::disconnected:text=L"x";break;
    case State::unsupported:text=L"?";break;
    default:text=L"!";break;
    }
    COLORREF color=device.reading.state==State::reading?batteryColor(device.reading.percent,dark):gray;
    HICON number=symbolOnly?nullptr:numberIcon(text.c_str(),RGB(255,255,255),w);
    if(number){
        ICONINFO ii{};GetIconInfo(number,&ii);HDC dc=CreateCompatibleDC(nullptr);
        BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-w;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
        std::vector<DWORD> source(w*w);GetDIBits(dc,ii.hbmColor,0,w,source.data(),&bi,DIB_RGB_COLORS);
        int left=w,right=0,top=w,bottom=0;
        for(int y=0;y<w;++y)for(int x=0;x<w;++x)if(source[y*w+x]>>24){left=std::min(left,x);right=std::max(right,x+1);top=std::min(top,y);bottom=std::max(bottom,y+1);}
        if(right>left && bottom>top){
            const int height=w*9/16,maxWidth=w*14/16;
            int width=std::min(maxWidth,(right-left)*height/(bottom-top));width=std::max(width,1);
            int x0=(w-width)/2,y0=w*7/16;
            for(int y=0;y<height;++y)for(int x=0;x<width;++x){
                int sx=left+x*(right-left)/width,sy=top+y*(bottom-top)/height;
                pixels[(y0+y)*w+x0+x]=colorPixel(color,source[sy*w+sx]>>24);
            }
        }
        DeleteObject(ii.hbmColor);DeleteObject(ii.hbmMask);DeleteDC(dc);DestroyIcon(number);
    }
    std::vector<DWORD> small(size*size);
    for(int y=0;y<size;++y)for(int x=0;x<size;++x){unsigned a=0,r=0,g=0,b=0;
        for(int yy=0;yy<scale;++yy)for(int xx=0;xx<scale;++xx){DWORD p=pixels[(y*scale+yy)*w+x*scale+xx];a+=p>>24;r+=(p>>16)&255;g+=(p>>8)&255;b+=p&255;}
        small[y*size+x]=((a/16)<<24)|((r/16)<<16)|((g/16)<<8)|(b/16);
    }
    HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=size;bi.bmiHeader.biHeight=-size;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    void* raw=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&raw,nullptr,0);HICON result=nullptr;
    if(bitmap){memcpy(raw,small.data(),small.size()*4);std::vector<BYTE> maskBits(((size+15)/16)*2*size,0);HBITMAP mask=CreateBitmap(size,size,1,1,maskBits.data());ICONINFO ii{};ii.fIcon=TRUE;ii.hbmColor=bitmap;ii.hbmMask=mask;result=CreateIconIndirect(&ii);DeleteObject(mask);DeleteObject(bitmap);}
    DeleteDC(dc);return result;
}
}
