# Używanie:
### 1. Plik wejściowy:
```txt
(liczba wierzchołków)
- - - -
- - - -
- - - -
- - - -
```
- koszty (-)

- brak połączenia (-1)

#### Przykład:
```txt
6
-1 2 3 4 5 6
2 -1 3 4 5 6
3 3 -1 4 5 6
4 4 4 -1 5 6
5 5 5 5 -1 6
6 6 6 6 6 -1
```

### 2. Plik konfiguracyjny:
```ini
[instance]
input_path = (ścieżka do pliku wejściowego)
algorithm_parameter = (parametr algorytmu)
[optimal]
path = (ścieżka optymalnego rozwiązania)
cost = (koszt optymalnego rozwiązania)
```

- parametr algorytmu opcjonalny
- sekcja optimal opcjonalna 
- ścieżka absolutna lub względna względem pliku konfiguracyjnego

#### Przykład:
```ini
[instance]
input_path = ../input.txt
algorithm_parameter = 50
[optimal]
path = 1 2 5 0 3 4
cost = 123
```

### 3. Uruchamianie:
```powershell
> ./pea_z1_gusta.exe --config=(ścieżka do pliku konfiguracyjnego) -(algorytm)
```

- Dostępne algorytmy: 
  - BruteForce (-bf)
  - NearestNeighbour (-nn)
  - Random (-r)
    - Parametr: liczba ms powtarzania algorytmu

#### Przykład:
```powershell
> ./pea_z1_gusta.exe --config=../data/configs/config.ini -nn
```
