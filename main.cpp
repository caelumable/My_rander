#include <iostream>
#include <limits>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include <algorithm>
const int width=800;
const int height=800;

Model *model;

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P);

void line(Vec2i x0, Vec2i x1, TGAImage &img, const TGAColor color)
{
    bool steep = false;
    if(std::abs(x1.x-x0.x)<std::abs(x1.y-x0.y))
    {
        std::swap(x0.x, x0.y);
        std::swap(x1.x, x1.y);
        steep = true;
    }
    
    if(x0.x>x1.x)
        std::swap(x0, x1);

    float dt = float(x1.y - x0.y) / (x1.x - x0.x);
    for (int x = x0.x; x < x1.x;x++)
    {
        int y = x0.y + (x -x0.x) * dt;
        if(steep)
        {
            img.set(y, x, color);
        }
        else 
            img.set(x, y, color);
    }
}

void triangle(Vec3f *pts, float *zbuffer, TGAImage &image, Vec2i uv[3])
{
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    Vec3f P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
    {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
        {
            Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            P.z = 0;
            Vec2i uvP(0, 0);
            for (int i = 0; i < 3; i++)
            {
                P.z += pts[i][2] * bc_screen[i];
                uvP.x += uv[i][0] * bc_screen[i];
                uvP.y += uv[i][1] * bc_screen[i];
            }
            if (zbuffer[int(P.x + P.y * width)] < P.z)
            {
                zbuffer[int(P.x + P.y * width)] = P.z;
                TGAColor color = model->diffuse(uvP);
                image.set(P.x, P.y, color);
            }
        }
    }
}



/*
//the rigth function
void triangle(Vec3f *pts, float *zbuffer, TGAImage &image, Vec2i *uv)
{
    Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 2; j++)
        {
            bboxmin[j] = std::max(0.f, std::min(bboxmin[j], pts[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
        }
    }
    Vec3f P;
    for (P.x = bboxmin.x; P.x <= bboxmax.x; P.x++)
    {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y++)
        {
            Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            P.z = 0;
            for (int i = 0; i < 3; i++)
                P.z += pts[i][2] * bc_screen[i];
            if (zbuffer[int(P.x + P.y * width)] < P.z)
            {
                zbuffer[int(P.x + P.y * width)] = P.z;
                image.set(P.x, P.y, color);
            }
        }
    }
}
*/


void triangle(Vec3f t0, Vec3f t1, Vec3f t2,float *zbuffer, TGAImage &image, TGAColor color)
{
    //让三角形的三个顶点有序排列
    // 
    //    -------*-------
    //    ----*----------
    //    ----------*----
    //    上面三个点就是三个三角形，要以y的大小排列三个三角形的顺序，当我们知道三角形的三个顶点时，
    //    我们的边界就可以知道了，所以通过排序来得到三条直线的斜率，知道要计算着色哪一款地方
    // */

    if (t0.y > t1.y)
        std::swap(t0, t1);
    if (t0.y > t2.y)
        std::swap(t0, t2);
    if (t1.y > t2.y)
        std::swap(t1, t2);
    int total_height = t2.y - t0.y;
    for (int y = t0.y; y <= t1.y; y++)
    {
        int segment_height = t1.y - t0.y + 1;
        float alpha = (float)(y - t0.y) / total_height;
        float beta = (float)(y - t0.y) / segment_height; // be careful with divisions by zero
        Vec3f A = t0 + (t2 - t0) * alpha;
        Vec3f B = t0 + (t1 - t0) * beta;
        if (A.x > B.x)
            std::swap(A, B);
        for (int j = A.x; j <= B.x; j++)
        {
            Vec3f P(j,y,0);
            Vec3f bc_screen = barycentric(t0, t1,t2, P);
            // if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
            //     continue;
            P.z = 0;
            P.z = t0.z * bc_screen[0] + t1.z * bc_screen[1] + t2.z * bc_screen[2];

            if (zbuffer[int(P.x + P.y * width)] < P.z)
            {
                zbuffer[int(P.x + P.y * width)] = P.z;
                image.set(P.x, P.y, color);
            }
            image.set(j, y, color); // attention, due to int casts t0.y+i != A.y
        }
        // image.set(A.x, y, red);
        // image.set(B.x, y, green);
    }
    for (int y = t2.y; y >= t1.y; y--)
    {
        int segment_height = t2.y - t1.y + 1;
        float alpha = (float)(t2.y - y) / total_height;
        float beta = (float)(t2.y - y) / segment_height; // be careful with divisions by zero
        Vec3f A = t2 + (t1 - t2) * beta;
        Vec3f B = t2 + (t0 - t2) * alpha;
        if (A.x > B.x)
            std::swap(A, B);
        for (int j = A.x; j <= B.x; j++)
        {
            Vec3f P(j, y, 0);
            Vec3f bc_screen = barycentric(t0, t1, t2, P);
            // if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
            //     continue;
            P.z = 0;
            P.z = t0.z * bc_screen[0]+t1.z*bc_screen[1]+t2.z*bc_screen[2];
            if (zbuffer[int(P.x + P.y * width)] < P.z)
            {
                zbuffer[int(P.x + P.y * width)] = P.z;
                image.set(P.x, P.y, color);
            }
            image.set(j, y, color); // attention, due to int casts t0.y+i != A.y
        }
    }
    // line(t0, t1, image, green);
    // line(t1, t2, image, green);
    // line(t0, t2, image, red);
}

