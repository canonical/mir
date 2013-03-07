#include <iostream>

int main()
{
    auto x = new int{5};
    delete x;
    std::cout << *x << std::endl;
}
