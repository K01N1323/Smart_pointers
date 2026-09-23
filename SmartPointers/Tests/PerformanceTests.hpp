#ifndef PERFORMANCE_TESTS_HPP
#define PERFORMANCE_TESTS_HPP

#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <utility>

#include "UnqPtr.hpp"
#include "ShrdPtr.hpp"
#include "WeakPtr.hpp"

inline void PerformanceObserve(const void* Pointer) noexcept {
    __asm__ __volatile__("" : : "r"(Pointer) : "memory");
}

inline void PerformanceObserveValue(std::uint64_t Value) noexcept {
    __asm__ __volatile__("" : : "r"(Value) : "memory");
}

inline double PerformanceElapsed(std::chrono::steady_clock::time_point Start) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - Start).count();
}

template <typename UniquePointer, typename Pointer, bool IsArray = false>
Pointer PerformanceCreateOwner(int Value) {
    if constexpr (IsArray) {
        UniquePointer NewPointer(new int[64]{});
        NewPointer[Value % 64] = Value;
        return Pointer(std::move(NewPointer));
    } else {
        UniquePointer NewPointer(new int(Value));
        return Pointer(std::move(NewPointer));
    }
}

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

template <typename UniquePointer, typename Pointer>
double PerformanceMove(int Count) {
    std::unique_ptr<Pointer[]> First(new Pointer[Count]);
    std::unique_ptr<Pointer[]> Second(new Pointer[Count]);

    for (int index = 0; index < Count; index++) {
        First[index] = PerformanceCreateOwner<UniquePointer, Pointer>(index);
    }

    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Pointer TempPointer(std::move(First[index]));
        Second[index] = std::move(TempPointer);
        PerformanceObserve(Second[index].get());
    }

    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename Pointer>
double PerformanceReset(int Count) {
    std::unique_ptr<Pointer[]> Owners(new Pointer[Count]);

    for (int index = 0; index < Count; index++) {
        Owners[index] = PerformanceCreateOwner<UniquePointer, Pointer>(index);
    }

    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Owners[index].reset();
        PerformanceObserve(&Owners[index]);
    }

    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename Pointer>
double PerformanceAccess(int Count) {
    std::unique_ptr<Pointer[]> Owners(new Pointer[Count]);

    for (int index = 0; index < Count; index++) {
        Owners[index] = PerformanceCreateOwner<UniquePointer, Pointer>(index);
    }

    std::uint64_t Sum = 0;
    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Sum += static_cast<std::uint64_t>(*Owners[index]);
        PerformanceObserve(Owners[index].get());
    }

    PerformanceObserveValue(Sum);
    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename Pointer>
double PerformanceArrayAccess(int Count) {
    std::unique_ptr<Pointer[]> Owners(new Pointer[Count]);

    for (int index = 0; index < Count; index++) {
        Owners[index] = PerformanceCreateOwner<UniquePointer, Pointer, true>(index);
    }

    std::uint64_t Sum = 0;
    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Sum += static_cast<std::uint64_t>(Owners[index][index % 64]);
        PerformanceObserve(Owners[index].get());
    }

    PerformanceObserveValue(Sum);
    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename SharedPointer>
double PerformanceCopy(int Count) {
    SharedPointer Source = PerformanceCreateOwner<UniquePointer, SharedPointer>(10);
    std::unique_ptr<SharedPointer[]> Copies(new SharedPointer[Count]);
    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Copies[index] = Source;
        PerformanceObserve(Copies[index].get());
    }

    for (int index = 0; index < Count; index++) {
        Copies[index].reset();
        PerformanceObserve(&Copies[index]);
    }

    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename SharedPointer, bool IsStandard = false>
double PerformanceSharedInformation(int Count) {
    std::unique_ptr<SharedPointer[]> Owners(new SharedPointer[Count]);

    for (int index = 0; index < Count; index++) {
        Owners[index] = PerformanceCreateOwner<UniquePointer, SharedPointer>(index);
    }

    std::uint64_t Sum = 0;
    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Sum += static_cast<std::uint64_t>(*Owners[index]);

        if constexpr (IsStandard) {
            Sum += Owners[index].use_count();
        } else {
            Sum += Owners[index].UseCount();
        }

        PerformanceObserve(Owners[index].get());
    }

    PerformanceObserveValue(Sum);
    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename SharedPointer, typename WeakPointer>
double PerformanceWeakCreation(int Count) {
    SharedPointer Source = PerformanceCreateOwner<UniquePointer, SharedPointer>(10);
    std::unique_ptr<WeakPointer[]> Observers(new WeakPointer[Count]);
    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Observers[index] = Source;
        PerformanceObserve(&Observers[index]);
    }

    for (int index = 0; index < Count; index++) {
        Observers[index].reset();
        PerformanceObserve(&Observers[index]);
    }

    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename SharedPointer, typename WeakPointer>
