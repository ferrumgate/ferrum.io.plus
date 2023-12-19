#include "common/common.h"

struct A
{
    int a;
    virtual void f()
    {
        std::cout << "executing A" << std::endl;
    }
};
struct B : public A
{
    int b;
    virtual void f()
    {
        std::cout << "executing B" << std::endl;
    }
};

int main()
{
    std::vector<int> test;
    std::cout << test.size() << " " << test.capacity() << std::endl;
    test.reserve(100);
    std::cout << test.size() << " " << test.capacity() << std::endl;
    *(test.data()) = 10;
    std::cout << test.size() << " " << test.capacity() << std::endl;
    test.resize(1);
    std::cout << test.size() << " " << test.capacity() << " " << *(test.data())
              << std::endl;

    return 0;
}