// LionArray — basic usage
//
// Demonstrates Array<T>, Queue<T> and Stack<T> with a trivially-copyable
// element type (int). Open the Serial Monitor at 115200 baud.

#include <Array.h>

static void printArray(const char *label, const Array<int> &a)
{
    Serial.print(label);
    Serial.print(" (len=");
    Serial.print(a.Length());
    Serial.print("): ");
    for (int i = 0; i < a.Length(); i++)
    {
        Serial.print(a[i]);
        Serial.print(' ');
    }
    Serial.println();
}

void setup()
{
    Serial.begin(115200);
    delay(300);
    Serial.println();
    Serial.println("=== LionArray demo ===");

    // ---- Array ----
    Array<int> a;          // default capacity 8, length 0
    a += 30;
    a += 10;
    a += 20;               // append
    a.Insert(15, 1);       // -> 30 15 10 20
    a.Remove(0);           // -> 15 10 20
    printArray("array", a);

    // Sort with a lambda comparator (no std::function, no allocation)
    a.Sort([](const int &x, const int &y) { return x - y; });
    printArray("sorted", a);

    Serial.print("Find(10) -> index ");
    Serial.println(a.Find(10));

    // ---- Queue (FIFO) ----
    Queue<int> q;
    q.Push(1);
    q.Push(2);
    q.Push(3);
    int v;
    Serial.print("queue pop order: ");
    while (q.TryPop(v))    // TryPop: false when empty (unambiguous)
    {
        Serial.print(v);
        Serial.print(' ');
    }
    Serial.println();

    // ---- Stack (LIFO) ----
    Stack<int> s;
    s.Push(1);
    s.Push(2);
    s.Push(3);
    Serial.print("stack pop order: ");
    while (s.TryPop(v))
    {
        Serial.print(v);
        Serial.print(' ');
    }
    Serial.println();

    Serial.println("=== done ===");
}

void loop()
{
}