Vec3f barycentric(Vec3f A,Vec3f B,Vec3f C,Vec3f P)
{
    Vec3f s[2];
    for (int i = 2; i--;)
    {
        s[i][0] = C[i] - A[i];
        s[i][1] = B[i] - A[i];
        s[i][2] = A[i] - P[i];
    }
    Vec3f u = s[0]^s[1];
    if (std::abs(u[2]) > 1e-2) // dont forget that u[2] is integer. If it is zero then triangle ABC is degenerate
        return Vec3f(1.f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
    return Vec3f(-1, 1, 1);
}

/*
注意，这里的pts应该是2i,因为它是画在布上面，只有xy两个坐标，
这里写成Vec3f是因为需要计算重心坐标，而重心坐标是3个浮点数，类型只能为Vec3f
所以为了类型的匹配，三个点的坐标也应该转换为这个类型，不然就不能计算了
*/
/*
void triangle(Vec3f pts[3], TGAImage &img, const TGAColor color, float *zbuffer)
{
    //让三角形的三个顶点有序排列
    
    //    -------*-------
    //    ----*----------
    //    ----------*----
    //    上面三个点就是三个三角形，要以y的大小排列三个三角形的顺序，当我们知道三角形的三个顶点时，
    //    我们的边界就可以知道了，所以通过排序来得到三条直线的斜率，知道要计算着色哪一款地方
    
    if(pts[0].y > pts[1].y) std::swap(pts[0], pts[1]);
    if(pts[0].y > pts[2].y) std::swap(pts[0], pts[2]);
    if(pts[1].y > pts[2].y) std::swap(pts[1], pts[2]);

    int total_height = pts[2].y - pts[0].y;
    // int half_height = pts[1].y - pts[1].y;
    // float dt_01 = float(pts[1].x - pts[0].x) / (pts[1].y - pts[0].y);
    // float dt_02 = float(pts[2].x - pts[0].x) / (pts[2].y - pts[0].y);
    // float dt_12 = float(pts[2].x - pts[1].x) / (pts[2].y - pts[1].y);


    for (int y = pts[0].y; y<=pts[1].y;y++)
    {
        float alpha = float(y - pts[0].y) / total_height;
        float beta = float(y - pts[0].y) / (pts[1].y - pts[0].y + 1);
        Vec3f A = pts[0] + (pts[1] - pts[0]) * beta;
        Vec3f B = pts[0] + (pts[2] - pts[0]) * alpha;
        if(A.x>B.x)
            std::swap(A, B);
        for (int x = A.x; x <= B.x;x++)
        {
            Vec3f P(x,y,0);
            Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            for (int i = 0; i < 3;i++)
                P.z += pts[i][2]*bc_screen[i];
            if(P.z>zbuffer[int(y*width+x)])
            {
                zbuffer[int(y * width + x)] = P.z;
                img.set(x, y, color);
            }
        }
    }

    for (int y = pts[2].y; y >= pts[1].y;y--)
    {
        float alpha = (float)(pts[2].y - y) / total_height;
        float beta = (float)(pts[2].y - y) / (pts[2].y-pts[1].y+1);
        Vec3f A = pts[2] + (pts[1] - pts[2]) * beta;
        Vec3f B = pts[2] + (pts[0] - pts[2]) * alpha;
        if(A.x>B.x)
            std::swap(A, B);
        for (int x = A.x; x <= B.x; x++)
        {
            Vec3f P(x, y, 0);
            Vec3f bc_screen = barycentric(pts[0], pts[1], pts[2], P);
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            for (int i = 0; i < 3; i++)
                P.z += pts[i][2] * bc_screen[i];
            if (P.z > zbuffer[y * width + x])
            {
                zbuffer[y * width + x] = P.z;
                img.set(x, y, color);
            }
        }
    }
}
*/

Vec3f world2screen(Vec3f v)
{
    return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}

int main(int argc,char **argv)
{
    model = new Model("obj/african_head.obj");
    TGAImage img(width,height,TGAImage::RGB);
    //这里为什么需要new一个zbuffer？局部变量的作用域到底在哪里，为什么float zbuffer[width * height];不行
    float *zbuffer=new float[width * height];
    std::fill(zbuffer, zbuffer + width * height, -std::numeric_limits<float>::max());
    // for (int i = 0; i<width*height; i++)
    //     zbuffer[i]=-std::numeric_limits<float>::max();

    Vec3f light_dir(0, 0, -1);

    for (int i = 0;i<model->nfaces();i++)
    {
        std::vector<int> face = model->face(i);
        Vec3f pts[3];
        Vec3f world_coords[3];
        for (int j = 0; j < 3;j++)
        {
            Vec3f v = model->vert(face[j]);
            pts[j] = world2screen(v);
            // pts[j] = Vec3f((v.x+1.) * width/2., (v.y+1.) * height/2.,v.z);
            world_coords[j] = v;
        }
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light_dir;
        if(intensity>0)
        {
            Vec2i uv[3];
            for (int k = 0; k < 3;k++)
            {
                uv[k] = model->uv(i, k);
            }
            triangle(pts,zbuffer,img,uv);
        }
        // Vec2i uv[3];
        // for (int k = 0; k < 3;k++)
        // {
        //     uv[k] = model->uv(i, k);
        // }
        // triangle(pts, zbuffer, img,uv);
        //注意这两个类型要一样，都是Vec3f,不能因为light_dir中全是int就写成Vec3i，这样就不能和n相乘了,因为不是一个类型，不能相乘。
        // float intensity = n * light_dir;
        // triangle(pts, zbuffer, img, TGAColor(rand() % 255, rand() % 255, rand() % 255, 255) );
        // triangle(pts[0], pts[1], pts[2],zbuffer,img, red);
    }


    img.flip_vertically();
    img.write_tga_file("out.tga");
    delete model;
    delete[] zbuffer;
    return 0;
}