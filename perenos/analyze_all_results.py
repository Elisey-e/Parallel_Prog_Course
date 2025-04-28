import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import numpy as np

# Загрузка данных
df = pd.read_csv('benchmark_results.csv')

# Расчет базового времени выполнения (на 1 процессе)
# Так как в данных нет запусков с 1 процессом, будем оценивать базовое время
# как время на 2 процессорах * 2 (предполагая идеальное ускорение)
df['base_time'] = df.apply(lambda row: row['execution_time'] * row['processes'] if row['processes'] == 2 else np.nan, axis=1)
base_times = df.groupby(['K', 'M'])['base_time'].first().reset_index()

# Объединяем с основными данными для расчета ускорения и эффективности
df = pd.merge(df, base_times, on=['K', 'M'], suffixes=('', '_base'))

# Расчет ускорения и эффективности
df['speedup'] = df['base_time'] / df['execution_time']
df['efficiency'] = (df['speedup'] / df['processes']) * 100

# Сохранение расчетных данных
df.to_csv('performance_metrics.csv', index=False)

# Визуализация результатов
plt.figure(figsize=(18, 12))
sns.set_style("whitegrid")
plt.suptitle('Анализ параллельной производительности', fontsize=16)


# График 3: Время выполнения
plt.subplot(2, 2, 3)
for p in sorted(df['processes'].unique()):
    subset = df[df['processes'] == p]
    plt.plot(subset['K']*subset['M'], subset['execution_time'], 'o-', 
             label=f'{p} процессов')
plt.title('Время выполнения от размера задачи')
plt.xlabel('Размер задачи (K*M)')
plt.ylabel('Время (сек)')
plt.xscale('log')
plt.yscale('log')
plt.legend()
plt.grid(True, which="both", ls="-")

# График 4: Время выполнения для разных конфигураций
plt.subplot(2, 2, 4)
sns.lineplot(data=df, x='processes', y='execution_time', 
             hue='K', style='M', markers=True, dashes=False)
plt.title('Время выполнения для разных K и M')
plt.xlabel('Число процессов')
plt.ylabel('Время (сек)')
plt.yscale('log')
plt.xscale('log')
plt.grid(True)

plt.tight_layout()
plt.savefig('parallel_performance_analysis.png', dpi=300, bbox_inches='tight')
plt.show()

# Дополнительный анализ - таблица с метриками
print("\nСводная таблица производительности:")
summary = df.pivot_table(index=['K', 'M'], columns='processes', 
                        values=['execution_time', 'speedup', 'efficiency'])
print(summary.to_markdown(floatfmt=".2f"))