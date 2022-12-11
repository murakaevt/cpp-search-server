// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:

// Закомитьте изменения и отправьте их в свой репозиторий.
#include <iostream>
#include <string>
#include <algorithm> // Закомитьте изменения и отправьте их в свой репозиторий.
using namespace std;
int main() {
	int three_count = 0;
	for (int three = 0; three <= 1000; ++three) {
		string s = to_string(three);
		for (char i : s) {
			if (i == '3') {
				++three_count;
				break;
			}
		}
	}
	cout << three_count;
	return 0;
}