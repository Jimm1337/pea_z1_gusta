# PEA_GUSTA

Rozwiązania zadań projektowych przedmiotu Projektowanie Efektywnych Algorytmów (Politechnika Wrocławska).

- Wymagania sprzętowe:
    - Windows 10/11
    - Procesor obsługujący SSE2 oraz SSE4.2

- Biblioteki zewnętrzne:
    - ini.h (Dostarczane w folderze external)
    - {fmt} (Dostarczane przez dostarczany z projektem menadżer pakietów)

- Wymagania do kompilacji:
    - MSVC 2022 (Najnowszy)
    - CMake 3.22+
    - Ninja/Make/NMake ... (Program do budowy kompatybilny z CMake)

## 1. Plik wejściowy:

```txt
(liczba wierzchołków)
- - - -
- - - -
- - - -
- - - -
```

- koszty (-)
- brak połączenia (-1)

### Przykład:

```txt
6
-1 2 3 4 5 6
2 -1 3 4 5 6
3 3 -1 4 5 6
4 4 4 -1 5 6
5 5 5 5 -1 6
6 6 6 6 6 -1
```

## 2. Plik konfiguracyjny:

```ini
[instance]
input_path = (ścieżka do pliku wejściowego)
symmetric = (true/false, graf symetryczny?)
full = (true/false, graf pełny?)
[optimal]
path = (ścieżka optymalnego rozwiązania)
cost = (koszt optymalnego rozwiązania)
[random]
millis = (maksymalny czas w ms algorytmu losowego)
[tabu_search]
itr = (liczba iteracji algorytmu tabu search)
max_itr_no_improve = (maksymalna liczba iteracji bez poprawy algorytmu tabu search)
tabu_itr = (liczba iteracji w macierzy tabu)
[genetic]
itr = (liczba iteracji algorytmu genetycznego)
population_size = (liczba osobników w populacji, do reprodukcji)
children_per_itr = (maksymalna liczba dzieci na iterację)
max_children_per_pair = (maksymalna liczba dzieci na parę)
max_v_count_crossover = (maksymalna liczba wierzchołków biorących udział w krzyżowaniu)
mutations_per_1000 = (liczba mutacji na 1000 szans na mutację)
```

- parametry algorytmów wymagane tylko gdy używane
- sekcja optimal opcjonalna
- ścieżka do pliku wejściowego absolutna lub względna względem pliku konfiguracyjnego

### Przykład:

```ini
[instance]
input_path = ./input.txt
symmetric = true
full = false
[optimal]
path = 1 2 5 0 3 4
cost = 123
[random]
millis = 1000
[tabu_search]
itr = 1000
max_itr_no_improve = 100
tabu_itr = 10
[genetic]
itr = 1000
population_size = 100
children_per_itr = 100
max_children_per_pair = 10
max_v_count_crossover = 10
mutations_per_1000 = 10
```

## 3. Uruchamianie:

### Zadanie 1

```powershell
> ./pea_gusta_zadanie_1.exe --config=(ścieżka do pliku konfiguracyjnego) -(algorytm)
```

- Dostępne algorytmy:
    - BruteForce (-bf)
    - NearestNeighbour (-nn)
    - Random (-r)
        - Parametr: liczba ms powtarzania algorytmu

#### Przykład:

```powershell
> ./pea_gusta_zadanie_1.exe --config=./plik_konfiguracyjny.ini -nn
```

### Zadanie 2

```powershell
> ./pea_gusta_zadanie_2.exe --config=(ścieżka do pliku konfiguracyjnego) -(algorytm)
```

- Dostępne algorytmy:
    - Branch and Bound BFS (-bb)
    - Branch and Bound DFS (-bd)
    - Branch and Bound Least Cost (-lc)

#### Przykład:

```powershell
> ./pea_gusta_zadanie_2.exe --config=./plik_konfiguracyjny.ini -lc
```

