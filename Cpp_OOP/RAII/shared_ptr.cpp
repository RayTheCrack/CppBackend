#include <iostream>
#include <memory>
#include <stdexcept>

using namespace std;

class Ball
{
    public:
        Ball() { cout << "A ball has been created." << endl; }
        ~Ball() { cout << "A ball has been destroyed." << endl; }
        void bounce() { cout << "The ball is bouncing!" << endl; }
};

int main()
{   
    shared_ptr<Ball> p = make_shared<Ball>();
    cout << p.use_count() << endl;
    shared_ptr<Ball> p2 = p;
    cout << p.use_count() << ' ' << p2.use_count() << endl;
    shared_ptr<Ball> p3 = p2;
    cout << p.use_count() << ' ' << p2.use_count() << ' ' << p3.use_count() << endl;
    p->bounce();
    p2->bounce();
    p3->bounce();
    p.reset();
    cout << p2.use_count() << ' ' << p3.use_count() << ' ' << endl;
    p2.reset();
    cout << p3.use_count() << endl;
    p3.reset();
    return 0;
}