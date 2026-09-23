<img width="1440" height="520" alt="performance_scaling" src="https://github.com/user-attachments/assets/1d33fd35-c9fe-4362-aff4-e7beabc5a444" />
# Smart pointers
<img width="1400" height="945" alt="performance_ratio" src="https://github.com/user-attachments/assets/5f8bff89-a443-4764-ac70-09926676d95a" />

## Сборка и запуск

```bash
cd SmartPointers
make clean
make
```

Обычная команда `make` создаёт три запускаемых файла:

```bash
./smart_pointers          # AddressSanitizer + UndefinedBehaviorSanitizer
./smart_pointers_leaks    # проверка утечек утилитой leaks в macOS
./smart_pointers_release  # release-сборка для измерения производительности
```

Для запуска тестов производительности:

```bash
./smart_pointers_release
```

После запуска нужно выбрать пункт `6`. Результаты выводятся в терминал и автоматически сохраняются в два файла:

```text
performance_results.csv
performance_results.md
```

`CSV` подходит для Excel, Numbers и дальнейшего построения графиков. `Markdown` содержит готовую таблицу, которую можно открыть прямо в GitHub или VS Code.

## Назначение проекта

Проект содержит учебные реализации трёх умных указателей C++20:

| Класс | Назначение | Аналог стандартной библиотеки |
|---|---|---|
| `UnqPtr<T>` | Единоличное владение объектом | `std::unique_ptr<T>` |
| `ShrdPtr<T>` | Совместное владение объектом со счётчиком сильных ссылок | `std::shared_ptr<T>` |
| `WeakPtr<T>` | Наблюдение за объектом без продления его времени жизни | `std::weak_ptr<T>` |

Для всех трёх типов предусмотрены специализации для массивов: `UnqPtr<T[]>`, `ShrdPtr<T[]>` и `WeakPtr<T[]>`.

## Структура проекта

```text
SmartPointers/
├── Include/
│   ├── UnqPtr.hpp
│   ├── ShrdPtr.hpp
│   ├── WeakPtr.hpp
│   ├── DynamicArray.hpp
│   └── TestObject.hpp
├── Tests/
│   ├── UnqPtrTests.hpp
│   ├── ShrdPtrTests.hpp
│   ├── PolymorphismTests.hpp
│   └── PerformanceTests.hpp
├── Ui/
│   ├── Menu.h
│   └── Menu.cpp
├── Src/
│   └── main.cpp
├── Results/
│   ├── reference_results.csv
│   └── reference_results.md
├── docs/
│   ├── performance_ratio.svg
│   └── performance_scaling.svg
├── Tools/
│   └── generate_performance_charts.py
└── makefile
```

## Реализованные указатели

### UnqPtr

`UnqPtr` хранит единственный владеющий указатель. Копирование запрещено, потому что после копирования у одного объекта оказалось бы два независимых владельца. Передать владение можно перемещением.

```cpp
UnqPtr<int> first(new int(10));
UnqPtr<int> second(std::move(first));

// first пустой, объектом владеет second
```

Класс поддерживает `get`, `operator*`, `operator->`, `operator bool`, `release`, `reset` и `swap`. Специализация `UnqPtr<T[]>` вызывает `delete[]` и предоставляет `operator[]`.

### ShrdPtr

Копии `ShrdPtr` разделяют объект и один управляющий блок:

```cpp
struct ShrdPtrControlBlock {
    std::size_t StrongCount = 1;
    std::size_t WeakCount = 1;
};
```

`StrongCount` показывает количество сильных владельцев. Когда он становится равен нулю, управляемый объект удаляется. Сам управляющий блок продолжает существовать, если на него ещё ссылаются слабые указатели.

```cpp
ShrdPtr<int> first(UnqPtr<int>(new int(10)));
ShrdPtr<int> second(first);

// first.UseCount() == 2
// second.UseCount() == 2
```

### WeakPtr

`WeakPtr` использует тот же управляющий блок, но не увеличивает `StrongCount`. Поэтому слабая ссылка не мешает удалить объект и разрывает циклы совместного владения.

