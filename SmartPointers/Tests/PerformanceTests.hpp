#ifndef PERFORMANCE_TESTS_HPP
#define PERFORMANCE_TESTS_HPP

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>

#include "UnqPtr.hpp"
#include "ShrdPtr.hpp"

// Защита от удаления измеряемых операций оптимизатором.
// Работает в Clang и GCC.
inline void PerformanceObserve(const void* Pointer) noexcept {
    __asm__ __volatile__("" : : "r"(Pointer) : "memory");
}

inline double PerformanceElapsed(std::chrono::steady_clock::time_point Start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
}

// Одинаковый способ создания для наших и стандартных указателей.
// IsArray определяет создание одиночного int или массива из 64 int.
template <typename UniquePointer, typename Pointer, bool IsArray = false>
Pointer PerformanceCreateOwner(int Value) {
    if constexpr (IsArray) {
        UniquePointer NewPointer(new int[64]{});
        return Pointer(std::move(NewPointer));
    } else {
        UniquePointer NewPointer(new int(Value));
        return Pointer(std::move(NewPointer));
    }
}

// Создание N объектов и освобождение всех владельцев.
// Хранилище пустых указателей создается до начала замера.
template <typename UniquePointer, typename Pointer, bool IsArray = false>
double PerformanceCreation(int Count) {
    std::unique_ptr<Pointer[]> Owners(new Pointer[Count]);

    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Owners[index] = PerformanceCreateOwner<UniquePointer, Pointer, IsArray>(index);
        PerformanceObserve(Owners[index].get());
    }

    for (int index = 0; index < Count; index++) {
        Owners[index].reset();
        PerformanceObserve(&Owners[index]);
    }

    return PerformanceElapsed(Start);
}

// Для каждого объекта выполняются move-конструктор и move-присваивание.
// Создание объектов и их последующее уничтожение находятся вне замера.
template <typename UniquePointer, typename Pointer>
double PerformanceMove(int Count) {
    std::unique_ptr<Pointer[]> First(new Pointer[Count]);
    std::unique_ptr<Pointer[]> Second(new Pointer[Count]);

    for (int index = 0; index < Count; index++) {
        First[index] = PerformanceCreateOwner<UniquePointer, Pointer>(index);
    }

    PerformanceObserve(First.get());
    PerformanceObserve(Second.get());

    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Pointer TempPointer(std::move(First[index]));
        PerformanceObserve(&TempPointer);

        Second[index] = std::move(TempPointer);

        PerformanceObserve(&Second[index]);
        PerformanceObserve(&First[index]);
        PerformanceObserve(&TempPointer);
    }

    return PerformanceElapsed(Start);
}

// Освобождение N объектов через reset() единственного владельца.
// Первоначальное создание объектов не входит в замер.
template <typename UniquePointer, typename Pointer>
double PerformanceReset(int Count) {
    std::unique_ptr<Pointer[]> Owners(new Pointer[Count]);

    for (int index = 0; index < Count; index++) {
        Owners[index] = PerformanceCreateOwner<UniquePointer, Pointer>(index);
    }

    PerformanceObserve(Owners.get());

    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Owners[index].reset();
        PerformanceObserve(&Owners[index]);
    }

    return PerformanceElapsed(Start);
}

// Создание N дополнительных владельцев одного объекта и освобождение копий.
// Исходный владелец остается живым до конца функции.
template <typename UniquePointer, typename SharedPointer>
double PerformanceCopy(int Count) {
    SharedPointer Source = PerformanceCreateOwner<UniquePointer, SharedPointer>(10);
    std::unique_ptr<SharedPointer[]> Copies(new SharedPointer[Count]);

    PerformanceObserve(&Source);
    PerformanceObserve(Copies.get());

    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        SharedPointer TempPointer(Source);
        PerformanceObserve(&TempPointer);

        Copies[index] = std::move(TempPointer);

        PerformanceObserve(&Copies[index]);
        PerformanceObserve(&TempPointer);
    }

    for (int index = 0; index < Count; index++) {
        Copies[index].reset();
        PerformanceObserve(&Copies[index]);
    }

    PerformanceObserve(&Source);

    return PerformanceElapsed(Start);
}

// Ручная сортировка пяти результатов для получения медианы.
inline double PerformanceMedian(double Values[5]) {
    for (int first = 0; first < 4; first++) {
        for (int second = first + 1; second < 5; second++) {
            if (Values[second] < Values[first]) {
                double TempValue = Values[first];
                Values[first] = Values[second];
                Values[second] = TempValue;
            }
        }
    }

    return Values[2];
}

