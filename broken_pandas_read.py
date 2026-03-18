import pandas as pd
from io import StringIO

csv_text = "id;value\n1;10\n2;20\n3;30\n"

print("--- Часть 1: Ошибка чтения (по умолчанию sep=',') ---")
df_bad = pd.read_csv(StringIO(csv_text))
print("Dtypes:\n", df_bad.dtypes)
try:
    print(df_bad["value"].mean())
except KeyError as e:
    print(f"Ошибка KeyError: столбца {e} не существует, так как таблица склеилась в один столбец 'id;value'")

print("\n--- Часть 2: Исправленное чтение (sep=';') ---")
df_fixed = pd.read_csv(StringIO(csv_text), sep=";")
print("Dtypes:\n", df_fixed.dtypes)
print("Среднее значение value:", df_fixed["value"].mean())

print("\n--- Часть 3: Тест 2 (Пропуск в value) ---")
csv_text_3 = "id;value\n1;10\n2;\n3;30\n"
df_miss = pd.read_csv(StringIO(csv_text_3), sep=";")
print("Dtypes:\n", df_miss.dtypes)
print("Как выглядит таблица:\n", df_miss)
print("Среднее с учетом пропуска:", df_miss["value"].mean())
