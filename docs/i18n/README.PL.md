Far-NetBox: Klient SFTP/FTP(S)/SCP/WebDAV/S3 dla Far Manager 3.0 x86/x64/ARM64
==============

| Środowisko      | Status budowania                                                                                                                                                                       |
|-----------------|----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| GitHub Actions  | [![build](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg)](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml)        |
| AppVeyor        | [![Build status](https://ci.appveyor.com/api/projects/status/91lhdjygkenumcmv?svg=true)](https://ci.appveyor.com/project/michaellukashov/far-netbox)                                   |

Bazuje na [WinSCP](http://winscp.net/eng/index.php) wersja 6.5.1 Copyright (c) 2000-2025 Martin Prikryl

Bazuje na [WinSCP jako wtyczka FAR: SFTP/FTP/SCP klient dla FAR wersja 1.6.2](http://winscp.net/download/winscpfar162setup.exe) Copyright (c) 2000-2009 Martin Prikryl

Kod SSH i SCP bazuje na PuTTY 0.81 Copyright (c) 1997-2024 Simon Tatham

Kod FTP bazuje na FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse

Jak skompilować ze źródeł
=======================

### Wymagania wstępne

* Visual Studio 2022 (z pakietem „Desktop development with C++”)
* CMake 3.15 lub nowszy
* Ninja (rekomendowane; opcjonalnie jeśli używasz generatora Visual Studio)

### Szybka kompilacja (przy użyciu plików wsadowych)

Repozytorium zawiera gotowe pliki wsadowe (batch files) w katalogu głównym, które automatyzują proces budowania:

- `build-all.bat` – buduje wszystkie obsługiwane platformy (x86, x64, ARM64)
- `build-x64.bat` – buduje wersję x64 z informacjami debugowania (RelWithDebugInfo)
- `build-x86.bat` – buduje wersję x86 z informacjami debugowania
- `build-arm64.bat` – buduje wersję ARM64 z informacjami debugowania

Wystarczy uruchomić żądany plik wsadowy z katalogu głównego repozytorium. Skrypty automatycznie skonfigurują środowisko Visual Studio i wywołają CMake z odpowiednimi parametrami.

**Uwaga:** Pliki wsadowe domyślnie odwołują się do ścieżki Visual Studio 2022 Professional. Jeśli używasz innej edycji (np. Community) lub innej lokalizacji instalacji, dostosuj ścieżkę do `vcvarsall.bat` w odpowiednim pliku wsadowym.

### Ręczna kompilacja

Jeśli wolisz ręcznie wykonać kroki, postępuj zgodnie z poniższymi instrukcjami:

1. Otwórz wiersz poleceń i skonfiguruj środowisko Visual Studio:

   ```batch
   "%VS170COMNTOOLS%..\..\VC\vcvarsall.bat" x86_amd64
   ```

   Lub użyj pełnej ścieżki do instalacji Visual Studio:

   ```batch
   "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
   ```

2. Skonfiguruj i zbuduj przy użyciu CMake (przykład dla x64):

   ```batch
   cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
   cmake --build build-RelWithDebugInfo -j
   ```

   Aby wygenerować rozwiązanie Visual Studio 2022:

   ```batch
   cmake -S . -B build-RelWithDebugInfo -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
   cmake --build build-RelWithDebugInfo -j
   ```

   Zbudowany plugin będzie znajdował się w katalogu `build-RelWithDebugInfo\Plugins\NetBox\x64\` (lub odpowiednim podkatalogu platformy).

Linki
-----

* Strona główna projektu: [https://github.com/michaellukashov/Far-NetBox](https://github.com/michaellukashov/Far-NetBox)
* Forum Far Manager: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* Dyskusja o Far-NetBox (po rosyjsku): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* Dyskusja o Far-NetBox (po angielsku): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39§t=6638)
* Najnowsze buildy: <https://nightly.link/michaellukashov/Far-NetBox/workflows/release/main?preview>

Licencja
--------

NetBox jest [wolnym](http://www.gnu.org/philosophy/free-sw.html) oprogramowaniem: możesz używać, rozpowszechniać i/lub modyfikować zgodnie z warunkami [GNU General Public License](http://www.gnu.org/licenses/gpl.html) opublikowanej przez Free Software Foundation, aktualnie wersja 3 Licencji, lub (jak wolisz) dowolnej kolejnej wersji.

NetBox jest rozpowszechniany w nadziei, że będzie użyteczny, ale bez jakiejkolwiek gwarancji; nawet bez dorozumianej gwarancji przydatności handlowej lub przydatności do określonego celu. Więcej szczegółów w [GNU General Public License](http://www.gnu.org/licenses/gpl.html).