```cpp
struct Employee {
    WeakPtr<Employee> Colleague;
};

ShrdPtr<Employee> first(UnqPtr<Employee>(new Employee));
ShrdPtr<Employee> second(UnqPtr<Employee>(new Employee));

first->Colleague = second;
second->Colleague = first;

// У каждого объекта остаётся ровно один сильный владелец.
// При выходе из области видимости оба объекта будут удалены.
```

Метод `expired()` сообщает, уничтожен ли объект. Метод `lock()` возвращает временный `ShrdPtr`, если объект ещё существует, либо пустой `ShrdPtr`.

```cpp
ShrdPtr<Employee> Colleague = first->Colleague.lock();

if (Colleague) {
    // Внутри этого блока объект гарантированно существует.
}
```

Реализация рассчитана на однопоточное использование. Счётчики не являются атомарными.

## Безопасность преобразований

Одиночные указатели разрешают преобразование `Derived*` в `Base*`, если оно допустимо в C++:

```cpp
template <typename U>
requires std::convertible_to<U*, T*>
```

При удалении производного объекта через базовый указатель базовый класс должен иметь виртуальный деструктор.

Для массивов используется более строгое условие:

```cpp
template <typename U>
requires std::convertible_to<U(*)[], T(*)[]>
```

Оно запрещает опасное преобразование массива `Derived[]` в `Base[]`. У элементов могут отличаться размеры, поэтому арифметика `Base*` неверно вычисляла бы адреса элементов массива `Derived`.

## Тесты корректности

Интерактивное меню позволяет отдельно запустить:

| Пункт | Проверка |
|---:|---|
| 3 | `UnqPtr`, включая перемещение, `release`, `reset`, `swap` и массивы |
| 4 | `ShrdPtr`, включая копирование, счётчики, перемещение, освобождение и массивы |
| 5 | Преобразования между производными и базовыми типами |
| 7 | Все наборы тестов корректности последовательно |

Для поиска ошибок времени выполнения используется сборка `./smart_pointers`:

- AddressSanitizer обнаруживает выход за границы памяти, использование освобождённой памяти и двойное удаление;
- UndefinedBehaviorSanitizer обнаруживает ряд операций с неопределённым поведением;
- `./smart_pointers_leaks` запускает debug-сборку через системную утилиту `leaks` в macOS.

Отсутствие сообщения санитайзера означает, что в выполненных ветках программы ошибка не обнаружена. Для полной проверки нужно запускать все пункты меню.

