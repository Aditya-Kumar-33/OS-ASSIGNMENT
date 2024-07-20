#include <stdio.h>
#include <stdlib.h>

// Function to calculate the nth Fibonacci number
long long fibonacci(int n)
{
    if (n <= 1)
        return n;

    long long fib[n + 1];
    fib[0] = 0;
    fib[1] = 1;

    for (int i = 2; i <= n; i++)
    {
        fib[i] = fib[i - 1] + fib[i - 2];
    }

    return fib[n];
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    if (n < 0)
    {
        printf("Invalid input. Please enter a non-negative integer.\n");
        return 1;
    }

    long long result = fibonacci(n);
    printf("fib(%d) = %lld\n", n, result);

    return 0;
}
