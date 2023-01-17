// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>

using namespace std;

int main() {
    int count = 0;
    for (int i=1; i<=1000; ++i) {
        string word = to_string(i);
        for (char c : word) {
            if (c == '3') {
                ++count;
                break;
            }
        }
    }
    cout << count;
    return 0;
}
// Закомитьте изменения и отправьте их в свой репозиторий.
