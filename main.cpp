#include <iostream>
#include <limits>
#include <cmath>
#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include <algorithm>
const int width=800;
const int height=800;

Model *model;
float *zbuffer = new float[width * height];
Vec3f light_dir(0, 0, -1);
Vec3f camera(0, 0, 3);

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
constexpr double MY_PI = 3.1415926;

Vec3f barycentric(Vec3f A, Vec3f B, Vec3f C, Vec3f P);


//首先获得这个模型是怎么旋转的，旋转角度是多少
Matrix get_model_matrix(float rotation_angle)
{
    Matrix model = Matrix::identity(4);
    float radian = (rotation_angle / 360.0) * 2 * MY_PI;
    Matrix rota;
    rota << cos(radian), -sin(radian), 0, 0,
        sin(radian), cos(radian), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;
    model = rota * model;
    return model;
}

//绕任意轴进行旋转，vec是轴的向量
Matrix get_model_randomrotate_matrix(float rotation_angle, Vec3f vec)
{
    Matrix model = Matrix::identity(4);
    vec.normalize();
    
    float radian = (rotation_angle / 360.0) * 2 * MY_PI;
    float nx = vec[0];
    float ny = vec[1];
    float nz = vec[2];
    float tem_store = 1 - cos(radian);
    // Eigen::Matrix3f rota;
    Matrix rota(3,3);
    // Eigen::Matrix3f k, I = Eigen::Matrix3f::Identity();
    Matrix k(3,3), I(3,3) = Matrix::identity(3);
    k << 0, nz, ny,
        nz, 0, -nx,
        ny, nx, 0;
    rota = cos(radian) * I + (1 - cos(radian)) * (k * k.transpose()) + sin(radian) * k;
    Matrix rotate(4,4);
    rotate << rota(0, 0), rota(0, 1), rota(0, 2), 0,
        rota(1, 0), rota(1, 1), rota(1, 2), 0,
        rota(2, 0), rota(2, 1), rota(2, 2), 0,
        0, 0, 0, 1;
    model = rotate * model;
    return model;
}

//把眼睛移到坐标原点位置，方便计算
Matrix get_view_matrix(Vec3f eye_pos)
{
    Matrix view = Matrix::identity(4);
    Matrix translate;
    translate << 1, 0, 0, -eye_pos[0],
                 0, 1, 0, -eye_pos[1],
                 0, 0, 1,-eye_pos[2],
                 0, 0, 0, 1;

    view = translate * view;
    return view;
}

Matrix get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    Matrix projection = Matrix::identity(4);

    //把视界锥变成orthotical
    Matrix presp2ortho;
    presp2ortho << zNear, 0, 0, 0,
                   0, zNear, 0, 0,
                   0, 0, zNear + zFar, -zNear * zFar,
                   0, 0, 1, 0;
    float w = tan(((eye_fov / 360.0) * 2.0 * MY_PI) / 2) * zNear * 2.0;
    float h = w * aspect_ratio;
    Matrix scale;
    scale << 2.0 / w, 0, 0, 0,
        0, 2.0 / h, 0, 0,
        0, 0, 2.0 / (zNear - zFar), 0,
        0, 0, 0, 1;
    projection = scale * presp2ortho;
    return projection;
}

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

//第一种方法画三角形，用bounding box来找到三角形的范围，遍历box中的点，通过重心坐标的方法来确定在不在pts形成三角形内
//如果存在于三角形内部，那么就对这个点着色，否则，这个点不着色。
/*
注意，这里的pts应该是2i,因为它是画在布上面，只有xy两个坐标，
这里写成Vec3f是因为需要计算重心坐标，而重心坐标是3个浮点数，类型只能为Vec3f
所以为了类型的匹配，三个点的坐标也应该转换为这个类型，不然就不能计算了
下面的第二种方法也一样
*/
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
            //如果有一个值<0就说明这个点不在pts三角形内
            if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
                continue;
            P.z = 0;
            Vec2i uvP(0, 0);
            for (int i = 0; i < 3; i++)
            {
                P.z += pts[i][2] * bc_screen[i]; //通过重心坐标计算此点z坐标的值，更新zbuffer
                //去得到这个点的纹素
                uvP.x += uv[i][0] * bc_screen[i];
                uvP.y += uv[i][1] * bc_screen[i];
            }
            //如果z大，说明这个z在视线前面，应该去着色
            if (zbuffer[int(P.x + P.y * width)] < P.z)
            {
                zbuffer[int(P.x + P.y * width)] = P.z;
                TGAColor color = model->diffuse(uvP);
                image.set(P.x, P.y, color);
            }
        }
    }
}




