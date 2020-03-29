#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include "model.h"
#include "tgaimage.h"
#include "geometry.h"
#define width 800
#define height 800

Model *model;

Vec3f light_dir(0,0,-1);

void line(int x0, int y0, int x1, int y1, TGAImage &image, TGAColor color) {
    bool steep = false;
    if (std::abs(x0-x1)<std::abs(y0-y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0>x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    for (int x=x0; x<=x1; x++) {
        float t = (x-x0)/(float)(x1-x0);
        int y = y0*(1.-t) + y1*t;
        if (steep) {
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
    }
}

Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P) {
    Vec3f s[2];
    for (int i=2; i--; ) {
        s[i][0] = C[i]-A[i];
        s[i][1] = B[i]-A[i];
        s[i][2] = A[i]-P[i];
    }
    Vec3f u = cross(s[0], s[1]);
    if (std::abs(u[2])>1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f-(u.x+u.y)/u.z, u.y/u.z, u.x/u.z);
    return Vec3f(-1,1,1); // in this case generate negative coordinates, it will be thrown away by the rasterizator
}

// void triangle(Vec3f t0, Vec3f t1, Vec3f t2, Vec2i uv0, Vec2i uv1, Vec2i uv2, TGAImage &image, float intensity, float *zbuffer) {
//     if (t0.y==t1.y && t0.y==t2.y) return; // i dont care about degenerate triangles
//     if (t0.y>t1.y) { std::swap(t0, t1); std::swap(uv0, uv1); }
//     if (t0.y>t2.y) { std::swap(t0, t2); std::swap(uv0, uv2); }
//     if (t1.y>t2.y) { std::swap(t1, t2); std::swap(uv1, uv2); }

//     int total_height = t2.y-t0.y;
//     for (int i=0; i<total_height; i++) {
//         bool second_half = i>t1.y-t0.y || t1.y==t0.y;
//         int segment_height = second_half ? t2.y-t1.y : t1.y-t0.y;
//         float alpha = (float)i/total_height;
//         float beta  = (float)(i-(second_half ? t1.y-t0.y : 0))/segment_height; // be careful: with above conditions no division by zero here
//         Vec3i A   =               t0  + Vec3f(t2-t0  )*alpha;
//         Vec3i B   = second_half ? t1  + Vec3f(t2-t1  )*beta : t0  + Vec3f(t1-t0  )*beta;
//         Vec2i uvA =               uv0 +      (uv2-uv0)*alpha;
//         Vec2i uvB = second_half ? uv1 +      (uv2-uv1)*beta : uv0 +      (uv1-uv0)*beta;
//         if (A.x>B.x) { std::swap(A, B); std::swap(uvA, uvB); }
//         for (int j=A.x; j<=B.x; j++) {
//             float phi = B.x==A.x ? 1. : (float)(j-A.x)/(float)(B.x-A.x);
//             Vec3i   P = Vec3f(A) + Vec3f(B-A)*phi;
//             Vec2i uvP =     uvA +   (uvB-uvA)*phi;
//             int idx = P.x+P.y*width;
//             if (zbuffer[idx]<P.z) {
//                 zbuffer[idx] = P.z;
//                 TGAColor color = model->diffuse(uvP);
//                 image.set(P.x, P.y, TGAColor(color[0]*intensity, color[1]*intensity, color[2]*intensity));
//             }
//         }
//     }
// }

void triangle(Vec3f *pts, float *zbuffer, TGAImage &image,float intensity,Vec2i *uv) {
    Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width()-1, image.get_height()-1);
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::max(0.f,      std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    Vec3f P;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            Vec3f bc_screen  = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0) continue;
            P.z = 0;
            Vec2i uv1;
            for(int i=0;i<3;i++) uv1[0]+=uv[i][0]*bc_screen[i];
            for(int i=0;i<3;i++) uv1[1]+=uv[i][1]*bc_screen[i];
            for (int i=0; i<3; i++) P.z += pts[i][2]*bc_screen[i];
            if (zbuffer[int(P.x+P.y*width)]<P.z) {
                zbuffer[int(P.x+P.y*width)] = P.z;
                TGAColor color=model->diffuse(uv1);
                image.set(P.x, P.y,TGAColor(255,255,255));
                // image.set(P.x, P.y, TGAColor(255));

            }
        }
    }
}



Vec3f word2screen(Vec3f v)
{
    return Vec3f(int(width/2)*v.x+0.5,int(height/2)*v.y+0.5,v.z);
}

int main(int argc, char** argv)
{
    //model=new Model("./obj/african_head/african_head.obj");
    // model=new Model("obj/african_head/african_head.obj");
    if (2==argc) {
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head/african_head.obj");
    }
    float zbuffer[width*height];
    for(int i=width*height;i--;zbuffer[i]=-std::numeric_limits<float>::max());
    
    TGAImage image(width,height,TGAImage::RGB);
    for(int i=0;i<model->nfaces();i++)
    {
        std::vector<int> face =model->face(i);
        Vec3f pts[3];
        for(int i=0;i<3;i++)
        {
            pts[i]=model->vert(face[i]);//三个顶点
        }

        Vec3f tem1=pts[2]-pts[0];
        Vec3f tem2=pts[1]-pts[0];
        Vec3f n(tem1.y*tem2.z-tem1.z*tem2.y, tem1.z*tem2.x-tem1.x*tem2.z, tem1.x*tem2.y-tem1.y*tem2.x);
        n.normalize();//法向量
        float intensity = n*light_dir;
        //如果小于0的话就说明没有照到
        if (intensity>0) {
            Vec2i uv[3];
            for (int k=0; k<3; k++) {
                uv[k] = model->uv(i, k);
            }
            triangle(pts, zbuffer, image,intensity,uv);
            // triangle(pts[0], pts[1], pts[2], uv[0], uv[1], uv[2], image, intensity, zbuffer);
        }
        
    }
    
    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    
    return 0;
}