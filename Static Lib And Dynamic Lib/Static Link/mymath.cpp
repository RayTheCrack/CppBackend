#include "mymath.h"

static long long MOD = 1000000007;

void SetMod(long long x)
{
    MOD = x;
}

long long Add(long long x,long long y)
{
    x += y;
    if(x >= MOD) x %= MOD;
    if(x < 0) x += MOD;
    return x;
}

long long Mul(long long x,long long y)
{
    return ((x % MOD) * (y % MOD)) % MOD;
}

long long QuickPower(long long x,long long y)
{
    long long res = 1;
    while(y)
    {
        if(y & 1) res = (res * x) % MOD;
        x = (x * x) % MOD;
        y >>= 1;
    }
    return res;
}

long long Inv(long long x)
{
    return QuickPower(x,MOD - 2);
}

long long Divide(long long x,long long y)
{
    return Mul(x,Inv(y));
}
