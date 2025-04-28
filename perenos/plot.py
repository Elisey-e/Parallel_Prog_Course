import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns

# Данные из вашего вывода
data = {
    'processes': [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16],
    'execution_time': [22.4666, 11.4176, 7.8442, 5.9994, 4.8311, 4.0437, 3.4875, 3.0937, 
                      2.7689, 2.5239, 2.5053, 2.1970, 2.0826, 2.0007, 1.9022, 1.8220]
}

# Создаем DataFrame
df = pd.DataFrame(data)

# Настройка стиля графика
plt.figure(figsize=(10, 6))
sns.set_style("whitegrid")
plt.title('Зависимость времени выполнения от числа процессов', fontsize=14)
plt.xlabel('Число процессов', fontsize=12)
plt.ylabel('Время выполнения (секунды)', fontsize=12)

# Построение графика
plt.plot(df['processes'], df['execution_time'], 
         marker='o', 
         linestyle='-', 
         color='b', 
         linewidth=2, 
         markersize=8)

# Добавление значений точек
for i, row in df.iterrows():
    plt.text(row['processes'], row['execution_time']+0.2, 
             f"{row['execution_time']:.2f}s", 
             ha='center', 
             fontsize=9)

# Оптимизация масштаба
plt.ylim(0, df['execution_time'].max() * 1.1)

# Горизонтальные линии для лучшей читаемости
plt.grid(axis='y', linestyle='--', alpha=0.7)

# Сохранение графика
plt.tight_layout()
plt.savefig('execution_time_vs_processes.png', dpi=300)
plt.show()