## Тесты производительности
<svg xmlns="http://www.w3.org/2000/svg" width="1400" height="945" viewBox="0 0 1400 945">
<rect width="100%" height="100%" fill="#0d1117"/>
<style>text{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Arial,sans-serif}</style>
<text x="50" y="48" fill="#f0f6fc" font-size="28" font-weight="700">Моя реализация относительно std</text>
<text x="50" y="80" fill="#8b949e" font-size="17">N = 100 000; меньше 1 — моя реализация быстрее, больше 1 — медленнее</text>
<line x1="516.62" y1="100" x2="516.62" y2="883" stroke="#30363d" stroke-width="1"/>
<text x="516.62" y="919" fill="#8b949e" font-size="15" text-anchor="middle">0.2</text>
<line x1="824.38" y1="100" x2="824.38" y2="883" stroke="#30363d" stroke-width="1"/>
<text x="824.38" y="919" fill="#8b949e" font-size="15" text-anchor="middle">0.5</text>
<line x1="1057.19" y1="100" x2="1057.19" y2="883" stroke="#f0f6fc" stroke-width="2"/>
<text x="1057.19" y="919" fill="#8b949e" font-size="15" text-anchor="middle">1</text>
<line x1="1290.00" y1="100" x2="1290.00" y2="883" stroke="#30363d" stroke-width="1"/>
<text x="1290.00" y="919" fill="#8b949e" font-size="15" text-anchor="middle">2</text>
<text x="402" y="136" fill="#c9d1d9" font-size="15" text-anchor="end">UnqPtr · создание и освобождение</text>
<rect x="1057.19" y="120" width="17.32" height="22" rx="5" fill="#f0883e"/>
<text x="1082.51" y="137" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="start">1.053</text>
<text x="402" y="174" fill="#c9d1d9" font-size="15" text-anchor="end">UnqPtr · перемещение</text>
<rect x="1053.37" y="158" width="3.82" height="22" rx="5" fill="#2f81f7"/>
<text x="1045.37" y="175" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.989</text>
<text x="402" y="212" fill="#c9d1d9" font-size="15" text-anchor="end">UnqPtr · reset последнего владельца</text>
<rect x="1057.19" y="196" width="22.69" height="22" rx="5" fill="#f0883e"/>
<text x="1087.88" y="213" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="start">1.070</text>
<text x="402" y="250" fill="#c9d1d9" font-size="15" text-anchor="end">UnqPtr · get и разыменование</text>
<rect x="1057.19" y="234" width="4.21" height="22" rx="5" fill="#f0883e"/>
<text x="1069.40" y="251" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="start">1.013</text>
<text x="402" y="288" fill="#c9d1d9" font-size="15" text-anchor="end">ShrdPtr · создание и освобождение</text>
<rect x="1054.87" y="272" width="2.32" height="22" rx="5" fill="#2f81f7"/>
<text x="1046.87" y="289" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.993</text>
<text x="402" y="326" fill="#c9d1d9" font-size="15" text-anchor="end">ShrdPtr · перемещение</text>
<rect x="1057.19" y="310" width="36.63" height="22" rx="5" fill="#f0883e"/>
<text x="1101.82" y="327" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="start">1.115</text>
<text x="402" y="364" fill="#c9d1d9" font-size="15" text-anchor="end">ShrdPtr · reset последнего владельца</text>
<rect x="1026.84" y="348" width="30.35" height="22" rx="5" fill="#2f81f7"/>
<text x="1018.84" y="365" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.914</text>
<text x="402" y="402" fill="#c9d1d9" font-size="15" text-anchor="end">ShrdPtr · копирование и reset копий</text>
<rect x="1001.97" y="386" width="55.22" height="22" rx="5" fill="#2f81f7"/>
<text x="993.97" y="403" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.848</text>
<text x="402" y="440" fill="#c9d1d9" font-size="15" text-anchor="end">ShrdPtr · доступ и счетчик</text>
<rect x="1027.65" y="424" width="29.54" height="22" rx="5" fill="#2f81f7"/>
<text x="1019.65" y="441" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.916</text>
<text x="402" y="478" fill="#c9d1d9" font-size="15" text-anchor="end">WeakPtr · создание из shared и reset</text>
<rect x="1057.19" y="462" width="42.25" height="22" rx="5" fill="#f0883e"/>
<text x="1107.44" y="479" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="start">1.134</text>
<text x="402" y="516" fill="#c9d1d9" font-size="15" text-anchor="end">WeakPtr · копирование и reset</text>
<rect x="1057.19" y="500" width="190.62" height="22" rx="5" fill="#f0883e"/>
<text x="1255.82" y="517" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="start">1.764</text>
<text x="402" y="554" fill="#c9d1d9" font-size="15" text-anchor="end">WeakPtr · перемещение</text>
<rect x="1057.19" y="538" width="65.68" height="22" rx="5" fill="#f0883e"/>
<text x="1130.87" y="555" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="start">1.216</text>
<text x="402" y="592" fill="#c9d1d9" font-size="15" text-anchor="end">WeakPtr · lock живого объекта</text>
<rect x="495.71" y="576" width="561.48" height="22" rx="5" fill="#2f81f7"/>
<text x="487.71" y="593" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.188</text>
<text x="402" y="630" fill="#c9d1d9" font-size="15" text-anchor="end">WeakPtr · lock уничтоженного объекта</text>
<rect x="565.14" y="614" width="492.05" height="22" rx="5" fill="#2f81f7"/>
<text x="557.14" y="631" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.231</text>
<text x="402" y="668" fill="#c9d1d9" font-size="15" text-anchor="end">WeakPtr · expired и счетчик</text>
<rect x="1057.19" y="652" width="2.00" height="22" rx="5" fill="#f0883e"/>
<text x="1065.20" y="669" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="start">1.000</text>
<text x="402" y="706" fill="#c9d1d9" font-size="15" text-anchor="end">UnqPtr[] · создание и освобождение</text>
<rect x="1049.76" y="690" width="7.43" height="22" rx="5" fill="#2f81f7"/>
<text x="1041.76" y="707" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.978</text>
<text x="402" y="744" fill="#c9d1d9" font-size="15" text-anchor="end">UnqPtr[] · доступ по индексу</text>
<rect x="1015.40" y="728" width="41.79" height="22" rx="5" fill="#2f81f7"/>
<text x="1007.40" y="745" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.883</text>
<text x="402" y="782" fill="#c9d1d9" font-size="15" text-anchor="end">ShrdPtr[] · создание и освобождение</text>
<rect x="1040.01" y="766" width="17.19" height="22" rx="5" fill="#2f81f7"/>
<text x="1032.01" y="783" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.950</text>
<text x="402" y="820" fill="#c9d1d9" font-size="15" text-anchor="end">ShrdPtr[] · доступ по индексу</text>
<rect x="1057.19" y="804" width="10.70" height="22" rx="5" fill="#f0883e"/>
<text x="1075.89" y="821" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="start">1.032</text>
<text x="402" y="858" fill="#c9d1d9" font-size="15" text-anchor="end">WeakPtr[] · lock живого массива</text>
<rect x="514.99" y="842" width="542.20" height="22" rx="5" fill="#2f81f7"/>
<text x="506.99" y="859" fill="#f0f6fc" font-size="14" font-weight="600" text-anchor="end">0.199</text>
</svg>


