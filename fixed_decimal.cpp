#include <iostream>

#include "ivymike/decimal.h"

using ivy_mike::decimal;
int main() {
    decimal<2> a(10.4);
    decimal<2> b(2);
    
    std::cout << (a * b)<< "\n";
    std::cout << (a / b)<< "\n";
    std::cout << (a + b)<< "\n";
    std::cout << (a - b)<< "\n";
    
    std::cout << (a >= b) << "\n";
    
    decimal<2> c = 10.4000000001;
    std::cout << "c: " << c << "\n";
    
    std::cout << (a > c) << "\n";
    std::cout << (a >= 10.401) << "\n";
    
}