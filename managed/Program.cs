using System;
using System.Runtime.InteropServices;

class Program
{
    [DllImport("hello")]
    static extern int add(int a, int b);

    [DllImport("hello")]
    static extern void greet(string name);

    static void Main()
    {
        greet("Safi");
        int result = add(3, 4);
        Console.WriteLine($"3 + 4 = {result}");
    }
}