![Uploading perfo<svg xmlns="http://www.w3.org/2000/svg" width="1440" height="520" viewBox="0 0 1440 520">
<rect width="100%" height="100%" fill="#0d1117"/>
<style>text{font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Arial,sans-serif}</style>
<text x="50" y="48" fill="#f0f6fc" font-size="28" font-weight="700">Масштабирование времени выполнения</text>
<text x="50" y="80" fill="#8b949e" font-size="17">Медиана семи release-замеров; шкала времени своя для каждого сценария</text>
<line x1="1040" y1="54" x2="1080" y2="54" stroke="#2f81f7" stroke-width="4"/><text x="1090" y="60" fill="#c9d1d9" font-size="15">Моя реализация</text>
<line x1="1240" y1="54" x2="1280" y2="54" stroke="#f0883e" stroke-width="4"/><text x="1290" y="60" fill="#c9d1d9" font-size="15">std</text>
<rect x="55.00" y="115" width="411.33" height="330" rx="12" fill="#161b22" stroke="#30363d"/>
<text x="260.67" y="145" fill="#f0f6fc" font-size="17" font-weight="600" text-anchor="middle">UnqPtr: создание и освобождение</text>
<line x1="107.00" y1="403.00" x2="448.33" y2="403.00" stroke="#30363d"/>
<text x="99.00" y="408.00" fill="#8b949e" font-size="12" text-anchor="end">0.00</text>
<line x1="107.00" y1="342.50" x2="448.33" y2="342.50" stroke="#30363d"/>
<text x="99.00" y="347.50" fill="#8b949e" font-size="12" text-anchor="end">0.68</text>
<line x1="107.00" y1="282.00" x2="448.33" y2="282.00" stroke="#30363d"/>
<text x="99.00" y="287.00" fill="#8b949e" font-size="12" text-anchor="end">1.36</text>
<line x1="107.00" y1="221.50" x2="448.33" y2="221.50" stroke="#30363d"/>
<text x="99.00" y="226.50" fill="#8b949e" font-size="12" text-anchor="end">2.04</text>
<line x1="107.00" y1="161.00" x2="448.33" y2="161.00" stroke="#30363d"/>
<text x="99.00" y="166.00" fill="#8b949e" font-size="12" text-anchor="end">2.71</text>
<polyline points="127.00,401.32 277.67,385.93 428.33,192.57" fill="none" stroke="#2f81f7" stroke-width="4"/>
<circle cx="127.00" cy="401.32" r="5" fill="#2f81f7"/>
<circle cx="277.67" cy="385.93" r="5" fill="#2f81f7"/>
<circle cx="428.33" cy="192.57" r="5" fill="#2f81f7"/>
<polyline points="127.00,401.35 277.67,386.94 428.33,203.14" fill="none" stroke="#f0883e" stroke-width="4"/>
<circle cx="127.00" cy="401.35" r="5" fill="#f0883e"/>
<circle cx="277.67" cy="386.94" r="5" fill="#f0883e"/>
<circle cx="428.33" cy="203.14" r="5" fill="#f0883e"/>
<text x="127.00" y="429" fill="#8b949e" font-size="13" text-anchor="middle">1,000</text>
<text x="277.67" y="429" fill="#8b949e" font-size="13" text-anchor="middle">10,000</text>
<text x="428.33" y="429" fill="#8b949e" font-size="13" text-anchor="middle">100,000</text>
<text x="69.00" y="173" fill="#8b949e" font-size="12">мс</text>
<rect x="514.33" y="115" width="411.33" height="330" rx="12" fill="#161b22" stroke="#30363d"/>
<text x="720.00" y="145" fill="#f0f6fc" font-size="17" font-weight="600" text-anchor="middle">ShrdPtr: создание и освобождение</text>
<line x1="566.33" y1="403.00" x2="907.67" y2="403.00" stroke="#30363d"/>
<text x="558.33" y="408.00" fill="#8b949e" font-size="12" text-anchor="end">0.00</text>
<line x1="566.33" y1="342.50" x2="907.67" y2="342.50" stroke="#30363d"/>
<text x="558.33" y="347.50" fill="#8b949e" font-size="12" text-anchor="end">1.53</text>
<line x1="566.33" y1="282.00" x2="907.67" y2="282.00" stroke="#30363d"/>
<text x="558.33" y="287.00" fill="#8b949e" font-size="12" text-anchor="end">3.07</text>
<line x1="566.33" y1="221.50" x2="907.67" y2="221.50" stroke="#30363d"/>
<text x="558.33" y="226.50" fill="#8b949e" font-size="12" text-anchor="end">4.60</text>
<line x1="566.33" y1="161.00" x2="907.67" y2="161.00" stroke="#30363d"/>
<text x="558.33" y="166.00" fill="#8b949e" font-size="12" text-anchor="end">6.14</text>
<polyline points="586.33,401.50 737.00,384.76 887.67,194.02" fill="none" stroke="#2f81f7" stroke-width="4"/>
<circle cx="586.33" cy="401.50" r="5" fill="#2f81f7"/>
<circle cx="737.00" cy="384.76" r="5" fill="#2f81f7"/>
<circle cx="887.67" cy="194.02" r="5" fill="#2f81f7"/>
<polyline points="586.33,401.42 737.00,383.24 887.67,192.57" fill="none" stroke="#f0883e" stroke-width="4"/>
<circle cx="586.33" cy="401.42" r="5" fill="#f0883e"/>
<circle cx="737.00" cy="383.24" r="5" fill="#f0883e"/>
<circle cx="887.67" cy="192.57" r="5" fill="#f0883e"/>
<text x="586.33" y="429" fill="#8b949e" font-size="13" text-anchor="middle">1,000</text>
<text x="737.00" y="429" fill="#8b949e" font-size="13" text-anchor="middle">10,000</text>
<text x="887.67" y="429" fill="#8b949e" font-size="13" text-anchor="middle">100,000</text>
<text x="528.33" y="173" fill="#8b949e" font-size="12">мс</text>
<rect x="973.67" y="115" width="411.33" height="330" rx="12" fill="#161b22" stroke="#30363d"/>
<text x="1179.33" y="145" fill="#f0f6fc" font-size="17" font-weight="600" text-anchor="middle">WeakPtr: lock живого объекта</text>
<line x1="1025.67" y1="403.00" x2="1367.00" y2="403.00" stroke="#30363d"/>
<text x="1017.67" y="408.00" fill="#8b949e" font-size="12" text-anchor="end">0.00</text>
<line x1="1025.67" y1="342.50" x2="1367.00" y2="342.50" stroke="#30363d"/>
<text x="1017.67" y="347.50" fill="#8b949e" font-size="12" text-anchor="end">0.38</text>
<line x1="1025.67" y1="282.00" x2="1367.00" y2="282.00" stroke="#30363d"/>
<text x="1017.67" y="287.00" fill="#8b949e" font-size="12" text-anchor="end">0.75</text>
<line x1="1025.67" y1="221.50" x2="1367.00" y2="221.50" stroke="#30363d"/>
<text x="1017.67" y="226.50" fill="#8b949e" font-size="12" text-anchor="end">1.13</text>
<line x1="1025.67" y1="161.00" x2="1367.00" y2="161.00" stroke="#30363d"/>
<text x="1017.67" y="166.00" fill="#8b949e" font-size="12" text-anchor="end">1.50</text>
<polyline points="1045.67,402.65 1196.33,399.47 1347.00,363.45" fill="none" stroke="#2f81f7" stroke-width="4"/>
<circle cx="1045.67" cy="402.65" r="5" fill="#2f81f7"/>
<circle cx="1196.33" cy="399.47" r="5" fill="#2f81f7"/>
<circle cx="1347.00" cy="363.45" r="5" fill="#2f81f7"/>
<polyline points="1045.67,401.15 1196.33,385.02 1347.00,192.57" fill="none" stroke="#f0883e" stroke-width="4"/>
<circle cx="1045.67" cy="401.15" r="5" fill="#f0883e"/>
<circle cx="1196.33" cy="385.02" r="5" fill="#f0883e"/>
<circle cx="1347.00" cy="192.57" r="5" fill="#f0883e"/>
<text x="1045.67" y="429" fill="#8b949e" font-size="13" text-anchor="middle">1,000</text>
<text x="1196.33" y="429" fill="#8b949e" font-size="13" text-anchor="middle">10,000</text>
<text x="1347.00" y="429" fill="#8b949e" font-size="13" text-anchor="middle">100,000</text>
<text x="987.67" y="173" fill="#8b949e" font-size="12">мс</text>
</svg>rmance_scaling.svg…]()

