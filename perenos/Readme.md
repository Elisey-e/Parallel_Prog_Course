Для оценки ускорения и эффективности параллельной реализации мы проведем теоретический анализ и практические измерения. Вот комплексное решение:

### 1. Теоретическая оценка

Закон Амдаля:
```
S = 1 / ((1 - p) + p/N)
```
где:
- S - ускорение
- p - доля параллелизуемого кода
- N - число процессов

Для нашей задачи (явная схема):
- Основные вычисления хорошо параллелятся (p ≈ 0.95)
- Коммуникационные затраты растут с числом процессов

### 3. Методика измерений

1. **Ускорение (Speedup)**:
   ```
   S(N) = T(1) / T(N)
   ```
   где T(N) - время выполнения на N процессах

2. **Эффективность (Efficiency)**:
   ```
   E(N) = S(N) / N * 100%
   ```

3. **Метрики для анализа**:
   - Зависимость ускорения от числа процессов
   - Зависимость эффективности от числа процессов
   - Зависимость времени выполнения от размера задачи

### 4. Интерпретация результатов

1. **Идеальное ускорение**: Линейный рост с числом процессов
2. **Реальное ускорение**: Учитывает:
   - Накладные расходы на коммуникации
   - Неидеальную балансировку нагрузки
   - Последовательные участки кода

3. **Эффективность**:
   - 100% - идеальный случай
   - 70-90% - хороший результат
   - <50% - плохая масштабируемость

### 5. Рекомендации по запуску

1. Скомпилируйте программу:
   ```bash
   mpicc -o transport_mpi transport_mpi_benchmark.c -lm
   ```

2. Запустите тесты:
   ```bash
   chmod +x run_benchmarks.sh
   ./run_benchmarks.sh
   ```

3. Проанализируйте результаты:
   ```bash
   python3 analyze_results.py
   ```

Этот комплекс позволит вам получить полную картину о масштабируемости вашей параллельной реализации и выявить оптимальные параметры запуска для различных размеров задач.