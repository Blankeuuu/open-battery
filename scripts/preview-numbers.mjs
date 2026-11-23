import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const require=createRequire(path.join(root,'.tools/icon-tools/package.json'));
const {PNG}=require('pngjs'),{Resvg}=require('@resvg/resvg-js');
const values=[100,70,69,30,29,0];
for(const theme of ['dark','light']){
 const bg=theme==='dark'?'#202020':'#f3f3f3',fg=theme==='dark'?'white':'#111111';
 let svg='<svg xmlns="http://www.w3.org/2000/svg" width="680" height="340"><rect width="680" height="340" fill="'+bg+'"/><g fill="'+fg+'" font-family="Segoe UI" font-size="14"><text x="20" y="28">JetBrains Mono Bold — '+theme+' — actual native renderer</text>';
 [16,20,24,32].forEach((size,row)=>{
  const y=52+row*44;svg+='<text x="20" y="'+(y+size-2)+'">'+size+' px</text>';
  values.forEach((value,col)=>{
   const b=fs.readFileSync(path.join(root,'build/numbers',theme+'-'+size+'-'+value+'.bmp'));
   const offset=b.readUInt32LE(10),w=b.readInt32LE(18),h=Math.abs(b.readInt32LE(22));
   const png=new PNG({width:w,height:h});
   for(let i=0;i<w*h;++i){
    const p=offset+i*4,a=b[p+3];png.data[i*4+3]=a;
    png.data[i*4]=a?Math.min(255,Math.round(b[p+2]*255/a)):0;
    png.data[i*4+1]=a?Math.min(255,Math.round(b[p+1]*255/a)):0;
    png.data[i*4+2]=a?Math.min(255,Math.round(b[p]*255/a)):0;
   }
   const data=PNG.sync.write(png).toString('base64');
   svg+='<image x="'+(110+col*86)+'" y="'+y+'" width="'+size+'" height="'+size+'" href="data:image/png;base64,'+data+'"/>';
   if(size===16){
    const zoom=new PNG({width:64,height:64});
    for(let yy=0;yy<64;yy++)for(let xx=0;xx<64;xx++){
      const q=(Math.floor(yy/4)*16+Math.floor(xx/4))*4;
      png.data.copy(zoom.data,(yy*64+xx)*4,q,q+4);
    }
    svg+='<image x="'+(110+col*86)+'" y="252" width="64" height="64" href="data:image/png;base64,'+PNG.sync.write(zoom).toString('base64')+'"/>';
   }
  });
 });
 svg+='<text x="20" y="280">16 px ×4</text></g></svg>';
 fs.writeFileSync(path.join(root,'build/numbers/preview-'+theme+'.png'),new Resvg(svg).render().asPng());
}
