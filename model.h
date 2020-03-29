#ifndef _MODEL_H_
#define _MODEL_H_

#include "tgaimage.h"
#include "geometry.h"
#include <vector>
class Model
{
    private:
        std::vector<Vec3f> verts_; //每一个顶点都是世界坐标系下，有三个浮点类型的数字
        std::vector<std::vector<Vec3i> > faces_;//每一个三角形有三组Vec3i的坐标，face[0]为顶点，face[1]为vt,face[2]为vn
        std::vector< Vec3f > norms_;//vn
        std::vector<Vec2f> uv_;//纹素坐标
        TGAImage diffusemap_;//纹理图，texture
        void load_texture(const char *filename, const char *suffix, TGAImage &img);

    public:
        Model(const char *filename);
        ~Model();
        int nverts();//顶点个数
        int nfaces();//三角形面的个数
        Vec3f vert(int i);//返回一个三角形的第i个顶点
        TGAColor diffuse(Vec2i uv);//得到像素坐标的颜色
        Vec2i uv(int iface, int nvert);//获得纹素坐标在这张图上的位置
        std::vector<int> face(int idx);//返回三角形三个顶点在文件中的位置,这个vector大小为3
};


#endif