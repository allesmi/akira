#include "fixed-point.h"
#include "lib/debug.h"
#include <inttypes.h>

fixed_point to_fixed_point(int n)
{
	return n * power(2, EXPONENT);
}

int to_int(fixed_point x)
{
	return x / power(2, EXPONENT);
}

int to_int_nearest(fixed_point x)
{
	if(x >= 0)
	{
		return ((x + power(2, EXPONENT - 1)) / power(2, EXPONENT));
	}

	return ((x - power(2, EXPONENT - 1)) / power(2, EXPONENT));
}

fixed_point add(fixed_point x, fixed_point y)
{
	return x + y;
}

fixed_point subtract(fixed_point x, fixed_point y)
{
	return x - y;
}

fixed_point add_int(fixed_point x, int n)
{
	return x + n * power(2, EXPONENT);
}

fixed_point sub_int(fixed_point x, int n)
{
	return x - n * power(2, EXPONENT);;
}

fixed_point multi(fixed_point x, fixed_point y)
{
	return ((int64_t) x) * y / power(2, EXPONENT);
}

fixed_point multi_int(fixed_point x, int n)
{
	return x * n;
}

fixed_point div(fixed_point x, fixed_point y)
{
	ASSERT(y != 0);
	return ((int64_t) x) * power(2, EXPONENT) / y;
}

fixed_point div_int(fixed_point x, int n)
{	
	ASSERT(n != 0);
	return x / n;;
}

fixed_point max(fixed_point x, fixed_point y)
{
	return (x > y) ? x : y;
}


fixed_point min(fixed_point x, fixed_point y)
{
	return (x < y) ? x : y;
}

int power(int base, int exponent)
{
	
	if(base != 0)
	{
		int pow = 1;
		int i;

		for(i = 0; i < exponent; i++)
		{
			pow = pow * base;
		}

		return pow;
	}
	
	return 1;
}