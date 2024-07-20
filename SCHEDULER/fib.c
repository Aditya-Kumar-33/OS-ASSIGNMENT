#include <stdio.h>
#include <stdlib.h>

// Function to calculate the nth Fibonacci number
long long fibonacci(int n)
{
    if (n <= 1)
        return n;

    return fibonacci(n-1)+fibonacci(n-2);
}

int main()
{
    int n=45;

    if (n < 0)
    {
        printf("Invalid input. Please enter a non-negative integer.\n");
        return 1;
    }

    long long result = fibonacci(n);
    printf("fib(%d) = %lld\n", n, result);

    return 0;
}
