// The ONLY source of logo geometry is the pinned 125A-Branding SVG.
// Keep its exact 2:1 aspect and the existing 156x78 layout footprint.
const fs=require("node:fs");
const {Resvg}=require("@resvg/resvg-js");
const source="tests/branding_reference/125A_Logo_Master_FINAL.svg";
const target="vst/resource/125A_Logo_Master.png";
const svg=fs.readFileSync(source,"utf8");
if(!svg.includes('viewBox="0 0 1774 887"')||!svg.includes('fill="#D71920"'))
 throw new Error("Unexpected official 125A master geometry");
const png=new Resvg(svg,{fitTo:{mode:"width",value:156}}).render().asPng();
fs.writeFileSync(target,png);
console.log("Official 125A SVG master rendered to 156x78 PNG:",png.length,"bytes");
