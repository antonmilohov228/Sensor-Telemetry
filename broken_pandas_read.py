import pandas as pd
from io import StringIO

csv_text = "id;value\n1;10\n2;20\n3;30\n"

print("--- Попытка 1: Ошибка чтения (стандартный sep=',') ---")
try:
    df = pd.read_csv(StringIO(csv_text))
    print("Dtypes:\n", df.dtypes)
    print("Mean value:", df["value"].mean()) 
except Exception as e:
    print(f"Ошибка: {e}")

print("\n--- Попытка 2: Исправленное чтение (sep=';') ---")
df_fixed = pd.read_csv(StringIO(csv_text), sep=";")
print("Dtypes:\n", df_fixed.dtypes)
print("Mean value:", df_fixed["value"].mean())

print("\n--- Тест 3: Пропуск в value ---")
csv_text_3 = "id;value\n1;10\n2;\n3;30\n"
df_miss = pd.read_csv(StringIO(csv_text_3), sep=";")
print("Данные с пропуском:\n", df_miss)
print("Dtypes с NaN:", df_miss.dtypes)
print("Среднее с учетом пропуска:", df_miss["value"].mean())