double PerformanceWeakCopy(int Count) {
    SharedPointer Owner = PerformanceCreateOwner<UniquePointer, SharedPointer>(10);
    WeakPointer Source(Owner);
    std::unique_ptr<WeakPointer[]> Copies(new WeakPointer[Count]);
    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Copies[index] = Source;
        PerformanceObserve(&Copies[index]);
    }

    for (int index = 0; index < Count; index++) {
        Copies[index].reset();
        PerformanceObserve(&Copies[index]);
    }

    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename SharedPointer, typename WeakPointer>
double PerformanceWeakMove(int Count) {
    SharedPointer Owner = PerformanceCreateOwner<UniquePointer, SharedPointer>(10);
    std::unique_ptr<WeakPointer[]> First(new WeakPointer[Count]);
    std::unique_ptr<WeakPointer[]> Second(new WeakPointer[Count]);

    for (int index = 0; index < Count; index++) {
        First[index] = Owner;
    }

    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        WeakPointer TempPointer(std::move(First[index]));
        Second[index] = std::move(TempPointer);
        PerformanceObserve(&Second[index]);
    }

    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename SharedPointer, typename WeakPointer, bool IsArray = false>
double PerformanceWeakLock(int Count) {
    SharedPointer Owner = PerformanceCreateOwner<UniquePointer, SharedPointer, IsArray>(10);
    WeakPointer Observer(Owner);
    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        SharedPointer Locked = Observer.lock();
        PerformanceObserve(Locked.get());
    }

    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename SharedPointer, typename WeakPointer>
double PerformanceWeakExpiredLock(int Count) {
    WeakPointer Observer;

    {
        SharedPointer Owner = PerformanceCreateOwner<UniquePointer, SharedPointer>(10);
        Observer = Owner;
    }

    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        SharedPointer Locked = Observer.lock();
        PerformanceObserve(Locked.get());
    }

    return PerformanceElapsed(Start);
}

template <typename UniquePointer, typename SharedPointer, typename WeakPointer, bool IsStandard = false>
double PerformanceWeakInformation(int Count) {
    SharedPointer Owner = PerformanceCreateOwner<UniquePointer, SharedPointer>(10);
    WeakPointer Observer(Owner);
    std::uint64_t Sum = 0;
    auto Start = std::chrono::steady_clock::now();

    for (int index = 0; index < Count; index++) {
        Sum += Observer.expired() ? 1u : 0u;

        if constexpr (IsStandard) {
            Sum += Observer.use_count();
        } else {
            Sum += Observer.UseCount();
        }

        PerformanceObserve(&Observer);
    }

    PerformanceObserveValue(Sum);
    return PerformanceElapsed(Start);
}

inline double PerformanceMedian(double Values[7]) {
    for (int first = 0; first < 6; first++) {
        for (int second = first + 1; second < 7; second++) {
            if (Values[second] < Values[first]) {
                double TempValue = Values[first];
                Values[first] = Values[second];
                Values[second] = TempValue;
            }
        }
    }

    return Values[3];
}

struct PerformanceResult {
    const char* PointerType = nullptr;
    const char* Operation = nullptr;
    int Count = 0;
    double OwnMilliseconds = 0;
    double StandardMilliseconds = 0;
    double OwnNanoseconds = 0;
    double StandardNanoseconds = 0;
    double Ratio = 0;
};

inline PerformanceResult ComparePerformance(const char* PointerType, const char* Operation, int Count, double (*OwnFunction)(int), double (*StandardFunction)(int)) {
    double OwnTimes[7]{};
    double StandardTimes[7]{};

    OwnFunction(Count);
    StandardFunction(Count);

    for (int Repeat = 0; Repeat < 7; Repeat++) {
        if (Repeat % 2 == 0) {
            OwnTimes[Repeat] = OwnFunction(Count);
            StandardTimes[Repeat] = StandardFunction(Count);
        } else {
            StandardTimes[Repeat] = StandardFunction(Count);
            OwnTimes[Repeat] = OwnFunction(Count);
        }
    }

    PerformanceResult Result;
    Result.PointerType = PointerType;
    Result.Operation = Operation;
    Result.Count = Count;
    Result.OwnMilliseconds = PerformanceMedian(OwnTimes);
    Result.StandardMilliseconds = PerformanceMedian(StandardTimes);
    Result.OwnNanoseconds = Result.OwnMilliseconds * 1000000.0 / Count;
    Result.StandardNanoseconds = Result.StandardMilliseconds * 1000000.0 / Count;

    if (Result.StandardMilliseconds > 0) {
        Result.Ratio = Result.OwnMilliseconds / Result.StandardMilliseconds;
    }

    std::cout << PointerType << "; " << Operation << "; " << Count << "; "
              << Result.OwnMilliseconds << "; " << Result.StandardMilliseconds << "; "
              << Result.OwnNanoseconds << "; " << Result.StandardNanoseconds << "; "
              << Result.Ratio << "\n";

    return Result;
}

