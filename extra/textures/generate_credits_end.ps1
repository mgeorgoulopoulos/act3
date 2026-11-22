# A fixed optical-film title: no animated letters, camera, flicker or grain.
# Typeface outlines are rasterized before simulating light through the film mask.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
Add-Type -ReferencedAssemblies System.Drawing.Common,System.Drawing.Primitives,System.Collections,System.Runtime,System.Private.Windows.GdiPlus,System.Private.Windows.Core -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
public static class CreditsEndingArt {
    const int W=3200,H=2400;
    static void Line(GraphicsPath logo,string text,float top,float width,float height) {
        using(var line=new GraphicsPath()) using(var family=new FontFamily("Baskerville Old Face")) {
            line.AddString(text,family,0,440,new PointF(0,0),StringFormat.GenericTypographic);
            var b=line.GetBounds();
            using(var transform=new Matrix(width/b.Width,0,0,height/b.Height,
                (W-width)/2-b.X*width/b.Width,top-b.Y*height/b.Height)) {
                line.Transform(transform); logo.AddPath(line,false);
            }
        }
    }
    static float[] Box(float[] input,int radius) {
        var temp=new float[W*H]; var output=new float[W*H]; float divisor=2*radius+1;
        for(int y=0;y<H;y++) {
            int row=y*W; double sum=0;
            for(int x=-radius;x<=radius;x++) sum+=input[row+Math.Max(0,Math.Min(W-1,x))];
            for(int x=0;x<W;x++) {
                temp[row+x]=(float)(sum/divisor);
                sum+=input[row+Math.Min(W-1,x+radius+1)]-input[row+Math.Max(0,x-radius)];
            }
        }
        for(int x=0;x<W;x++) {
            double sum=0;
            for(int y=-radius;y<=radius;y++) sum+=temp[Math.Max(0,Math.Min(H-1,y))*W+x];
            for(int y=0;y<H;y++) {
                output[y*W+x]=(float)(sum/divisor);
                sum+=temp[Math.Min(H-1,y+radius+1)*W+x]-temp[Math.Max(0,y-radius)*W+x];
            }
        }
        return output;
    }
    static float[] Diffuse(float[] input,int radius) { return Box(Box(Box(input,radius),radius),radius); }
    static byte Channel(double value) { return (byte)Math.Round(255*Math.Max(0,Math.Min(1,value))); }
    public static void Generate(string output) {
        using(var mask=new Bitmap(W,H,PixelFormat.Format32bppArgb)) using(var g=Graphics.FromImage(mask))
        using(var logo=new GraphicsPath()) {
            g.Clear(Color.Black); g.SmoothingMode=SmoothingMode.AntiAlias;
            // A standalone two-line serif composition.
            Line(logo,"ALL RIGHTS",805,2440,276);
            Line(logo,"REVERSED",1132,2440,306);
            using(var aperture=new Pen(Color.White,4.8f)) {
                aperture.LineJoin=LineJoin.Round; g.DrawPath(aperture,logo);
            }
            var rect=new Rectangle(0,0,W,H);
            var data=mask.LockBits(rect,ImageLockMode.ReadOnly,PixelFormat.Format32bppArgb);
            var raw=new byte[W*H*4]; Marshal.Copy(data.Scan0,raw,0,raw.Length); mask.UnlockBits(data);
            var core=new float[W*H];
            for(int i=0;i<core.Length;i++) core[i]=raw[4*i+2]/255f;
            var soft=Diffuse(core,1); var bleed=Diffuse(core,3);
            var halo=Diffuse(core,12); var scatter=Diffuse(core,40);
            for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
                int i=y*W+x;
                // Uneven gel exposure and fixed fine grain, rather than a uniform neon stroke.
                double exposure=0.83+0.13*Math.Sin(x*0.004+y*0.002)+0.09*Math.Cos(x*0.001-y*0.004);
                uint hash=unchecked((uint)(x*374761393+y*668265263+19));
                hash=(hash^(hash>>13))*1274126177u; hash^=hash>>16;
                double grain=(hash/4294967295.0-0.5)*0.16;
                double sharp=0.62*core[i]+0.70*soft[i];
                double light=(sharp+1.6*bleed[i]+2.2*halo[i]+2.5*scatter[i])*exposure;
                light*=1+grain;
                double hot=Math.Pow(Math.Max(0,sharp*exposure),2);
                raw[4*i]=Channel(0.009*light+0.026*hot);
                raw[4*i+1]=Channel(0.015*light+0.085*hot);
                raw[4*i+2]=Channel(1-Math.Exp(-2.15*light));
                raw[4*i+3]=255;
            }
            using(var image=new Bitmap(W,H,PixelFormat.Format32bppArgb)) {
                data=image.LockBits(rect,ImageLockMode.WriteOnly,PixelFormat.Format32bppArgb);
                Marshal.Copy(raw,0,data.Scan0,raw.Length); image.UnlockBits(data);
                image.Save(output,ImageFormat.Png);
            }
        }
    }
}
'@
[CreditsEndingArt]::Generate((Join-Path $PSScriptRoot 'Credits/end.png'))
