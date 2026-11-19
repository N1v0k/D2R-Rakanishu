






#include "../xbg_cam.h"
#include <stdio.h>

static int failures;
#define CHECK(expression) do { if(!(expression)){printf("FAIL line %d: %s\n",__LINE__,#expression);++failures;} } while(0)
static bool near(double a,double b,double eps){return fabs(a-b)<=eps;}
static void put(uint8_t *block,size_t at,const float *m,size_t n){memcpy(block+at,m,n*sizeof(float));}
static void get(const uint8_t *block,size_t at,float *m,size_t n){memcpy(m,block+at,n*sizeof(float));}


static void xf(const float *M,const float v[4],float o[4]){for(int j=0;j<4;j++)o[j]=v[0]*M[j]+v[1]*M[4+j]+v[2]*M[8+j]+v[3]*M[12+j];}
static bool identity(const float *a,const float *b,double eps){float m[16];xbg_cam_m4_mul(a,b,m);for(int i=0;i<16;i++)if(!near(m[i],i%5==0?1:0,eps))return false;return true;}






static void original(uint8_t *block,float px,float pz,float py=0,float zoom=1.2f){
    memset(block,0,XBG_CAM_BLOCK);
    const float r[3]={0.70711f,0,-0.70711f},u[3]={-0.35355f,0.86603f,-0.35355f},b[3]={0.61237f,0.5f,0.61237f};
    const double k=atan(0.03/zoom)/atan(0.03),H=0.75*70.7107*k,d=H/(2*0.03/zoom),s=7.25*(k-1)+0.176777*8*k,bl=hypot(b[0],b[2]);
    const float eye[3]={float(px+b[0]*d+s*b[0]/bl),float(py+b[1]*d),float(pz+b[2]*d+s*b[2]/bl)};
    float V[16]={r[0],u[0],b[0],0, r[1],u[1],b[1],0, r[2],u[2],b[2],0,
        -(eye[0]*r[0]+eye[1]*r[1]+eye[2]*r[2]),-(eye[0]*u[0]+eye[1]*u[1]+eye[2]*u[2]),-(eye[0]*b[0]+eye[1]*b[1]+eye[2]*b[2]),1};
    float iV[16];xbg_cam_m4_inv(V,iV);
    float P[16]={float(2/(H*16/9)),0,0,0, 0,float(2/H),0,0, 0,0,1/1000.0f,0, 0,0,1.55f,1},VP[16],iVP[16];
    xbg_cam_m4_mul(V,P,VP);xbg_cam_m4_inv(VP,iVP);
    put(block,80,V,16);put(block,144,iV,16);put(block,208,P,16);put(block,336,VP,16);put(block,400,iVP,16);
    const float depth[4]={1000,100,0,1};put(block,592,depth,4);
}
static double heading_deg(const xbg_cam_state &s){return atan2(s.fz,s.fx)*180/3.14159265358979;}


static void top_ray(const float *ivp,double o[3],double d[3]){const float t1[4]={0,1,1,1},t0[4]={0,1,0.001f,1};float a[4],b[4];xf(ivp,t1,a);xf(ivp,t0,b);
    for(int i=0;i<3;i++){o[i]=a[i]/a[3];d[i]=b[i]/b[3]-o[i];}}
static double top_edge(const float *ivp){double o[3],d[3];top_ray(ivp,o,d);return atan2(-d[1],hypot(d[0],d[2]))*180/3.14159265358979;}
static double top_ground(const float *ivp,double px,double pz){double o[3],d[3];top_ray(ivp,o,d);double t=-o[1]/d[1];return hypot(o[0]+t*d[0]-px,o[2]+t*d[2]-pz);}

