#include <list>

class Calculator {
private:
    std::list<double> values;
public:
    void push(double);
    double add();
    double divide();
};

