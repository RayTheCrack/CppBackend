#include <iostream>
using namespace std;

// 前置声明：告诉编译器 Point 是模板类，operator+/{<<}/{>>} 是模板函数
template<typename T> class Point;
template<typename T> Point<T> operator+(const Point<T>& a, const Point<T>& b);
template<typename T> ostream& operator<<(ostream& out, const Point<T>& p);
template<typename T> istream& operator>>(istream& in, Point<T>& p);

template<typename T>
class Point
{
	private:
		T x,y;
	public:
		Point() : x(0), y(0) {}
		Point(T a, T b) : x(a), y(b) {}
		friend Point operator+<>(const Point<T>& x,const Point<T>& y);
		friend ostream& operator<<<>(ostream& out, const Point<T>& p);
		friend istream& operator>><>(istream& in, Point<T>& p);
};
template<typename T>
Point<T> operator+(const Point<T>& a,const Point<T>& b)
{
	return Point<T>(a.x + b.x, a.y + b.y);
}

template<typename T>
istream& operator>>(istream& in, Point<T>& p)
{
	in >> p.x >> p.y;
	return in;
}

template<typename T>
ostream& operator<<(ostream& out, const Point<T>& p)
{
	out << "(" << p.x << "," << p.y << ")";
	return out;
};

int main()
{
	Point<double> a,b;
	cin >> a >> b;
	cout << a << b << a + b << endl;
	return 0;
}

