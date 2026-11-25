#pragma once
#include "number-icon.hpp"
#include "devices.hpp"
namespace ob {
inline HICON ringIcon(const Device& device,int size,bool dark){
    size=std::max(16,size);const int scale=4,w=size*scale;
    std::vector<DWORD> pixels(w*w,0);
    auto ink=[](COLORREF c,unsigned a){return DWORD((a<<24)|((GetRValue(c)*a/255)<<16)|((GetGValue(c)*a/255)<<8)|(GetBValue(c)*a/255));};
    COLORREF ring=COLORREF(device.identityColor());
    double center=(w-1)/2.0,outer=w/2.0-.25,inner=outer-std::max(4.,w/16.0);
    for(int y=0;y<w;++y)for(int x=0;x<w;++x){double dx=x-center,dy=y-center,distance=std::sqrt(dx*dx+dy*dy);
        if(distance<=outer && distance>=inner)pixels[y*w+x]=ink(ring,255);
    }
    std::wstring text;
    switch(device.reading.state){case State::reading:text=std::to_wstring(device.reading.percent);break;
    case State::inactive:text=L"z";break;case State::disconnected:text=L"x";break;case State::unsupported:text=L"?";break;default:text=L"!";break;}
    COLORREF color=device.reading.state==State::reading?batteryColor(device.reading.percent,dark):dark?RGB(200,200,200):RGB(90,90,90);
    HICON number=numberIcon(text.c_str(),RGB(255,255,255),w);
    if(number){
        ICONINFO ii{};GetIconInfo(number,&ii);HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO bi{};
        bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-w;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
        std::vector<DWORD> source(w*w);GetDIBits(dc,ii.hbmColor,0,w,source.data(),&bi,DIB_RGB_COLORS);
        int left=w,top=w,right=0,bottom=0;for(int y=0;y<w;++y)for(int x=0;x<w;++x)if(source[y*w+x]>>24){left=std::min(left,x);right=std::max(right,x+1);top=std::min(top,y);bottom=std::max(bottom,y+1);}
        if(right>left && bottom>top){
            int height=std::max(1,w*9/16),maxWidth=w*12/16;
            int width=std::max(1,std::min(maxWidth,(right-left)*height/(bottom-top)));
            int x0=(w-width)/2,y0=(w-height)/2;
            for(int y=0;y<height;++y)for(int x=0;x<width;++x){int sx=left+x*(right-left)/width,sy=top+y*(bottom-top)/height;unsigned a=source[sy*w+sx]>>24;
                if(a)pixels[(y0+y)*w+x0+x]=ink(color,a);
            }
        }
        DeleteObject(ii.hbmColor);DeleteObject(ii.hbmMask);DeleteDC(dc);DestroyIcon(number);
    }
    // Known charging is a tiny neutral bolt at the bottom; identity hue never changes.
    if(device.reading.chargingKnown && device.reading.charging){
        COLORREF bolt=dark?RGB(245,245,245):RGB(30,30,30);
        const char* shape[]={"011","110","011","110"};
        for(int y=0;y<4;++y)for(int x=0;x<3;++x)if(shape[y][x]=='1'){
            for(int yy=(12+y)*w/16;yy<(13+y)*w/16;++yy)for(int xx=(6+x)*w/16;xx<(7+x)*w/16;++xx)pixels[yy*w+xx]=ink(bolt,255);
        }
    }
    std::vector<DWORD> output(size*size);for(int y=0;y<size;++y)for(int x=0;x<size;++x){unsigned a=0,r=0,g=0,b=0;
        for(int yy=0;yy<scale;++yy)for(int xx=0;xx<scale;++xx){DWORD p=pixels[(y*scale+yy)*w+x*scale+xx];a+=p>>24;r+=(p>>16)&255;g+=(p>>8)&255;b+=p&255;}
        output[y*size+x]=((a/16)<<24)|((r/16)<<16)|((g/16)<<8)|(b/16);
    }
    HDC dc=CreateCompatibleDC(nullptr);BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=size;bi.bmiHeader.biHeight=-size;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    void* raw=nullptr;HBITMAP bitmap=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&raw,nullptr,0);HICON result=nullptr;
    if(bitmap){memcpy(raw,output.data(),output.size()*4);std::vector<BYTE> bits(((size+15)/16)*2*size,0);HBITMAP mask=CreateBitmap(size,size,1,1,bits.data());ICONINFO ii{};ii.fIcon=TRUE;ii.hbmColor=bitmap;ii.hbmMask=mask;result=CreateIconIndirect(&ii);DeleteObject(mask);DeleteObject(bitmap);}
    DeleteDC(dc);return result;
}
}