//第二种画三角形的方法
void triangle(Vec3f t0, Vec3f t1, Vec3f t2,float *zbuffer, TGAImage &image, Vec2i *uv)
{
    //让三角形的三个顶点有序排列
    // 
    //    -------*-------t2
    //    ----*----------t1
    //    ----------*----t0
    //    上面三个点就是三个三角形，要以y的大小排列三个三角形的顺序，当我们知道三角形的三个顶点时，
    //    我们的边界就可以知道了，所以通过排序来得到三条直线的斜率，知道要计算着色哪一款地方
    // 

    //下面使得三点按t0.y<t1.y<t2.y的顺序排列
    if (t0.y > t1.y)
        std::swap(t0, t1);
    if (t0.y > t2.y)
        std::swap(t0, t2);
    if (t1.y > t2.y)
        std::swap(t1, t2);
    
    int total_height = t2.y - t0.y;
    //这里只是画了三角形的下半区域
    for (int y = t0.y; y <= t1.y; y++)
    {
        int segment_height = t1.y - t0.y + 1; //这里加一是为了防止除以0
        float alpha = (float)(y - t0.y) / total_height;
        float beta = (float)(y - t0.y) / segment_height; // be careful with divisions by zero
        //一个点t的坐标为：基点t0+向量，向量长度为|t-t0|,方向为t-t0
        Vec3f A = t0 + (t2 - t0) * alpha;
        Vec3f B = t0 + (t1 - t0) * beta;
        //因为需要从x小值到大值，所以需要知道那个小，哪个大
        if (A.x > B.x)
            std::swap(A, B);
        for (int j = A.x; j <= B.x; j++)
        {
            Vec3f P(j,y,0);
            //计算这个单纯是为了去算P.z和uvP坐标，因为上面保证了这个P点一定是在三角形内部的
            Vec3f bc_screen = barycentric(t0, t1,t2, P);
            // if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0)
            //     continue;
            P.z = 0;
            P.z = t0.z * bc_screen[0] + t1.z * bc_screen[1] + t2.z * bc_screen[2];
            Vec2i uvP(0, 0);
            for (int i = 0; i < 3;i++)
            {
                uvP[0] += uv[i][0] * bc_screen[i];
                uvP[1] += uv[i][1] * bc_screen[i];
            }
            TGAColor color = model->diffuse(uvP);
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

    //这个是画三角形的上半部分区域
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
            Vec2i uvP(0, 0);
            for (int i = 0; i < 3; i++)
            {
                uvP[0] += uv[i][0] * bc_screen[i];
                uvP[1] += uv[i][1] * bc_screen[i];
            }
            TGAColor color = model->diffuse(uvP);
            if (zbuffer[int(P.x + P.y * width)] < P.z)
            {
                zbuffer[int(P.x + P.y * width)] = P.z;
                image.set(P.x, P.y, color);
            }
            image.set(j, y, color); // attention, due to int casts t0.y+i != A.y
        }
    }
    //这两个循环是可以写成一个的，但是为了程序的可读性，就先放在这里了。
    // line(t0, t1, image, green);
    // line(t1, t2, image, green);
    // line(t0, t2, image, red);
}

Vec3f barycentric(Vec3f A,Vec3f B,Vec3f C,Vec3f P)
{
    //这里的计算公式详见https://zhuanlan.zhihu.com/p/111540355
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


Vec3f world2screen(Vec3f v)
{
    return Vec3f(int((v.x + 1.) * width / 2. + .5), int((v.y + 1.) * height / 2. + .5), v.z);
}

int main(int argc,char **argv)
{
    model = new Model("obj/african_head.obj");
    TGAImage img(width,height,TGAImage::RGB);
    //这里为什么需要new一个zbuffer？局部变量的作用域到底在哪里，为什么float zbuffer[width * height];不行
    
    std::fill(zbuffer, zbuffer + width * height, -std::numeric_limits<float>::max());


    for (int i = 0;i<model->nfaces();i++)
    {
        std::vector<int> face = model->face(i);
        Vec3f pts[3];
        Vec3f world_coords[3];
        for (int j = 0; j < 3;j++)
        {
            Vec3f v = model->vert(face[j]);
            // pts[j] = world2screen(v);
            //一定要注意数据中的类型应该是什么样子的，比如，pts是三维坐标系中的点，所有的值都是float，
            //但是换算到屏幕时，pts的x,y坐标应该是整数，因为pts的x，y坐标是img图上的坐标，
            //这个坐标只有整数，所以应该为int类型，但是z坐标是用来计算zbuffer的，需要精确，不需要换成int型
            pts[j] = Vec3f(int((v.x+1.) * width/2.+.5), int((v.y+1.) * height/2.+.5),v.z);
            world_coords[j] = v;
        }
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        //注意这两个类型要一样，都是Vec3f,不能因为light_dir中全是int就写成Vec3i，这样就不能和n相乘了,因为不是一个类型，不能相乘。
        float intensity = n * light_dir;
        if(intensity>0)
        {
            Vec2i uv[3];
            for (int k = 0; k < 3;k++)
            {
                uv[k] = model->uv(i, k);
            }
            // triangle(pts,zbuffer,img,uv);
            triangle(pts[0], pts[1], pts[2],zbuffer,img, uv);
        }
    }


    img.flip_vertically();
    img.write_tga_file("out.tga");
    delete model;
    delete[] zbuffer;
    return 0;
}