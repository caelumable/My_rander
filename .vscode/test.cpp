#include <iostream>
using namespace std;

void test(int *zbuffer)
{
    for (int i = 0; i < 10;i++)
        zbuffer[i] += i;
}

int main()
{
    int zbuffer[10];
    for (int i = 0; i < 10;i++)
        zbuffer[i] = 0;
    test(zbuffer);
    for (int i = 0; i < 10;i++)
    {
        cout << zbuffer[i];
    }
    const char *path = "/obj/african_head.obj";
    string filename(path);
    cout << filename.find_last_of(".");
    return 0;
}