int main(){
    uint8_t src[XBG_CAM_BLOCK],dst[XBG_CAM_BLOCK];original(src,100,50);



    CHECK(xbg_cam_sig(src,XBG_CAM_BLOCK,XBG_CAM_BLOCK));
    CHECK(xbg_cam_sig(src,272,XBG_CAM_BLOCK));
    CHECK(!xbg_cam_sig(src,271,XBG_CAM_BLOCK));
    CHECK(!xbg_cam_sig(src,XBG_CAM_BLOCK,XBG_CAM_BLOCK-1));
    {uint8_t bad[XBG_CAM_BLOCK];memcpy(bad,src,sizeof bad);float x=0.69f;memcpy(bad+80,&x,4);CHECK(!xbg_cam_sig(bad,XBG_CAM_BLOCK,XBG_CAM_BLOCK));}
    {uint8_t persp[XBG_CAM_BLOCK];memcpy(persp,src,sizeof persp);float m1=-1;memcpy(persp+208+44,&m1,4);CHECK(!xbg_cam_sig(persp,XBG_CAM_BLOCK,XBG_CAM_BLOCK));}




    {float px,py,pz;xbg_cam_ground(src,&px,&py,&pz);CHECK(near(px,100,1e-2) && near(py,0,1e-2) && near(pz,50,1e-2));
     uint8_t low[XBG_CAM_BLOCK];original(low,100,50,-12);xbg_cam_ground(low,&px,&py,&pz);CHECK(near(px,100,1e-2) && near(py,-12,1e-2) && near(pz,50,1e-2));}



    {uint8_t z[XBG_CAM_BLOCK];original(z,100,50,0,0.6f);float pr[16],iv[16];get(z,208,pr,16);get(z,144,iv,16);CHECK(near(2/pr[5],88.4,0.1));
     float px,py,pz;xbg_cam_ground(z,&px,&py,&pz);CHECK(near(px,100,1e-2) && near(py,0,1e-2) && near(pz,50,1e-2));
     CHECK(hypot(iv[12]-884.03*iv[8]-100,iv[14]-884.03*iv[10]-50)>6);
     original(z,100,50,-12,0.6f);xbg_cam_ground(z,&px,&py,&pz);CHECK(near(px,100,1e-2) && near(py,-12,1e-2) && near(pz,50,1e-2));}



    xbg_cam_state st;xbg_cam_init(&st);xbg_cam_params par=xbg_cam_default_params(1);
    CHECK(xbg_cam_follow(&st,100,50,1,10.0,0,0,0,par.tau)==1);
    CHECK(st.have && near(st.fx,-0.70711,1e-4) && near(st.fz,-0.70711,1e-4));
    CHECK(xbg_cam_follow(&st,100,50,1,10.1,1,1,0,par.tau)==0);




    memcpy(dst,src,sizeof dst);float vp[16],ivp[16];xbg_cam_build(&st,src,dst,1,&par,vp,ivp);
    {float V[16],iV[16],P[16],iP[16],VP[16],iVP[16];get(dst,80,V,16);get(dst,144,iV,16);get(dst,208,P,16);get(dst,272,iP,16);get(dst,336,VP,16);get(dst,400,iVP,16);
     CHECK(identity(V,iV,1e-4) && identity(P,iP,1e-4) && identity(VP,iVP,1e-3));
     CHECK(memcmp(VP,vp,64)==0 && memcmp(iVP,ivp,64)==0);
     CHECK(near(st.eye[0],100+0.70711*20,2e-2) && near(st.eye[2],50+0.70711*20,2e-2) && st.eye[1]>=13);
     const float feet[4]={100,0,50,1};float c[4];xf(VP,feet,c);CHECK(c[3]>0 && near(c[0]/c[3],0,1e-4) && c[1]/c[3]<0 && c[1]/c[3]>-1);
     const float n0[4]={0,0,-par.near_,1},f0[4]={0,0,-3000,1};float cn[4],cf[4];xf(P,n0,cn);xf(P,f0,cf);CHECK(near(cn[2]/cn[3],1,1e-5) && near(cf[2]/cf[3],0,1e-5));


     const float t1[4]={0,1,1,1},t0[4]={0,1,0.001f,1};float a[4],b[4];xf(iVP,t1,a);xf(iVP,t0,b);
     double dx=b[0]/b[3]-a[0]/a[3],dy=b[1]/b[3]-a[1]/a[3],dz=b[2]/b[3]-a[2]/a[3];CHECK(atan2(-dy,hypot(dx,dz))*180/3.14159265358979>=3-0.05);}


    {float prev[16];get(dst,464,prev,16);CHECK(memcmp(prev,vp,64)==0);
     float xy[2];int32_t flag;get(dst,592,xy,2);memcpy(&flag,dst+604,4);CHECK(flag==0);
     float P[16];get(dst,208,P,16);const float v100[4]={0,0,-100,1};float c[4];xf(P,v100,c);double d=c[2]/c[3];CHECK(near(xy[0]/(d-xy[1]),100,1e-2));}


    {float last[16];memcpy(last,vp,64);original(src,101,50);CHECK(xbg_cam_follow(&st,101,50,2,10.0+1/60.0,0,0,0,par.tau)==1);
     memcpy(dst,src,sizeof dst);xbg_cam_build(&st,src,dst,1,&par,vp,ivp);float prev[16];get(dst,464,prev,16);CHECK(memcmp(prev,last,64)==0);}



    {xbg_cam_state s;xbg_cam_init(&s);xbg_cam_follow(&s,0,0,1,1.0,0,0,0,0.2f);s.fx=1;s.fz=0;
     xbg_cam_follow(&s,0,0,2,1.2,1,0,1,0.2f);CHECK(near(heading_deg(s),90*(1-exp(-1.0)),0.2));
     s.fx=float(cos(170*3.14159265358979/180));s.fz=float(sin(170*3.14159265358979/180));
     xbg_cam_follow(&s,0,0,3,1.4,1,float(cos(-170*3.14159265358979/180)),float(sin(-170*3.14159265358979/180)),0.2f);
     double h=heading_deg(s);if(h<0)h+=360;CHECK(near(h,170+20*(1-exp(-1.0)),0.2));
     s.fx=1;s.fz=0;xbg_cam_follow(&s,0,5,4,1.6,0,0,0,0.2f);CHECK(heading_deg(s)>0 && heading_deg(s)<90);
     xbg_cam_follow(&s,0,30,5,1.8,1,1,0,0.2f);CHECK(near(s.fx,-0.70711,1e-4) && near(s.fz,-0.70711,1e-4));}



    {xbg_cam_state s;xbg_cam_init(&s);xbg_cam_params p2=xbg_cam_default_params(2);original(src,100,50);xbg_cam_follow(&s,100,50,1,1.0,0,0,0,p2.tau);
     memcpy(dst,src,sizeof dst);xbg_cam_build(&s,src,dst,2,&p2,vp,ivp);
     CHECK(near(s.eye[0],100-0.70711*2.2,2e-2) && near(s.eye[1],6,1e-2) && near(s.eye[2],50-0.70711*2.2,2e-2));
     CHECK(near(asin(-s.fwd[1])*180/3.14159265358979,8,1e-3) && p2.fov==75);


     uint8_t low[XBG_CAM_BLOCK],d2[XBG_CAM_BLOCK];original(low,100,50,-12);xbg_cam_state s2=s;memcpy(d2,low,sizeof d2);xbg_cam_build(&s2,low,d2,2,&p2,vp,ivp);
     CHECK(near(s2.eye[1],-6,1e-2) && near(s2.eye[0],s.eye[0],2e-2) && near(s2.eye[2],s.eye[2],2e-2) && near(s2.fwd[1],s.fwd[1],1e-5));
     xbg_cam_params p1=xbg_cam_default_params(1);xbg_cam_state a=s,b=s;a.dcur=b.dcur=0;
     memcpy(d2,src,sizeof d2);xbg_cam_build(&a,src,d2,1,&p1,vp,ivp);memcpy(d2,low,sizeof d2);xbg_cam_build(&b,low,d2,1,&p1,vp,ivp);
     CHECK(near(b.eye[1],a.eye[1]-12,1e-2) && near(b.eye[0],a.eye[0],2e-2) && near(b.fwd[1],a.fwd[1],1e-5));}



    {uint8_t scene[XBG_CAM_SCENE];memset(scene,0,sizeof scene);const float dir[3]={-0.61237f,-0.5f,-0.61237f},lin[3]={1,1,550};put(scene,32,dir,3);put(scene,848,lin,3);
     CHECK(xbg_cam_sig10(scene,XBG_CAM_SCENE,XBG_CAM_SCENE) && xbg_cam_sig10(scene,44,XBG_CAM_SCENE));
     CHECK(!xbg_cam_sig10(scene,43,XBG_CAM_SCENE) && !xbg_cam_sig10(scene,XBG_CAM_SCENE,XBG_CAM_SCENE-16));
     uint8_t copy[XBG_CAM_SCENE];memcpy(copy,scene,sizeof copy);xbg_cam_state empty;xbg_cam_init(&empty);xbg_cam_build10(&empty,copy);CHECK(memcmp(copy,scene,sizeof copy)==0);
     xbg_cam_build10(&st,copy);float e[3],f[3],l[3];get(copy,16,e,3);get(copy,32,f,3);get(copy,848,l,3);
     CHECK(memcmp(e,st.eye,12)==0 && memcmp(f,st.fwd,12)==0 && l[0]==1 && l[1]==1 && l[2]==0);}




    {xbg_cam_state s;xbg_cam_init(&s);
     CHECK(near(xbg_cam_pull(&s,20,8,0),8,1e-6));
     CHECK(near(xbg_cam_pull(&s,20,0,0.4),8+12*(1-exp(-1.0)),1e-3));
     float before=s.dcur;CHECK(near(xbg_cam_pull(&s,20,4,0.08),before+(4-before)*(1-exp(-1.0)),1e-3));
     xbg_cam_state f;xbg_cam_init(&f);CHECK(near(xbg_cam_pull(&f,20,30,0),20,1e-6));
     xbg_cam_state g;xbg_cam_init(&g);CHECK(near(xbg_cam_pull(&g,20,0.5f,0),2,1e-6));}



    {xbg_cam_state s;xbg_cam_init(&s);xbg_cam_params p=xbg_cam_default_params(1);original(src,100,50);xbg_cam_follow(&s,100,50,1,1.0,0,0,0,p.tau);xbg_cam_pull(&s,p.dist,8,0);
     memcpy(dst,src,sizeof dst);xbg_cam_build(&s,src,dst,1,&p,vp,ivp);
     const double boom=p.lookh+sqrt(p.dist*p.dist+(p.h-p.lookh)*(p.h-p.lookh)-64.0);
     CHECK(near(hypot(s.eye[0]-100,s.eye[2]-50),8,2e-2) && s.eye[1]>=boom-1e-3);
     CHECK(top_edge(ivp)>=3-0.05 && top_ground(ivp,100,50)<=p.reach+0.5);


     const float feet[4]={100,0,50,1};float c[4];xf(vp,feet,c);CHECK(c[3]>0 && c[1]/c[3]<0 && c[1]/c[3]>-0.75);}



    {xbg_cam_params p=xbg_cam_default_params(1);CHECK(p.pull_pitch==0);original(src,100,50);
     auto down=[&](float pitch){xbg_cam_state s;xbg_cam_init(&s);xbg_cam_follow(&s,100,50,1,1.0,0,0,0,p.tau);xbg_cam_pull(&s,p.dist,2,0);xbg_cam_params q=p;q.pull_pitch=pitch;
      memcpy(dst,src,sizeof dst);xbg_cam_build(&s,src,dst,1,&q,vp,ivp);const double la=p.look*2/p.dist,ax=100+s.fx*la,az=50+s.fz*la;
      return atan2(s.eye[1]-p.lookh,hypot(s.eye[0]-ax,s.eye[2]-az))*180/3.14159265358979;};
     CHECK(down(0)>80);CHECK(down(40)<=40+0.3 && down(40)>30);}



    {xbg_cam_state s;xbg_cam_init(&s);xbg_cam_params p=xbg_cam_default_params(1);original(src,100,50);xbg_cam_follow(&s,100,50,1,1.0,0,0,0,p.tau);
     memcpy(dst,src,sizeof dst);xbg_cam_build(&s,src,dst,1,&p,vp,ivp);
     CHECK(near(hypot(s.eye[0]-100,s.eye[2]-50),20,2e-2) && top_ground(ivp,100,50)<=p.reach+0.5 && top_ground(ivp,100,50)>p.reach-5);
     xbg_cam_params far=p;far.reach=0;xbg_cam_state t;xbg_cam_init(&t);xbg_cam_follow(&t,100,50,1,1.0,0,0,0,p.tau);xbg_cam_build(&t,src,dst,1,&far,vp,ivp);
     CHECK(near(top_edge(ivp),3,0.05));}




    {xbg_cam_state s;xbg_cam_init(&s);xbg_cam_follow(&s,0,0,1,1.0,0,0,0,0.2f);s.fx=1;s.fz=0;
     const float rx=-s.fz,rz=s.fx;xbg_cam_turn(&s,0.1f);CHECK(s.fx*rx+s.fz*rz>0 && near(heading_deg(s),0.1*180/3.14159265358979,1e-3));
     s.hold=1;const float hx=s.fx,hz=s.fz;xbg_cam_follow(&s,0,3,2,1.2,1,0,1,0.2f);CHECK(s.fx==hx && s.fz==hz);
     xbg_cam_follow(&s,0,40,3,1.4,1,0,1,0.2f);CHECK(near(s.fx,-0.70711,1e-4) && near(s.fz,-0.70711,1e-4));
     s.hold=0;xbg_cam_follow(&s,0,40,4,1.6,1,0,1,0.2f);CHECK(heading_deg(s)>-135+1);}






    {xbg_cam_state s;xbg_cam_init(&s);CHECK(xbg_cam_drag(&s,0,0,1,1.0,20,1)==1);CHECK(near(s.fx,-0.70711,1e-4) && near(s.fz,-0.70711,1e-4));
     s.fx=1;s.fz=0;xbg_cam_drag(&s,5,0,2,1.1,20,1);CHECK(near(s.fx,1,1e-6) && near(s.fz,0,1e-6));
     xbg_cam_drag(&s,5,5,3,1.2,20,1);CHECK(near(heading_deg(s),atan2(5.0,20.0)*180/3.14159265358979,1e-3));
     CHECK(xbg_cam_drag(&s,9,9,3,1.25,20,1)==0 && s.px==5 && s.pz==5);
     s.fx=1;s.fz=0;xbg_cam_drag(&s,0,5,4,1.3,20,1);CHECK(near(s.fx,1,1e-6) && near(s.fz,0,1e-6));
     xbg_cam_drag(&s,0,30,5,1.4,20,1);CHECK(near(s.fx,-0.70711,1e-4) && near(s.fz,-0.70711,1e-4));
     s.fx=1;s.fz=0;const double cx0=s.px-20*s.fx,cz0=s.pz-20*s.fz;double turned=0,x=s.px,z=s.pz;uint64_t fr=6;
     for(int i=0;i<200;++i){const double a0=atan2(s.fz,s.fx);x+=0.5*s.fz;z+=-0.5*s.fx;xbg_cam_drag(&s,float(x),float(z),fr++,1.5+i/60.0,20,1);
      double d=atan2(s.fz,s.fx)-a0;while(d>3.14159265358979)d-=2*3.14159265358979;while(d<-3.14159265358979)d+=2*3.14159265358979;turned+=d;}
     CHECK(hypot(s.px-20*s.fx-cx0,s.pz-20*s.fz-cz0)<2 && near(fabs(turned),100.0/20,0.25));}



    {xbg_cam_state s;xbg_cam_init(&s);s.fx=1;s.fz=0;xbg_cam_ease(&s,0,1,0.35,0.35f);CHECK(near(heading_deg(s),90*(1-exp(-1.0)),0.2));
     s.fx=1;s.fz=0;xbg_cam_ease(&s,float(cos(-170*3.14159265358979/180)),float(sin(-170*3.14159265358979/180)),0.35,0.35f);CHECK(heading_deg(s)<0);
     s.fx=1;s.fz=0;xbg_cam_ease(&s,0,1,0,0.35f);xbg_cam_ease(&s,0,0,1,0.35f);CHECK(s.fx==1 && s.fz==0);}


    {xbg_cam_state s;xbg_cam_init(&s);xbg_cam_drag(&s,5,0,1,1.0,20,0.5f);s.fx=1;s.fz=0;xbg_cam_drag(&s,5,5,2,1.1,20,0.5f);
     CHECK(near(heading_deg(s),0.5*atan2(5.0,20.0)*180/3.14159265358979,1e-3));
     s.fx=1;s.fz=0;xbg_cam_drag(&s,5,10,3,1.2,20,0);CHECK(near(s.fx,1,1e-6) && near(s.fz,0,1e-6));}






    {const double deg=3.14159265358979/180;xbg_cam_state t0=st;t0.tilt=0;uint8_t d0[XBG_CAM_BLOCK];memcpy(d0,src,sizeof d0);float v0[16],i0[16];xbg_cam_build(&t0,src,d0,1,&par,v0,i0);
     const double hd0=hypot(t0.eye[0]-100,t0.eye[2]-50),h0=t0.eye[1],run=hd0+par.look,rise=h0-par.lookh,R=hypot(run,rise),e0=atan2(rise,run);
     CHECK(near(hd0,20,2e-2) && near(t0.tilt_hi,e0-4*deg,1e-3) && near(t0.tilt_lo,e0-75*deg,1e-3));
     auto rig=[&](float tilt,double &hd,double &h){xbg_cam_state t=st;t.tilt=tilt;uint8_t d1[XBG_CAM_BLOCK];memcpy(d1,src,sizeof d1);float v1[16],i1[16];xbg_cam_build(&t,src,d1,1,&par,v1,i1);hd=hypot(t.eye[0]-100,t.eye[2]-50);h=t.eye[1];};
     double hd,h;rig(float(10*deg),hd,h);CHECK(near(hd,20,2e-2) && near(h,par.lookh+R*sin(e0-10*deg),2e-2) && h<h0);
     rig(1.4f,hd,h);CHECK(near(hd,20,2e-2) && near(h,par.lookh+R*sin(4*deg),2e-2));
     rig(-0.3f,hd,h);CHECK(near(hd,R*cos(e0+0.3)-par.look,2e-2) && near(h,par.lookh+R*sin(e0+0.3),2e-2) && hd<20);
     xbg_cam_params fp=xbg_cam_default_params(2);xbg_cam_state t2=st;t2.tilt=float(5*deg);uint8_t d2[XBG_CAM_BLOCK];memcpy(d2,src,sizeof d2);float v2[16],i2[16];xbg_cam_build(&t2,src,d2,2,&fp,v2,i2);
     CHECK(near(atan2(-t2.fwd[1],hypot(t2.fwd[0],t2.fwd[2])),(fp.pitch-5)*deg,1e-3) && near(t2.tilt_hi,(fp.pitch+10)*deg,1e-4));}
    if(failures){printf("%d failure(s)\n",failures);return 1;}
    printf("cam_test: all checks passed\n");return 0;
}