### Zadanie 3

```powershell
> ./pea_gusta_zadanie_3.exe --config=(ścieżka do pliku konfiguracyjnego) -(algorytm)
```

- Dostępne algorytmy:
    - Tabu Search (-ts)

#### Przykład:

```powershell
> ./pea_gusta_zadanie_3.exe --config=./plik_konfiguracyjny.ini -ts
```

### Zadanie 4

```powershell
> ./pea_gusta_zadanie_4.exe --config=(ścieżka do pliku konfiguracyjnego) -(algorytm)
```

- Dostępne algorytmy:
    - Genetyczny (-g)

#### Przykład:

```powershell
> ./pea_gusta_zadanie_4.exe --config=./plik_konfiguracyjny.ini -g
```

## Dane:

### Folder data:

Przykładowe dane i pliki konfiguracyjne.

#### configs: Pliki konfiguracyjne.

- dr_lopuszynski: Dane dr. Łopuszyńskiego
- dr_mierzwa: Dane dr. Mierzwy
- rand_atsp: Losowe dane asymetryczne
- rand_tsp: Losowe dane symetryczne
- tsplib_atsp: Dane TSPLIB asymetryczne
- tsplib_tsp: Dane TSPLIB symetryczne

## Pomiary:

Program zakłada następującą strukturę na potrzeby pomiarów:

- (Plik wykonywalny programu)
- Folder data
    - Folder rand_tsp
        - Folder configs
            - (N)_rand_s.ini, gdzie N jest całkowite i należy do [5, 19]
        - (N)_rand_s.txt, gdzie N jest całkowite i należy do [5, 19]
    - Folder rand_atsp
        - Folder configs
            - (N)_rand_as.ini, gdzie N jest całkowite i należy do [5, 18]
        - (N)_rand_as.txt, gdzie N jest całkowite i należy do [5, 18]
    - folder tsplib_tsp [Tylko dla zadania 3 i 4]
        - Folder configs
            - 127_bier127.ini
            - 159_u159.ini
            - 180_brg180.ini
            - 225_tsp225.ini
            - 264_pr264.ini
            - 299_pr299.ini
            - 318_linhp318.ini
        - 127_bier127.txt
        - 159_u159.txt
        - 180_brg180.txt
        - 225_tsp225.txt
        - 264_pr264.txt
        - 299_pr299.txt
        - 318_linhp318.txt
    - folder tsplib_atsp [Tylko dla zadania 3 i 4]
        - Folder configs
            - 34_ftv33.ini
            - 43_p43.ini
            - 53_ft53.ini
            - 65_ftv64.ini
            - 71_ftv70.ini
            - 100_kro124p.ini
        - 34_ftv33.txt
        - 43_p43.txt
        - 53_ft53.txt
        - 65_ftv64.txt
        - 71_ftv70.txt
        - 100_kro124p.txt

### Uruchomienie pomiarów:

#### Zadanie 1:

```powershell
> ./pea_gusta_zadanie_1.exe --measure --verbose -bf -nn -r
```

#### Zadanie 2:

```powershell
> ./pea_gusta_zadanie_2.exe --measure --verbose -bb -bd -lc
```

#### Zadanie 3:

```powershell
> ./pea_gusta_zadanie_3.exe --measure --verbose -ts
```

#### Zadanie 4:

```powershell
> ./pea_gusta_zadanie_4.exe --measure --verbose -g
```

Flaga `--verbose` jest opcjonalna i powoduje wyświetlanie informacji o postępach pomiarów. Nie trzeba uruchamiać
wszystkich algorytmów z zadania, można wybrać tylko interesujące nas algorytmy.

### Wyniki:
Wyniki będą dostępne w folderze pliku wykonywalnego.

## Rozwiązania problemów z uruchomieniem:

1. Uruchomić w terminalu
2. Zainstalować MSVC CRT (VC_redist.x64.exe)
