import fs from 'node:fs';
import path from 'node:path';
import {createRequire} from 'node:module';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const require=createRequire(path.join(root,'.tools/icon-tools/package.json'));
const {PNG}=require('pngjs'),{Resvg}=require('@resvg/resvg-js');
function png(file){const b=fs.readFileSync(file),size=b.readInt32LE(18),offset=b.readUInt32LE(10);const p=new PNG({width:size,height:size});
 for(let i=0;i<size*size;i++){const q=offset+i*4,a=b[q+3];p.data[i*4+3]=a;p.data[i*4]=a?Math.min(255,Math.round(b[q+2]*255/a)):0;p.data[i*4+1]=a?Math.min(255,Math.round(b[q+1]*255/a)):0;p.data[i*4+2]=a?Math.min(255,Math.round(b[q]*255/a)):0;}return p;}
function encoded(p,zoom=1){if(zoom===1)return PNG.sync.write(p).toString('base64');const z=new PNG({width:p.width*zoom,height:p.height*zoom});for(let y=0;y<z.height;y++)for(let x=0;x<z.width;x++){const q=(Math.floor(y/zoom)*p.width+Math.floor(x/zoom))*4;p.data.copy(z.data,(y*z.width+x)*4,q,q+4);}return PNG.sync.write(z).toString('base64');}
for(const theme of ['dark','light']){
 const bg=theme==='dark'?'#25282c':'#f0f0f0',fg=theme==='dark'?'#eeeeee':'#202124';
 let s=`<svg xmlns="http://www.w3.org/2000/svg" width="760" height="460"><rect width="760" height="460" fill="${bg}"/><g font-family="Segoe UI" fill="${fg}"><text x="24" y="34" font-size="20">Open Battery 0.2.2 · kolor urządzenia w okręgu</text><text x="24" y="60" font-size="13">Win32/GDI · jeden wskaźnik na urządzenie · wartości przykładowe</text>`;
 for(const [row,size] of [16,20,24,32].entries()){
  const y=96+row*54;s+=`<text x="24" y="${y+15}" font-size="13">${size} px</text>`;
  for(let k=0;k<3;k++)for(const [part,role] of ['symbol'].entries()){
   const p=png(path.join(root,'build/ring-icons',`${theme}-${size}-${k}-${role}.bmp`));
   s+=`<image x="${130+k*190+part*(size+6)}" y="${y}" width="${size}" height="${size}" href="data:image/png;base64,${encoded(p)}"/>`;
   if(size===16)s+=`<image x="${130+k*190+part*70}" y="355" width="64" height="64" href="data:image/png;base64,${encoded(p,4)}"/>`;
  }
 }
 s+='<text x="24" y="378" font-size="13">16 px ×4</text><text x="24" y="443" font-size="12">Okrąg: tożsamość urządzenia. Cyfry: poziom baterii. Kolor okręgu wybierasz w Devices.</text></g></svg>';
 fs.writeFileSync(path.join(root,'preview',`native-ring-${theme}.png`),new Resvg(s).render().asPng());
}
console.log('Saved preview/native-ring-dark.png and native-ring-light.png');