inline bool SavePerformanceCsv(const PerformanceResult* Results, int ResultCount, const char* FileName) {
    std::ofstream File(FileName);
    if (!File) {return false;}

    File << "Pointer;Operation;N;MyMilliseconds;StdMilliseconds;MyNanosecondsPerElement;StdNanosecondsPerElement;MyToStd\n";
    File << std::fixed << std::setprecision(6);

    for (int index = 0; index < ResultCount; index++) {
        const PerformanceResult& Result = Results[index];
        File << Result.PointerType << ";" << Result.Operation << ";" << Result.Count << ";"
             << Result.OwnMilliseconds << ";" << Result.StandardMilliseconds << ";"
             << Result.OwnNanoseconds << ";" << Result.StandardNanoseconds << ";"
             << Result.Ratio << "\n";
    }

    return static_cast<bool>(File);
}

inline bool SavePerformanceMarkdown(const PerformanceResult* Results, int ResultCount, const char* FileName) {
    std::ofstream File(FileName);
    if (!File) {return false;}

    File << "# Результаты тестов производительности\n\n";
    File << "| Указатель | Операция | N | Мой, мс | std, мс | Мой, нс/элемент | std, нс/элемент | Мой/std |\n";
    File << "|---|---|---:|---:|---:|---:|---:|---:|\n";
    File << std::fixed << std::setprecision(6);

    for (int index = 0; index < ResultCount; index++) {
        const PerformanceResult& Result = Results[index];
        File << "| " << Result.PointerType << " | " << Result.Operation << " | " << Result.Count << " | "
             << Result.OwnMilliseconds << " | " << Result.StandardMilliseconds << " | "
             << Result.OwnNanoseconds << " | " << Result.StandardNanoseconds << " | "
             << Result.Ratio << " |\n";
    }

    return static_cast<bool>(File);
}