// Прогрев, пять измерений, вывод медиан и отношения времени.
// Функции получают одинаковое количество объектов.
inline void ComparePerformance(const char* Name, int Count, double (*OwnFunction)(int), double (*StandardFunction)(int)) {
    double OwnTimes[5]{};
    double StandardTimes[5]{};

    OwnFunction(Count);
    StandardFunction(Count);

    for (int Repeat = 0; Repeat < 5; Repeat++) {
        if (Repeat % 2 == 0) {
            OwnTimes[Repeat] = OwnFunction(Count);
            StandardTimes[Repeat] = StandardFunction(Count);
        } else {
            StandardTimes[Repeat] = StandardFunction(Count);
            OwnTimes[Repeat] = OwnFunction(Count);
        }
    }

    double OwnMedian = PerformanceMedian(OwnTimes);
    double StandardMedian = PerformanceMedian(StandardTimes);

    std::cout << Name << "; " << Count << "; "
              << OwnMedian << "; " << StandardMedian << "; ";

    if (StandardMedian > 0) {
        std::cout << OwnMedian / StandardMedian;
    } else {
        std::cout << "недостаточная точность таймера";
    }

    std::cout << "\n";
}

// 0 — измерения завершены или отключены для текущей сборки.
// 1 — измерения прерваны исключением.
// Более медленная работа не считается ошибкой корректности.
inline int run_performance_tests() {
#ifndef PERFORMANCE_ENABLED
    std::cout << "\nДля измерения производительности нужна release-сборка.\n";
    std::cout << "Выполни make release, затем ./smart_pointers.\n";
    return 0;
#else
    const int Sizes[3] = {1000, 10000, 100000};

    std::cout << "\n--- Сравнение производительности ---\n";
    std::cout << "Прогрев и пять измерений. Выводится медиана.\n";
    std::cout << "Время полного сценария указано в миллисекундах.\n";
    std::cout << "Наш/std меньше 1: наша реализация быстрее в этом сценарии.\n";
    std::cout << "В массивных сценариях N — число массивов по 64 int.\n\n";
    std::cout << "Сценарий; N; Наш, мс; std, мс; Наш/std\n";

    try {
        for (int index = 0; index < 3; index++) {
            int Count = Sizes[index];

            // 1. Создание и освобождение одиночных объектов через Unique.
            ComparePerformance("Unique: создание и освобождение", Count, PerformanceCreation<UnqPtr<int>, UnqPtr<int>>, PerformanceCreation<std::unique_ptr<int>, std::unique_ptr<int>>);

            // 2. Создание и освобождение одиночных объектов через Shared.
            ComparePerformance("Shared: создание и освобождение", Count, PerformanceCreation<UnqPtr<int>, ShrdPtr<int>>, PerformanceCreation<std::unique_ptr<int>, std::shared_ptr<int>>);

            // 3. Move-конструктор и move-присваивание Unique.
            ComparePerformance("Unique: перемещение", Count, PerformanceMove<UnqPtr<int>, UnqPtr<int>>, PerformanceMove<std::unique_ptr<int>, std::unique_ptr<int>>);

            // 4. Move-конструктор и move-присваивание Shared.
            ComparePerformance("Shared: перемещение", Count, PerformanceMove<UnqPtr<int>, ShrdPtr<int>>, PerformanceMove<std::unique_ptr<int>, std::shared_ptr<int>>);

            // 5. reset() уникального владельца.
            ComparePerformance("Unique: reset", Count, PerformanceReset<UnqPtr<int>, UnqPtr<int>>, PerformanceReset<std::unique_ptr<int>, std::unique_ptr<int>>);

            // 6. reset() единственного совместного владельца.
            ComparePerformance("Shared: reset", Count, PerformanceReset<UnqPtr<int>, ShrdPtr<int>>, PerformanceReset<std::unique_ptr<int>, std::shared_ptr<int>>);

            // 7. Копирование совместного владения и освобождение копий.
            ComparePerformance("Shared: копирование и освобождение копий", Count, PerformanceCopy<UnqPtr<int>, ShrdPtr<int>>, PerformanceCopy<std::unique_ptr<int>, std::shared_ptr<int>>);

            // 8. Создание и освобождение массивов через Unique.
            ComparePerformance("Unique[]: создание и освобождение", Count, PerformanceCreation<UnqPtr<int[]>, UnqPtr<int[]>, true>, PerformanceCreation<std::unique_ptr<int[]>, std::unique_ptr<int[]>, true>);

            // 9. Создание и освобождение массивов через Shared.
            ComparePerformance("Shared[]: создание и освобождение", Count, PerformanceCreation<UnqPtr<int[]>, ShrdPtr<int[]>, true>, PerformanceCreation<std::unique_ptr<int[]>, std::shared_ptr<int[]>, true>);
        }
    } catch (...) {
        std::cout << "\nИзмерения прерваны исключением.\n";
        return 1;
    }

    std::cout << "\nИзмерения завершены.\n";
    return 0;
#endif
}

#endif // PERFORMANCE_TESTS_HPP