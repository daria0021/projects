# Инструкция по быстрому старту для MovDino Interpreter

## Компиляция программы

### Linux/macOS:
```bash
gcc -o movdino movdino.c
```

### Windows (MinGW):
```bash
gcc -o movdino.exe movdino.c
```

### Windows (Visual Studio):
```bash
cl movdino.c
```

## Базовый запуск

### Минимальная команда:
```bash
./movdino input.txt output.txt
```

### Пример структуры input.txt:
```
SIZE 10 10
START 5 5
MOVE RIGHT
PAINT a
MOVE DOWN
DIG LEFT
```

## Параметры командной строки

### Основные опции:

| Опция | Назначение | Пример |
|-------|------------|---------|
| `interval N` | Задержка между шагами (секунды) | `interval 2` |
| `no-display` | Отключить визуализацию в консоли | `no-display` |
| `no-save` | Отключить сохранение в файл | `no-save` |

### Примеры использования:

**С задержкой 2 секунды между шагами:**
```bash
./movdino input.txt output.txt interval 2
```

**Без отображения в консоли (только сохранение):**
```bash
./movdino input.txt output.txt no-display
```

**Только просмотр без сохранения:**
```bash
./movdino input.txt output.txt no-save
```

**Комбинация опций:**
```bash
./movdino input.txt output.txt no-display interval 0
```

## Примеры входных файлов

### Простой пример (input_simple.txt):
```
SIZE 12 12
START 3 3
MOVE RIGHT
MOVE RIGHT
PAINT b
MOVE DOWN
DIG LEFT
MOUND UP
JUMP RIGHT 3
```

### Пример с объектами (input_objects.txt):
```
SIZE 15 15
START 2 2
GROW RIGHT
GROW DOWN
MAKE LEFT
MOVE RIGHT
PUSH RIGHT
CUT DOWN
PAINT g
```

### Пример с условными командами (input_conditional.txt):
```
SIZE 10 10
START 0 0
MOVE RIGHT
PAINT x
IF CELL 1 0 IS x THEN MOVE DOWN
IF CELL 1 1 IS _ THEN DIG RIGHT
```

## Проверка работы

1. **Создайте тестовый файл:**
```bash
echo "SIZE 8 8
START 2 2
MOVE RIGHT
PAINT z
MOVE DOWN
DIG LEFT" > test_input.txt
```

2. **Запустите программу:**
```bash
./movdino test_input.txt test_output.txt
```

3. **Проверьте результат:**
```bash
cat test_output.txt
```

## Советы по использованию

### Для отладки:
```bash
# Быстрый просмотр без задержек
./movdino input.txt output.txt interval 0

# Только сохранение без отображения
./movdino input.txt output.txt no-display
```

### Для демонстрации:
```bash
# Медленное выполнение с визуализацией
./movdino input.txt output.txt interval 2
```

### Для обработки больших файлов:
```bash
# Максимальная производительность
./movdino large_input.txt output.txt no-display interval 0
```

## Структура выходного файла

Программа создает файл с конечным состоянием поля:
- `#` - позиция динозавра
- `_` - пустая клетка  
- `%` - яма
- `^` - гора
- `&` - дерево
- `@` - камень
- `a-z` - окрашенные клетки

## Устранение неполадок

**Проблема**: "Error: cannot open 'input.txt'"
**Решение**: Убедитесь, что файл существует в текущей директории

**Проблема**: "Error: leading spaces"
**Решение**: Удалите пробелы в начале строк команд

**Проблема**: Программа завершается с ошибкой
**Решение**: Проверьте синтаксис команд в input.txt

**Проблема**: Нет визуализации в Windows
**Решение**: Используйте `no-display` или убедитесь, что терминал поддерживает очистку экрана

Эта инструкция позволит быстро начать работу с интерпретатором MovDino и освоить основные возможности программы.