Исходный код измерений находится в [`Tests/PerformanceTests.hpp`](Tests/PerformanceTests.hpp). Моя реализация сравнивается только с указателем того же назначения:

| Моя реализация | Стандартная реализация |
|---|---|
| `UnqPtr` | `std::unique_ptr` |
| `ShrdPtr` | `std::shared_ptr` |
| `WeakPtr` | `std::weak_ptr` |

Проверяются три размера нагрузки: `1 000`, `10 000` и `100 000` элементов. Для каждого сценария сначала выполняется один прогревочный запуск, результат которого отбрасывается. Затем выполняются семь измерений, порядок запуска моей и стандартной реализации чередуется. Результаты сортируются, после чего выбирается медиана — четвёртое значение. Такой подход уменьшает влияние единичных скачков нагрузки операционной системы.

Измерения выполняются через `std::chrono::steady_clock`. Этот таймер монотонный: изменение системного времени не меняет длительность теста. Барьер компилятора `PerformanceObserve` не позволяет оптимизатору полностью удалить результат измеряемых операций.

Время подготовки контейнеров и исходных владельцев вынесено за границы замера, если оно не относится к проверяемому сценарию. Например, в тесте перемещения объекты создаются заранее, а таймер охватывает только move-конструктор и move-присваивание.

