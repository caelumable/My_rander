#ifndef _GEOMETRY_H_
#define _GEOMETRY_H_

#include <cmath>
#include <iostream>

////////////////////////////////////////////////////////////////////////
template <class T> 
struct Vec2
{
    //使用联合体的第一个好处是可以节省空间，第二个好处是可以在三种形式中任选一，而不必修改名称，比如x=0后，u也等于0，raw[0]也为0
    union{
        struct {T x,y;};
        struct {T u,v;};
        T raw[2];
    };
    Vec2(T _x,T _y):x(_x),y(_y){};
    Vec2():x(0),y(0){};
    Vec2<T> operator+(const Vec2<T >& vec1){return Vec2(x+vec1.x,y+vec1.y);};
    //注意一下这里返回的应该是什么类型，需要加饮用吗？
    Vec2<T> operator+=(Vec2<T>& vec1) {
        x += vec1.x;
        y += vec1.y;
        return *this;
    };
    Vec2<T > operator-(const Vec2<T >& vec1){return Vec2(x-vec1.x,y-vec1.y);};
    Vec2<T> operator-=(Vec2<T> &vec1)
    {
        x -= vec1.x;
        y -= vec1.y;
        return *this;
    };
    Vec2<T > operator*(float k){return Vec2(x*k,y*k);};
    T& operator[](int ind){return raw[ind];};
    T operator*(Vec2<T > v){return x*v.x+y*v.y;};
    template <class > friend std::ostream& operator<<(std::ostream&s,Vec2<T >& v);
};

template <class T > std::ostream& operator<<(std::ostream &s,Vec2<T>& v)
{
    s<<"("<<v.x<<","<<v.y<<")";
    return s;
}

/////////////////////////////////////////////////////////////////////////////

template <class T >
struct Vec3
{
    union{
        struct {T x,y,z;};
        struct {T ivert, iuv, inorm; };
        T raw[3];
    };

    Vec3():x(0),y(0),z(0){};
    Vec3(T _x,T _y,T _z):x(_x),y(_y),z(_z){};
    Vec3<T> operator+(const Vec3<T> &v){return Vec3<T>(x+v.x,y+v.y,z+v.z);};
    Vec3<T > operator+=(const Vec3<T> &v){v.x+=x,v.y+=y,v.z+=z; return *this;};
    Vec3< T> operator-(const Vec3<T> &v){return Vec3<T>(x-v.x,y-v.y,z-v.z);};
    Vec3<T> operator-=(const Vec3<T> &v){v.x-=x,v.y-=y,v.z-=z; return *this;};
    Vec3<T> operator*(float v){return Vec3<T>(x*v,y*v,z*v);};
    T operator*(const Vec3<T> &v){return x*v.x+y*v.y+z*v.z;};
    T& operator[](int ind){return raw[ind];};
    Vec3<T> operator^(const Vec3<T> &v){return Vec3<T>(y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x);};
    float norm(){return std::sqrt(x*x+y*y+z*z);};
    Vec3<T>& normalize(T l=1){*this=(*this)*float(l/norm());return *this;};
    template <class > friend std::ostream& operator<<(std::ostream&s,Vec3<T>& v);

};

template <class T>
std::ostream& operator<<(std::ostream&s,Vec3<T>& v)
{
    s<<"("<<v.x<<","<<v.y<<","<<v.z<<")";
    return s;
}

////////////////////////////////////////////////////////////////////////////////////////////

typedef Vec2<int> Vec2i ;
typedef Vec2<float> Vec2f ;
typedef Vec3<int> Vec3i ;
typedef Vec3<float>  Vec3f ;
///////////////////////////////////////////////////////////////////////////////////////////
const int DEFAULT_ALLOC = 4;
class Matrix
{
    std::vector<std::vector<float> > m;
    int rows, cols;

public:
    Matrix(int r = DEFAULT_ALLOC, int c = DEFAULT_ALLOC);
    inline int nrows();
    inline int ncols();

    
    static Matrix identity(int dimensions);
    std::vector<float> &operator[](const int i);
    Matrix operator*(const Matrix &a);
    Matrix operator+(const Matrix &a);
    Matrix operator-(const Matrix &a);
    Matrix operator+=(const Matrix &a);
    Matrix operator-=(const Matrix &a);
    Matrix transpose();
    Matrix inverse();

    friend std::ostream &operator<<(std::ostream &s, Matrix &m);
};

#endif