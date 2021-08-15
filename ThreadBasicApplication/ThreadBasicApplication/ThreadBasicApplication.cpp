#include <iostream>
#include <thread>
#include <omp.h>
#include <mutex>

using namespace std;

int* origArray;

void GenerateArray(int* array, int count)
{
    for (int i = 0; i < count; i++)
    {
        array[i] = rand() % 80000 - 200;
    }
}

void CopyOrigArray(int* bufferArray, int count) 
{
    for (int i = 0; i < count; i++)
    {
        bufferArray[i] = origArray[i];
    }
}

void CompareArays(int* array1, int* array2, int count) 
{
    bool errorFound = false;
    for (int i = 0; i < count; i++)
    {
        if (array1[i] != array2[i])
        {
            cout << "Элемент массива " << i << " не верно отсортирован. Должно быть " << array1[i] << " а стоит " << array2[i] << endl;
        }
    }
    if (errorFound) 
    {
        cout << "Сортировка произведена неверно" << endl;
    }
}

void OneThreadCountingSort(int* array, int count)
{
    int max = INT_MIN, min = INT_MAX;
    for (int i = 0; i < count; i++) {
        if (array[i] > max)
            max = array[i];
        if (array[i] < min)
            min = array[i];
    }
    int* sortArray = new int[max + 1 - min];
    for (int i = 0; i < max + 1 - min; i++) {
        sortArray[i] = 0;
    }
    for (int i = 0; i < count; i++) {
        sortArray[array[i] - min] = sortArray[array[i] - min] + 1;
    }
    int i = 0;
    for (int j = min; j < max + 1; j++) {
        while (sortArray[j - min] != 0) {
            array[i] = j;
            sortArray[j - min]--;
            i++;
        }
    }
    delete[] sortArray;
}

double OneThreadTest(int* array, int maxCount)
{
    double start;
    double end;

    CopyOrigArray(array, maxCount);

    start = omp_get_wtime();
    
    thread th(OneThreadCountingSort, array, maxCount);
    th.join();
        
    end = omp_get_wtime();

    double res = end - start;

    return res;
}

void FindMaxMin(int* array, int start, int end, int* max, int* min)
{
    *max = INT_MIN, *min = INT_MAX;
    for (int i = start; i < end; i++) {
        if (array[i] > *max)
            *max = array[i];
        if (array[i] < *min)
            *min = array[i];
    }
}

void CountNumbers(int* array, int* sortArray, int start, int end, int min) 
{
    for (int i = start; i < end; i++) {
        sortArray[array[i] - min] = sortArray[array[i] - min] + 1;
    }
}

void FillSortedArray(int* array, int* sortArray, int start, int end, int min) 
{
    int count = 0;
    int k = 0;
    while (count < start)
    {
        count += sortArray[k++];
    }
    for (int i = start; i < count; i++)
    {
        array[i] = k - 1 + min;
    }

    for (; count < end; k++)
    {
        for (int i = 0; i < sortArray[k] && count < end; ++i)
        {
            array[count++] = k + min;
        }
    }
}

double MultiThreadTest(int* array, int maxCount, int threads) {

    double startTime;
    double endTime;

    CopyOrigArray(array, maxCount);

    startTime = omp_get_wtime();
    //Операция 1. Ищем минимальное и максимальное значение в массиве.
    int* maxArr = new int[threads];
    int* minArr = new int[threads];
    thread* th = new thread[threads];
    
    for (int i = 0; i < threads; i++)
    {
        int startInd = (maxCount / threads) * i;
        int buf = (maxCount / threads) * (i + 1);
        int endInd = buf > maxCount ? maxCount : buf;
        th[i] = thread(FindMaxMin, array, startInd, endInd, &maxArr[i], &minArr[i]);
    }

    for (int i = 0; i < threads; i++)
    {
        th[i].join();
    }
    int max = maxArr[0];
    int min = minArr[0];

    for (int i = 1; i < threads; i++)
    {
        if (maxArr[i] < max) max = maxArr[i];
        if (minArr[i] < min) min = minArr[i];
    }
    // Операция 2. Создаём массив для сортировки.
    int* sortArray = new int[max + 1 - min];
    for (int i = 0; i < max + 1 - min; i++) {
        sortArray[i] = 0;
    }
    int** reducedSortArray = new int*[threads];
    for (int i = 0; i < threads; i++)
    {
        reducedSortArray[i] = new int[max + 1 - min];
    }
    for (int i = 0; i < threads; i++)
    {
        for (int j = 0; j < max + 1 - min; j++)
        {
            reducedSortArray[i][j] = 0;
        }
    }

    for (int i = 0; i < threads; i++)
    {
        int startInd = (maxCount / threads) * i;
        int buf = (maxCount / threads) * (i + 1);
        int endInd = buf > maxCount ? maxCount : buf;

        th[i] = thread(CountNumbers, array, reducedSortArray[i], startInd, endInd, min);
    }
    for (int i = 0; i < threads; i++)
    {
        th[i].join();
    }
    for (int i = 0; i < threads; i++)
    {
        for (int j = 0; j < max + 1 - min; j++)
        {
            sortArray[j] += reducedSortArray[i][j];
        }
    }

    // Операция 3. Заполняем массив в сортированном порядке.
    //int t = 0;
    //for (int j = min; j < max + 1; j++) 
    //{
    //    while (sortArray[j - min] != 0) 
    //    {
    //        array[t] = j;
    //        sortArray[j - min]--;
    //        t++;
    //    }
    //}

    for (int i = 0; i < threads; i++)
    {
        int startInd = (maxCount / threads) * i;
        int buf = (maxCount / threads) * (i + 1);
        int endInd = buf > maxCount ? maxCount : buf;

        /*FillSortedArray(array, sortArray, startInd, endInd, min);*/

        th[i] = thread(FillSortedArray, array, sortArray, startInd, endInd, min);
    }
    for (int i = 0; i < threads; i++)
    {
        th[i].join();
    }

    endTime = omp_get_wtime();

    double res = endTime - startTime;

    for (int i = 0; i < threads; i++)
    {
        delete[] reducedSortArray[i];
    }
    delete[] reducedSortArray;

    delete[] th;
    delete[] maxArr;
    delete[] minArr;
    delete[] sortArray;
    

    return res;
}

