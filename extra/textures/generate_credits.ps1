# Deterministic HD typography and contour reconstruction of the original artwork.
# Run from any directory with PowerShell 7 and System.Drawing.
Add-Type -AssemblyName System.Drawing
$destination = Join-Path $PSScriptRoot 'Credits'
$original = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../assets/original/textures'))
$null = New-Item -ItemType Directory -Force -Path $destination
$ErrorActionPreference = 'Stop'
Add-Type -ReferencedAssemblies System.Drawing.Common,System.Drawing.Primitives,System.Collections,System.Runtime,System.Private.Windows.GdiPlus,System.Private.Windows.Core -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Collections.Generic;
public static class CreditsArt {
    static void Edge(Dictionary<Point,List<Point>> edges, Point a, Point b) {
        if (!edges.ContainsKey(a)) edges[a]=new List<Point>();
        edges[a].Add(b);
    }
    // Trace silhouettes, then rasterize smooth closed vector paths at the new resolution.
    static void Simplify(List<PointF> p,int first,int last,double tolerance,List<PointF> output) {
        double dx=p[last].X-p[first].X,dy=p[last].Y-p[first].Y,len=dx*dx+dy*dy;
        int split=first; double largest=0;
        for(int i=first+1;i<last;i++) {
            double t=len>0?Math.Max(0,Math.Min(1,((p[i].X-p[first].X)*dx+(p[i].Y-p[first].Y)*dy)/len)):0;
            double x=p[i].X-p[first].X-t*dx,y=p[i].Y-p[first].Y-t*dy,d=x*x+y*y;
            if(d>largest){largest=d;split=i;}
        }
        if(largest>tolerance*tolerance && split>first){Simplify(p,first,split,tolerance,output);Simplify(p,split,last,tolerance,output);}
        else output.Add(p[first]);
    }
    static void Contours(Graphics g, bool[,] mask, Color color, double tolerance) {
        int w=mask.GetLength(0), h=mask.GetLength(1);
        var edges=new Dictionary<Point,List<Point>>();
        for(int y=0;y<h;y++) for(int x=0;x<w;x++) if(mask[x,y]) {
            if(y==0 || !mask[x,y-1]) Edge(edges,new Point(x,y),new Point(x+1,y));
            if(x==w-1 || !mask[x+1,y]) Edge(edges,new Point(x+1,y),new Point(x+1,y+1));
            if(y==h-1 || !mask[x,y+1]) Edge(edges,new Point(x+1,y+1),new Point(x,y+1));
            if(x==0 || !mask[x-1,y]) Edge(edges,new Point(x,y+1),new Point(x,y));
        }
        using(var path=new GraphicsPath(FillMode.Alternate)) {
            while(edges.Count>0) {
                Point start=default(Point); foreach(var key in edges.Keys){ start=key; break; }
                Point p=start; var points=new List<PointF>(); int guard=0;
                do {
                    points.Add(new PointF(p.X,p.Y));
                    if(!edges.ContainsKey(p)) break;
                    var next=edges[p]; Point q=next[next.Count-1]; next.RemoveAt(next.Count-1);
                    if(next.Count==0) edges.Remove(p); p=q;
                } while(p!=start && ++guard<1000000);
                if(points.Count<4) continue;
                // Fit smooth vector silhouettes rather than retain source-pixel stair steps.
                var reduced=new List<PointF>();
                points.Add(points[0]); Simplify(points,0,points.Count-1,tolerance,reduced);
                if(reduced.Count>=4) path.AddClosedCurve(reduced.ToArray(),0.24f);
            }
            using(var brush=new SolidBrush(color)) g.FillPath(brush,path);
        }
    }
    public static void Trace(string input,string output,bool floor) {
        using(var src=new Bitmap(input)) using(var dst=new Bitmap(floor?2048:1536,floor?1708:3072))
        using(var g=Graphics.FromImage(dst)) {
            g.Clear(Color.Black); g.SmoothingMode=SmoothingMode.AntiAlias;
            g.ScaleTransform((float)dst.Width/src.Width,(float)dst.Height/src.Height);
            int[] thresholds=floor?new[]{32,62,86,112,168}:new[]{18,55,112};
            Color[] colors=floor?new[]{Color.FromArgb(23,36,23),Color.FromArgb(39,66,38),Color.FromArgb(58,91,49),Color.FromArgb(94,126,67),Color.FromArgb(197,199,156)}
                :new[]{Color.FromArgb(8,13,71),Color.FromArgb(59,76,143),Color.FromArgb(151,161,191)};
            for(int layer=0;layer<thresholds.Length;layer++) {
                var mask=new bool[src.Width,src.Height];
                for(int y=0;y<src.Height;y++) for(int x=0;x<src.Width;x++) {
                    Color c=src.GetPixel(x,y);
                    int value=floor?(c.R+2*c.G+c.B)/4:Math.Max(c.R,c.G);
                    mask[x,y]=value>=thresholds[layer];
                }
                Contours(g,mask,colors[layer],floor?0.55:1.05);
            }
            dst.Save(output,ImageFormat.Png);
        }
    }
    static void Center(GraphicsPath path,string text,float em,float y,float width) {
        using(var family=new FontFamily("Arial")) {
            var style=FontStyle.Bold;
            using(var line=new GraphicsPath()) {
                line.AddString(text,family,(int)style,em,new PointF(0,y),StringFormat.GenericTypographic);
                var bounds=line.GetBounds(); using(var move=new Matrix()) {
                    move.Translate((width-bounds.Width)/2-bounds.X,0); line.Transform(move); path.AddPath(line,false);
                }
            }
        }
    }
    public static void Title(string output,string role,string name) {
        using(var dst=new Bitmap(1900,744)) using(var g=Graphics.FromImage(dst)) using(var path=new GraphicsPath()) {
            g.Clear(Color.Black); g.SmoothingMode=SmoothingMode.AntiAlias;
            Center(path,role,136,192,1900); Center(path,name,136,360,1900);
            g.FillPath(Brushes.White,path); dst.Save(output,ImageFormat.Png);
        }
    }
    public static void Star(string output) {
        using(var dst=new Bitmap(512,512)) {
            for(int y=0;y<512;y++) for(int x=0;x<512;x++) {
                double dx=(x-255.5)/256,dy=(y-255.5)/256,r=Math.Sqrt(dx*dx+dy*dy);
                double angle=Math.Atan2(dy,dx);
                double core=Math.Exp(-r*r*55), halo=Math.Exp(-r*4.5)*0.70;
                double rays=Math.Pow(Math.Abs(Math.Cos(angle*8)),14)*Math.Exp(-r*5)*0.65;
                double light=Math.Min(1,core+halo+rays);
                dst.SetPixel(x,y,Color.FromArgb(255,(int)(255*light),(int)(255*Math.Min(1,core+0.75*(halo+rays))),(int)(255*Math.Min(1,core+0.30*(halo+rays)))));
            }
            dst.Save(output,ImageFormat.Png);
        }
    }
    public static void Chrome(string folder) {
        // Continuous grey/white liquid-chrome veins, without DDS blocks or face seams.
        for(int face=0;face<6;face++) using(var dst=new Bitmap(1024,1024)) {
            for(int y=0;y<1024;y++) for(int x=0;x<1024;x++) {
                double u=2*(x+0.5)/1024-1,v=2*(y+0.5)/1024-1,dx=0,dy=0,dz=0;
                switch(face) {
                    case 0: dx=1;dy=-v;dz=-u;break; case 1:dx=-1;dy=-v;dz=u;break;
                    case 2: dx=u;dy=1;dz=v;break; case 3:dx=u;dy=-1;dz=-v;break;
                    case 4: dx=u;dy=-v;dz=1;break; default:dx=-u;dy=-v;dz=-1;break;
                }
                double length=Math.Sqrt(dx*dx+dy*dy+dz*dz);dx/=length;dy/=length;dz/=length;
                double field=Math.Sin(dx*9+Math.Sin(dy*6))+Math.Sin(dy*8+Math.Cos(dz*7))
                    +Math.Sin(dz*11+Math.Cos(dx*5));
                double vein=Math.Exp(-field*field*6);
                double soft=0.5+0.5*Math.Sin(dx*6+dy*4+dz*7);
                int c=(int)(255*Math.Min(1,0.38+0.18*soft+0.44*vein));
                dst.SetPixel(x,y,Color.FromArgb(c,c,c));
            }
            dst.Save(System.IO.Path.Combine(folder,"chrome-"+face+".png"),ImageFormat.Png);
        }
    }
}
'@
[CreditsArt]::Trace((Join-Path $original 'cow.png'),(Join-Path $destination 'cow.png'),$false)
[CreditsArt]::Trace((Join-Path $original 'floor.png'),(Join-Path $destination 'floor.png'),$true)
[CreditsArt]::Title((Join-Path $destination 'Code.png'),'Code','msamurai')
[CreditsArt]::Title((Join-Path $destination '3d gfx.png'),'3d Gfx','quiesce')
[CreditsArt]::Title((Join-Path $destination '2d gfx.png'),'2d Gfx','sybex')
[CreditsArt]::Title((Join-Path $destination 'music.png'),'Music',([string][char]0x00a2+'at'))
& (Join-Path $PSScriptRoot 'generate_credits_end.ps1')
[CreditsArt]::Star((Join-Path $destination 'star.png'))
[CreditsArt]::Chrome($destination)
