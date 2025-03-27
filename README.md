# fork-diff-myers

Сборка: 
```bash
g++ -std=c++17 -o diff_app main.cpp UniversalTokenizer.cpp MyersDiff.cpp
```

Применение: 
```bash
./diff_app old.txt new.txt
```
либо
```bash
./diff_app
```

Тесты: 
```bash
g++ -std=c++17 -o run_tests tests.cpp UniversalTokenizer.cpp MyersDiff.cpp
./run_tests
```
