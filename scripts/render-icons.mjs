import fs from 'node:fs';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
import {createRequire} from 'node:module';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const require=createRequire(path.join(root,'.tools/icon-tools/package.json'));
const {Resvg}=require('@resvg/resvg-js');
const {PNG}=require('pngjs');
const version=process.argv[2]??'0.1.1';
if(!/^\d+\.\d+\.\d+$/.test(version))throw new Error('Expected major.minor.patch version');
const output=path.join(root,'build/icons');
fs.mkdirSync(output,{recursive:true});
const sizes=[16,20,24,32,40,48,64];
const sources={
  app:'app.svg',sleep:'sleep_32.svg',error:'error_32.svg',
  disconnected:'disconnected_mouse_x.svg',
  mouse_signal:'disconnected_mouse_signal.svg',
  broken_link:'disconnected_broken_link.svg',
  no_signal:'disconnected_no_signal.svg',
};
function render(file,size,tint){
  const svg=fs.readFileSync(path.join(root,'icons',file));
  const rendered=new Resvg(svg,{fitTo:{mode:'width',value:size},font:{loadSystemFonts:false}});
  const pixels=PNG.sync.read(rendered.render().asPng());
  if(pixels.width!==size || pixels.height!==size)throw new Error('Non-square icon: '+file);
  if(tint)for(let i=0;i<pixels.data.length;i+=4){
    pixels.data[i]=tint[0];pixels.data[i+1]=tint[1];pixels.data[i+2]=tint[2];
  } // Preserve rendered alpha, including mask cutouts; do not recolor SVG masks.
  return PNG.sync.write(pixels);
}
function ico(frames){
  const header=Buffer.alloc(6+frames.length*16);
  header.writeUInt16LE(1,2);header.writeUInt16LE(frames.length,4);
  let offset=header.length;
  frames.forEach(({size,data},i)=>{
    const p=6+i*16;header[p]=size===256?0:size;header[p+1]=size===256?0:size;
    header.writeUInt16LE(1,p+4);header.writeUInt16LE(32,p+6);
    header.writeUInt32LE(data.length,p+8);header.writeUInt32LE(offset,p+12);offset+=data.length;
  });
  return Buffer.concat([header,...frames.map(x=>x.data)]);
}
const manifest={sizes,sources,resources:[]};
const appFrames=[...sizes,128,256].map(size=>{
  const data=render(sources.app,size);fs.writeFileSync(path.join(output,'app-'+size+'.png'),data);return{size,data};
});
fs.writeFileSync(path.join(output,'app.ico'),ico(appFrames));
manifest.resources.push({id:100,name:'app',file:'app.ico'});
for(const [theme,tint] of Object.entries({dark:[184,184,184],light:[102,102,102]})){
  for(const [name,id] of Object.entries({sleep:101,disconnected:102,error:103})){
    const frames=sizes.map(size=>{
      const data=render(sources[name],size,tint);
      fs.writeFileSync(path.join(output,name+'-'+theme+'-'+size+'.png'),data);
      return{size,data};
    });
    const file=name+'-'+theme+'.ico';
    fs.writeFileSync(path.join(output,file),ico(frames));
    manifest.resources.push({id:id+(theme==='light'?10:0),name:name+'-'+theme,file});
  }
}
const rc=manifest.resources.map(r=>r.id+' ICON "'+path.join(output,r.file).replaceAll('\\','/')+'"').join('\n');
const numbers=version.split('.').map(Number);
const versionResource='1 VERSIONINFO\\n FILEVERSION '+numbers.join(',')+',0\\n PRODUCTVERSION '+numbers.join(',')+',0\\n FILEOS 0x40004\\n FILETYPE 1\\n BEGIN\\n BLOCK "StringFileInfo"\\n BEGIN\\n BLOCK "040904B0"\\n BEGIN\\n VALUE "FileDescription", "Open Battery\\0"\\n VALUE "FileVersion", "'+version+'\\0"\\n VALUE "ProductName", "Open Battery\\0"\\n VALUE "ProductVersion", "'+version+'\\0"\\n END\\n END\\n BLOCK "VarFileInfo"\\n BEGIN\\n VALUE "Translation", 0x409, 1200\\n END\\n END\\n';
fs.writeFileSync(path.join(root,'build/icons.rc'),rc+'\n'+versionResource.replaceAll('\\n','\n'));
fs.writeFileSync(path.join(root,'build/version.hpp'),'#pragma once\n#define OB_VERSION L"'+version+'"\n');
fs.writeFileSync(path.join(output,'manifest.json'),JSON.stringify(manifest,null,2));
for(const theme of ['dark','light']){
  const bg=theme==='dark'?'#202020':'#f3f3f3',fg=theme==='dark'?'#ffffff':'#111111';
  const rows=Object.keys(sources);
  let svg='<svg xmlns="http://www.w3.org/2000/svg" width="760" height="440"><rect width="760" height="440" fill="'+bg+'"/>';
  svg+='<g fill="'+fg+'" font-family="Segoe UI" font-size="14"><text x="20" y="25">Open Battery — '+theme+' — rendered assets (16 / 24 / 32 / 48 px)</text>';
  rows.forEach((name,row)=>{
    const y=44+row*54;svg+='<text x="20" y="'+(y+26)+'">'+name+'</text>';
    [16,24,32,48].forEach((size,col)=>{
      const data=render(sources[name],size,name==='app'?null:theme==='dark'?[184,184,184]:[102,102,102]);
      svg+='<image x="'+(170+col*100)+'" y="'+y+'" width="'+size+'" height="'+size+'" href="data:image/png;base64,'+data.toString('base64')+'"/>';
    });
  });
  svg+='</g></svg>';
  fs.writeFileSync(path.join(output,'preview-'+theme+'.png'),new Resvg(svg).render().asPng());
}
console.log('Generated app + 3 state icons in two themes; 7 tray sizes; previews include all alternatives.');