### Покрытие измерений

| Указатель | Измеряемые сценарии |
|---|---|
| `UnqPtr` | создание и освобождение; перемещение; `reset`; `get` и разыменование |
| `ShrdPtr` | создание и освобождение; перемещение; `reset`; копирование и сброс копий; доступ и чтение счётчика |
| `WeakPtr` | создание из сильного владельца; копирование; перемещение; `lock` живого объекта; `lock` уничтоженного объекта; `expired` и чтение счётчика |
| `UnqPtr[]` | создание и освобождение массива из 64 `int`; доступ по индексу |
| `ShrdPtr[]` | создание и освобождение массива из 64 `int`; доступ по индексу |
| `WeakPtr[]` | получение сильного владельца живого массива через `lock` |

Для каждого результата сохраняются:

- время всего сценария в миллисекундах;
- среднее время на один обрабатываемый элемент в наносекундах;
- отношение `Мой/std`.

Если `Мой/std < 1`, моя реализация выполнила конкретный сценарий быстрее. Если значение больше `1`, быстрее оказалась стандартная реализация. Это отношение относится только к данному сценарию и компьютеру, на котором проведён запуск.

## Результаты контрольного запуска

Контрольный запуск выполнен в release-режиме с `g++ 13.3.0`, `-O2`, C++20, Linux x86-64. На другом процессоре, компиляторе или версии стандартной библиотеки значения изменятся. Полный набор из 60 строк находится в [`Results/reference_results.csv`](Results/reference_results.csv) и [`Results/reference_results.md`](Results/reference_results.md).

