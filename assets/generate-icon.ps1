$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing.Common
Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
public static class GlassIcon {
 static GraphicsPath Round(float x,float y,float w,float h,float r) { var p=new GraphicsPath();p.AddArc(x,y,2*r,2*r,180,90);p.AddArc(x+w-2*r,y,2*r,2*r,270,90);p.AddArc(x+w-2*r,y+h-2*r,2*r,2*r,0,90);p.AddArc(x,y+h-2*r,2*r,2*r,90,90);p.CloseFigure();return p; }
 static void Plate(Graphics g,float x,float y,float w,float h,float r,Color a,Color b,Color edge) {using(var p=Round(x,y,w,h,r))using(var brush=new LinearGradientBrush(new RectangleF(x,y,w,h),a,b,60))using(var pen=new Pen(edge,2)){g.FillPath(brush,p);g.DrawPath(pen,p);}}
 static void Resource(BinaryWriter w,ushort type,ushort id,byte[] data){w.Write((uint)data.Length);w.Write((uint)32);w.Write((ushort)65535);w.Write(type);w.Write((ushort)65535);w.Write(id);w.Write((uint)0);w.Write((ushort)0x1030);w.Write((ushort)0x409);w.Write((uint)0);w.Write((uint)0);w.Write(data);while(w.BaseStream.Position%4!=0)w.Write((byte)0);}
 public static void Generate(string dir){
  using(var image=new Bitmap(512,512,PixelFormat.Format32bppArgb)){
   using(var g=Graphics.FromImage(image)){
    g.SmoothingMode=SmoothingMode.AntiAlias;g.ScaleTransform(2,2);g.Clear(Color.Transparent);
    Plate(g,12,12,232,232,57,Color.FromArgb(17,37,64),Color.FromArgb(18,61,112),Color.FromArgb(100,120,207,255));
    var state=g.Save();g.TranslateTransform(110,126);g.RotateTransform(-14);g.TranslateTransform(-110,-126);
    Plate(g,53,52,111,151,32,Color.FromArgb(32,157,224),Color.FromArgb(20,83,178),Color.FromArgb(180,165,247,255));g.Restore(state);
    state=g.Save();g.TranslateTransform(155,148);g.RotateTransform(12);g.TranslateTransform(-155,-148);
    Plate(g,99,76,111,146,33,Color.FromArgb(235,214,255,255),Color.FromArgb(170,35,120,229),Color.FromArgb(220,237,255,255));
    using(var p=new GraphicsPath())using(var pen=new Pen(Color.FromArgb(230,255,255,255),5)){p.AddBezier(117,109,119,94,129,94,138,94);pen.StartCap=pen.EndCap=LineCap.Round;g.DrawPath(pen,p);}g.Restore(state);
    using(var p=new GraphicsPath())using(var brush=new SolidBrush(Color.FromArgb(233,255,255))){p.AddBezier(199,34,201,50,201,50,216,52);p.AddBezier(216,52,201,54,201,54,199,70);p.AddBezier(199,70,197,54,197,54,182,52);p.AddBezier(182,52,197,50,197,50,199,34);p.CloseFigure();g.FillPath(brush,p);}
   }
   image.Save(Path.Combine(dir,"icon-preview.png"),ImageFormat.Png);
   int[] sizes={16,20,24,32,40,48,64,128,256};var pngs=new List<byte[]>();foreach(int size in sizes){using(var b=new Bitmap(size,size,PixelFormat.Format32bppArgb)){using(var g=Graphics.FromImage(b)){g.InterpolationMode=InterpolationMode.HighQualityBicubic;g.DrawImage(image,new Rectangle(0,0,size,size));}using(var m=new MemoryStream()){b.Save(m,ImageFormat.Png);pngs.Add(m.ToArray());}}}
   using(var w=new BinaryWriter(File.Create(Path.Combine(dir,"icon.ico")))){w.Write((ushort)0);w.Write((ushort)1);w.Write((ushort)sizes.Length);uint offset=(uint)(6+16*sizes.Length);for(int i=0;i<sizes.Length;i++){w.Write((byte)(sizes[i]==256?0:sizes[i]));w.Write((byte)(sizes[i]==256?0:sizes[i]));w.Write((ushort)0);w.Write((ushort)1);w.Write((ushort)32);w.Write((uint)pngs[i].Length);w.Write(offset);offset+=(uint)pngs[i].Length;}foreach(var png in pngs)w.Write(png);}
   using(var w=new BinaryWriter(File.Create(Path.Combine(dir,"icon.res")))){
    // Required null resource header.
    w.Write((uint)0);w.Write((uint)32);w.Write((ushort)65535);w.Write((ushort)0);w.Write((ushort)65535);w.Write((ushort)0);w.Write((uint)0);w.Write((ushort)0);w.Write((ushort)0);w.Write((uint)0);w.Write((uint)0);
    for(int i=0;i<pngs.Count;i++)Resource(w,3,(ushort)(10+i),pngs[i]);
    using(var m=new MemoryStream()){using(var group=new BinaryWriter(m,System.Text.Encoding.UTF8,true)){group.Write((ushort)0);group.Write((ushort)1);group.Write((ushort)sizes.Length);for(int i=0;i<sizes.Length;i++){group.Write((byte)(sizes[i]==256?0:sizes[i]));group.Write((byte)(sizes[i]==256?0:sizes[i]));group.Write((ushort)0);group.Write((ushort)1);group.Write((ushort)32);group.Write((uint)pngs[i].Length);group.Write((ushort)(10+i));}}Resource(w,14,101,m.ToArray());}
   }
  }
 }
}
'@ -ReferencedAssemblies System.Drawing.Common,System.Drawing.Primitives,System.Runtime,System.Collections,System.Private.Windows.GdiPlus,System.Private.Windows.Core
[GlassIcon]::Generate($PSScriptRoot)