inline int run_performance_tests() {
#ifndef PERFORMANCE_ENABLED
    std::cout << "\nДля измерения производительности нужна release-сборка.\n";
    std::cout << "Запусти ./smart_pointers_release и выбери пункт 6.\n";
    return 0;
#else
    const int Sizes[3] = {1000, 10000, 100000};
    PerformanceResult Results[60];
    int ResultCount = 0;

    std::cout << "\nСравнение производительности\n";
    std::cout << "Один прогрев, затем семь замеров. Показываю медиану.\n";
    std::cout << "Время указано для полного сценария и для одного элемента.\n";
    std::cout << "Мой/std меньше 1: моя реализация быстрее в этом сценарии.\n";
    std::cout << "Массив содержит 64 элемента int.\n\n";
    std::cout << "Указатель; Операция; N; Мой, мс; std, мс; Мой, нс/элемент; std, нс/элемент; Мой/std\n";

    try {
        for (int index = 0; index < 3; index++) {
            int Count = Sizes[index];

            Results[ResultCount++] = ComparePerformance("UnqPtr", "создание и освобождение", Count, PerformanceCreation<UnqPtr<int>, UnqPtr<int>>, PerformanceCreation<std::unique_ptr<int>, std::unique_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("UnqPtr", "перемещение", Count, PerformanceMove<UnqPtr<int>, UnqPtr<int>>, PerformanceMove<std::unique_ptr<int>, std::unique_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("UnqPtr", "reset последнего владельца", Count, PerformanceReset<UnqPtr<int>, UnqPtr<int>>, PerformanceReset<std::unique_ptr<int>, std::unique_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("UnqPtr", "get и разыменование", Count, PerformanceAccess<UnqPtr<int>, UnqPtr<int>>, PerformanceAccess<std::unique_ptr<int>, std::unique_ptr<int>>);

            Results[ResultCount++] = ComparePerformance("ShrdPtr", "создание и освобождение", Count, PerformanceCreation<UnqPtr<int>, ShrdPtr<int>>, PerformanceCreation<std::unique_ptr<int>, std::shared_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("ShrdPtr", "перемещение", Count, PerformanceMove<UnqPtr<int>, ShrdPtr<int>>, PerformanceMove<std::unique_ptr<int>, std::shared_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("ShrdPtr", "reset последнего владельца", Count, PerformanceReset<UnqPtr<int>, ShrdPtr<int>>, PerformanceReset<std::unique_ptr<int>, std::shared_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("ShrdPtr", "копирование и reset копий", Count, PerformanceCopy<UnqPtr<int>, ShrdPtr<int>>, PerformanceCopy<std::unique_ptr<int>, std::shared_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("ShrdPtr", "доступ и счетчик", Count, PerformanceSharedInformation<UnqPtr<int>, ShrdPtr<int>>, PerformanceSharedInformation<std::unique_ptr<int>, std::shared_ptr<int>, true>);

            Results[ResultCount++] = ComparePerformance("WeakPtr", "создание из shared и reset", Count, PerformanceWeakCreation<UnqPtr<int>, ShrdPtr<int>, WeakPtr<int>>, PerformanceWeakCreation<std::unique_ptr<int>, std::shared_ptr<int>, std::weak_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("WeakPtr", "копирование и reset", Count, PerformanceWeakCopy<UnqPtr<int>, ShrdPtr<int>, WeakPtr<int>>, PerformanceWeakCopy<std::unique_ptr<int>, std::shared_ptr<int>, std::weak_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("WeakPtr", "перемещение", Count, PerformanceWeakMove<UnqPtr<int>, ShrdPtr<int>, WeakPtr<int>>, PerformanceWeakMove<std::unique_ptr<int>, std::shared_ptr<int>, std::weak_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("WeakPtr", "lock живого объекта", Count, PerformanceWeakLock<UnqPtr<int>, ShrdPtr<int>, WeakPtr<int>>, PerformanceWeakLock<std::unique_ptr<int>, std::shared_ptr<int>, std::weak_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("WeakPtr", "lock уничтоженного объекта", Count, PerformanceWeakExpiredLock<UnqPtr<int>, ShrdPtr<int>, WeakPtr<int>>, PerformanceWeakExpiredLock<std::unique_ptr<int>, std::shared_ptr<int>, std::weak_ptr<int>>);
            Results[ResultCount++] = ComparePerformance("WeakPtr", "expired и счетчик", Count, PerformanceWeakInformation<UnqPtr<int>, ShrdPtr<int>, WeakPtr<int>>, PerformanceWeakInformation<std::unique_ptr<int>, std::shared_ptr<int>, std::weak_ptr<int>, true>);

            Results[ResultCount++] = ComparePerformance("UnqPtr[]", "создание и освобождение", Count, PerformanceCreation<UnqPtr<int[]>, UnqPtr<int[]>, true>, PerformanceCreation<std::unique_ptr<int[]>, std::unique_ptr<int[]>, true>);
            Results[ResultCount++] = ComparePerformance("UnqPtr[]", "доступ по индексу", Count, PerformanceArrayAccess<UnqPtr<int[]>, UnqPtr<int[]>>, PerformanceArrayAccess<std::unique_ptr<int[]>, std::unique_ptr<int[]>>);
            Results[ResultCount++] = ComparePerformance("ShrdPtr[]", "создание и освобождение", Count, PerformanceCreation<UnqPtr<int[]>, ShrdPtr<int[]>, true>, PerformanceCreation<std::unique_ptr<int[]>, std::shared_ptr<int[]>, true>);
            Results[ResultCount++] = ComparePerformance("ShrdPtr[]", "доступ по индексу", Count, PerformanceArrayAccess<UnqPtr<int[]>, ShrdPtr<int[]>>, PerformanceArrayAccess<std::unique_ptr<int[]>, std::shared_ptr<int[]>>);
            Results[ResultCount++] = ComparePerformance("WeakPtr[]", "lock живого массива", Count, PerformanceWeakLock<UnqPtr<int[]>, ShrdPtr<int[]>, WeakPtr<int[]>, true>, PerformanceWeakLock<std::unique_ptr<int[]>, std::shared_ptr<int[]>, std::weak_ptr<int[]>, true>);
        }
    } catch (...) {
        std::cout << "\nИзмерения прерваны исключением.\n";
        return 1;
    }

    bool CsvSaved = SavePerformanceCsv(Results, ResultCount, "performance_results.csv");
    bool MarkdownSaved = SavePerformanceMarkdown(Results, ResultCount, "performance_results.md");

    std::cout << "\nИзмерения завершены.\n";
    std::cout << (CsvSaved ? "CSV сохранен в performance_results.csv.\n" : "Не удалось сохранить performance_results.csv.\n");
    std::cout << (MarkdownSaved ? "Таблица сохранена в performance_results.md.\n" : "Не удалось сохранить performance_results.md.\n");

    return CsvSaved && MarkdownSaved ? 0 : 1;
#endif
}

#endif // PERFORMANCE_TESTS_HPP