В таблице ниже показана нагрузка `N = 100 000`. Время приведено в наносекундах на один элемент сценария.

| Указатель | Операция | Мой, нс | std, нс | Мой/std |
|---|---|---:|---:|---:|
| UnqPtr | создание и освобождение | 23.594 | 22.409 | 1.053 |
| UnqPtr | перемещение | 0.724 | 0.732 | 0.989 |
| UnqPtr | reset последнего владельца | 7.610 | 7.113 | 1.070 |
| UnqPtr | get и разыменование | 1.130 | 1.116 | 1.013 |
| ShrdPtr | создание и освобождение | 52.993 | 53.361 | 0.993 |
| ShrdPtr | перемещение | 1.314 | 1.178 | 1.115 |
| ShrdPtr | reset последнего владельца | 14.726 | 16.118 | 0.914 |
| ShrdPtr | копирование и reset копий | 3.401 | 4.009 | 0.848 |
| ShrdPtr | доступ и счётчик | 2.358 | 2.575 | 0.916 |
| WeakPtr | создание из shared и reset | 3.324 | 2.931 | 1.134 |
| WeakPtr | копирование и reset | 3.481 | 1.974 | 1.764 |
| WeakPtr | перемещение | 1.245 | 1.024 | 1.216 |
| WeakPtr | lock живого объекта | 2.457 | 13.076 | 0.188 |
| WeakPtr | lock уничтоженного объекта | 0.313 | 1.356 | 0.231 |
| WeakPtr | expired и счётчик | 0.470 | 0.470 | 1.000 |
| UnqPtr[] | создание и освобождение | 85.034 | 86.937 | 0.978 |
| UnqPtr[] | доступ по индексу | 3.849 | 4.359 | 0.883 |
| ShrdPtr[] | создание и освобождение | 65.948 | 69.410 | 0.950 |
| ShrdPtr[] | доступ по индексу | 4.167 | 4.036 | 1.032 |
| WeakPtr[] | lock живого массива | 2.622 | 13.174 | 0.199 |

![Отношение времени моей реализации к std](docs/performance_ratio.svg)

![Масштабирование времени выполнения](docs/performance_scaling.svg)

Большой выигрыш моей реализации в `WeakPtr::lock()` объясняется более простой однопоточной моделью счётчиков. Стандартный `std::shared_ptr` обеспечивает безопасное управление управляющим блоком между потоками и выполняет более сложные операции синхронизации. Поэтому этот результат нельзя трактовать как полное превосходство над стандартной библиотекой.

Микроизмерения зависят от частоты процессора, фоновой нагрузки, распределителя памяти, компилятора и оптимизатора. Небольшая разница около единицы может меняться от запуска к запуску. Надёжный вывод здесь состоит в сравнении порядка величин и масштабирования, а не отдельных тысячных наносекунды.

## Обновление графиков

После нового контрольного запуска нужно заменить `Results/reference_results.csv` полученным файлом и выполнить:

```bash
python3 Tools/generate_performance_charts.py
```

Скрипт использует только стандартную библиотеку Python и заново создаёт оба SVG-файла в каталоге `docs`.

## Ограничения реализации

- реализация предназначена для учебной однопоточной программы;
- счётчики `ShrdPtr` и `WeakPtr` не атомарные;
- разыменование пустого указателя, как и у стандартных умных указателей, недопустимо;
- размер массива указатель не хранит, поэтому правильность индекса контролирует вызывающий код;
- производный объект можно удалять через базовый тип только при виртуальном деструкторе базового класса.
