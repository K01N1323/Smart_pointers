#ifndef PERFORMANCE_TESTS_HPP
#define PERFORMANCE_TESTS_HPP

#include <chrono>
#include <iostream>
#include <memory>
#include <utility>

#include "UnqPtr.hpp"
#include "ShrdPtr.hpp"

// Не даю оптимизатору убрать измеряемые операции (Clang и GCC)
inline void PerformanceObserve(const void* Pointer) noexcept {
    __asm__ __volatile__("" : : "r"(Pointer) : "memory");
}

inline double PerformanceElapsed(std::chrono::steady_clock::time_point Start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
}

// Создаю объект одинаковым способом для обеих реализаций
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

// Измеряю создание и освобождение N объектов; хранилище готовлю заранее
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

// Измеряю move-конструктор и move-присваивание; объекты создаю до замера.
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

// Измеряю reset последнего владельца; объекты создаю до замера.
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

// Создаю N копий присваиванием и сбрасываю их; исходного владельца сохраняю.
template <typename UniquePointer, typename SharedPointer>
double PerformanceCopy(int Count) {
    SharedPointer Source = PerformanceCreateOwner<UniquePointer, SharedPointer>(10);
    std::unique_ptr<SharedPointer[]> Copies(new SharedPointer[Count]);

    PerformanceObserve(&Source);
    PerformanceObserve(Copies.get());

    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Copies[index] = Source;
        PerformanceObserve(&Copies[index]);
    }

    for (int index = 0; index < Count; index++) {
        Copies[index].reset();
        PerformanceObserve(&Copies[index]);
    }

    PerformanceObserve(&Source);

    return PerformanceElapsed(Start);
}

// Сортирую пять замеров и беру средний по порядку.
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

// Чередую порядок замеров моей и стандартной реализации.
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

// Возвращаю 1 при исключении, иначе 0.
inline int run_performance_tests() {
#ifndef PERFORMANCE_ENABLED
    std::cout << "\nДля измерения производительности нужна release-сборка.\n";
    std::cout << "Запусти ./smart_pointers_release и выбери пункт 6.\n";
    return 0;
#else
    const int Sizes[3] = {1000, 10000, 100000};

    std::cout << "\nСравнение производительности\n";
    std::cout << "Один прогрев, затем пять замеров. Показываю медиану.\n";
    std::cout << "Время сценария в миллисекундах; это не время одной операции.\n";
    std::cout << "Мой/std меньше 1: моя реализация быстрее в этом сценарии.\n";
    std::cout << "В массивных сценариях N — число массивов по 64 int.\n\n";
    std::cout << "Сценарий; N; Мой, мс; std, мс; Мой/std\n";

    try {
        for (int index = 0; index < 3; index++) {
            int Count = Sizes[index];

            // 1. Сравниваю создание и освобождение UnqPtr.
            ComparePerformance("Unique: создание и освобождение", Count, PerformanceCreation<UnqPtr<int>, UnqPtr<int>>, PerformanceCreation<std::unique_ptr<int>, std::unique_ptr<int>>);

            // 2. Сравниваю создание и освобождение ShrdPtr.
            ComparePerformance("Shared: создание и освобождение", Count, PerformanceCreation<UnqPtr<int>, ShrdPtr<int>>, PerformanceCreation<std::unique_ptr<int>, std::shared_ptr<int>>);

            // 3. Сравниваю перемещение UnqPtr.
            ComparePerformance("Unique: перемещение", Count, PerformanceMove<UnqPtr<int>, UnqPtr<int>>, PerformanceMove<std::unique_ptr<int>, std::unique_ptr<int>>);

            // 4. Сравниваю перемещение ShrdPtr.
            ComparePerformance("Shared: перемещение", Count, PerformanceMove<UnqPtr<int>, ShrdPtr<int>>, PerformanceMove<std::unique_ptr<int>, std::shared_ptr<int>>);

            // 5. Сравниваю reset у UnqPtr.
            ComparePerformance("Unique: reset", Count, PerformanceReset<UnqPtr<int>, UnqPtr<int>>, PerformanceReset<std::unique_ptr<int>, std::unique_ptr<int>>);

            // 6. Сравниваю reset у последнего ShrdPtr.
            ComparePerformance("Shared: reset", Count, PerformanceReset<UnqPtr<int>, ShrdPtr<int>>, PerformanceReset<std::unique_ptr<int>, std::shared_ptr<int>>);

            // 7. Сравниваю копирующее присваивание ShrdPtr и reset копий.
            ComparePerformance("Shared: копирующее присваивание и reset", Count, PerformanceCopy<UnqPtr<int>, ShrdPtr<int>>, PerformanceCopy<std::unique_ptr<int>, std::shared_ptr<int>>);

            // 8. Сравниваю создание и освобождение массивов UnqPtr.
            ComparePerformance("Unique[]: создание и освобождение", Count, PerformanceCreation<UnqPtr<int[]>, UnqPtr<int[]>, true>, PerformanceCreation<std::unique_ptr<int[]>, std::unique_ptr<int[]>, true>);

            // 9. Сравниваю создание и освобождение массивов ShrdPtr.
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
