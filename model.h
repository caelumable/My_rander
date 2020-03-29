#ifndef _MODEL_H_
#define _MODEL_H_

#include "tgaimage.h"
#include "geometry.h"
#include <vector>
class Model
{
    private:
        std::vector<Vec3f> verts_; //ÿһ�����㶼����������ϵ�£��������������͵�����
        std::vector<std::vector<Vec3i> > faces_;//ÿһ��������������Vec3i�����꣬face[0]Ϊ���㣬face[1]Ϊvt,face[2]Ϊvn
        std::vector< Vec3f > norms_;//vn
        std::vector<Vec2f> uv_;//��������
        TGAImage diffusemap_;//����ͼ��texture
        void load_texture(const char *filename, const char *suffix, TGAImage &img);

    public:
        Model(const char *filename);
        ~Model();
        int nverts();//�������
        int nfaces();//��������ĸ���
        Vec3f vert(int i);//����һ�������εĵ�i������
        TGAColor diffuse(Vec2i uv);//�õ������������ɫ
        Vec2i uv(int iface, int nvert);//�����������������ͼ�ϵ�λ��
        std::vector<int> face(int idx);//���������������������ļ��е�λ��,���vector��СΪ3
};


#endif