double MultiThreadTestWithСonsecutiveFill(int* array, int maxCount, int threads) {

    double startTime;
    double endTime;

    CopyOrigArray(array, maxCount);

    startTime = omp_get_wtime();
    //Операция 1. Ищем минимальное и максимальное значение в массиве.
    int* maxArr = new int[threads];
    int* minArr = new int[threads];
    thread* th = new thread[threads];

    for (int i = 0; i < threads; i++)
    {
        int startInd = (maxCount / threads) * i;
        int buf = (maxCount / threads) * (i + 1);
        int endInd = buf > maxCount ? maxCount : buf;
        th[i] = thread(FindMaxMin, array, startInd, endInd, &maxArr[i], &minArr[i]);
    }

    for (int i = 0; i < threads; i++)
    {
        th[i].join();
    }
    int max = maxArr[0];
    int min = minArr[0];

    for (int i = 1; i < threads; i++)
    {
        if (maxArr[i] < max) max = maxArr[i];
        if (minArr[i] < min) min = minArr[i];
    }
    // Операция 2. Создаём массив для сортировки.
    int* sortArray = new int[max + 1 - min];
    for (int i = 0; i < max + 1 - min; i++) {
        sortArray[i] = 0;
    }
    int** reducedSortArray = new int* [threads];
    for (int i = 0; i < threads; i++)
    {
        reducedSortArray[i] = new int[max + 1 - min];
    }
    for (int i = 0; i < threads; i++)
    {
        for (int j = 0; j < max + 1 - min; j++)
        {
            reducedSortArray[i][j] = 0;
        }
    }

    for (int i = 0; i < threads; i++)
    {
        int startInd = (maxCount / threads) * i;
        int buf = (maxCount / threads) * (i + 1);
        int endInd = buf > maxCount ? maxCount : buf;

        th[i] = thread(CountNumbers, array, reducedSortArray[i], startInd, endInd, min);
    }
    for (int i = 0; i < threads; i++)
    {
        th[i].join();
    }
    for (int i = 0; i < threads; i++)
    {
        for (int j = 0; j < max + 1 - min; j++)
        {
            sortArray[j] += reducedSortArray[i][j];
        }
    }

    // Операция 3. Заполняем массив в сортированном порядке.
    int t = 0;
    for (int j = min; j < max + 1; j++) 
    {
        while (sortArray[j - min] != 0) 
        {
            array[t] = j;
            sortArray[j - min]--;
            t++;
        }
    }


    endTime = omp_get_wtime();

    double res = endTime - startTime;

    for (int i = 0; i < threads; i++)
    {
        delete[] reducedSortArray[i];
    }
    delete[] reducedSortArray;

    delete[] th;
    delete[] maxArr;
    delete[] minArr;
    delete[] sortArray;


    return res;
}
int mainFunc(int maxCount, int threadCount, int testCount) 
{
    double start;
    double end;

    double testRes = 0;

    int* array1 = new int[maxCount];
    int* array2 = new int[maxCount];
    origArray = new int[maxCount];
    GenerateArray(origArray, maxCount);

    start = omp_get_wtime();
    for (int i = 0; i < testCount; i++)
    {
        testRes += OneThreadTest(array1, maxCount);
    }
    end = omp_get_wtime();
    cout << "-----------------------------------------------------------" << endl;
    cout << "Размер массива " << maxCount <<" Число потоков " << threadCount<< " Число тестов " << testCount<< endl << endl;
    //cout << "Секунд ушло на тестирование одногопоточной функции: " << end - start << endl;
    cout << "Однопоток:" << testRes / testCount << endl;

    testRes = 0;
    start = omp_get_wtime();
    for (int i = 0; i < testCount; i++)
    {
        testRes += MultiThreadTestWithСonsecutiveFill(array2, maxCount, threadCount);
    }
    end = omp_get_wtime();

    //cout << "Секунд ушло на тестирование многопоточной функции: " << end - start << endl;
    cout << "Многопоток (посл. наполн.): " << testRes / testCount << endl << endl;

    testRes = 0;
    start = omp_get_wtime();
    for (int i = 0; i < testCount; i++)
    {
        testRes += MultiThreadTest(array2, maxCount, threadCount);
    }
    end = omp_get_wtime();

    //cout << "Секунд ушло на тестирование многопоточной функции: " << end - start << endl;
    cout << "Многопоток: " << testRes / testCount << endl << endl;

    CompareArays(array1, array2, maxCount);

    delete[] array1;
    delete[] array2;
    delete[] origArray;
    cin >> testRes;
    return 0;
}

int main()
{
    setlocale(LC_ALL, "Russian");

    int a = 0;
    for (int i = 10000000; i < 1280000001; i*=2)
    {
        a = mainFunc(i, 16, 10);
    }
    cin >> a;

    return 0;
}
