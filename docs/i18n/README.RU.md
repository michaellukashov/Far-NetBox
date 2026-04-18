Far-NetBox: клиент SFTP/FTP(S)/SCP/WebDAV/S3 для Far Manager 3.0 (x86/x64/ARM64)
==============

| Сборка        | Статус                                                                                                                                                                          |
|---------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| GitHub Actions| [![build](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg)](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml)|
| AppVeyor      | [![Build status](https://ci.appveyor.com/api/projects/status/91lhdjygkenumcmv?svg=true)](https://ci.appveyor.com/project/michaellukashov/far-netbox)                               |

На основе WinSCP версии 6.5.1 Copyright (c) 2000-2025 Martin Prikryl

На основе WinSCP как плагина для FAR: SFTP/FTP/SCP клиент для FAR версии 1.6.2 Copyright (c) 2000-2009 Martin Prikryl

Код SSH и SCP основан на PuTTY 0.81 Copyright (c) 1997-2024 Simon Tatham

Код FTP основан на FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse

1. Общие сведения о плагине

   Плагин реализует клиентскую часть протоколов SFTP, FTP, SCP, FTPS, WebDAV и S3.
   SFTP, FTP, SCP, FTPS протоколы реализованы на основе плагина WinSCP [http://winscp.net/eng/download.php/](http://winscp.net/eng/download.php/)
   Поддержка протокола WebDAV реализована на основе библиотеки neon [http://www.webdav.org/neon/](http://www.webdav.org/neon/).
   Парсер xml работает c помощью библиотеки TinyXML [http://sourceforge.net/projects/tinyxml/](http://sourceforge.net/projects/tinyxml/).

2. Использование префикса командной строки

   Доступ к удалённому серверу возможен как через собственное хранилище сессий, так и через префикс.

   Возможны три варианта использования префикса:
     a. NetBox:Protocol://[[User]:[Password]@]HostName[:Port][/Path]

        где Protocol - имя протокола (ftp|ftps|ftpes|ssh|scp|sftp|dav|davs|http|https|s3|s3plain)
            User - имя пользователя
            Password - пароль пользователя
            HostName - имя хоста
            Port - номер порта
            Path - путь

     b. (sftp|ftp|scp|ftps|webdav|s3)://[[User]:[Password]@]HostName[:Port][/Path]
        где (sftp|ftp|scp|ftps|webdav|s3) - имя протокола
            User - имя пользователя
            Password - пароль пользователя
            HostName - имя хоста
            Port - номер порта
            Path - путь

      c. NetBox:SessionName[/Path]
         где SessionName - сохранённое имя сессии
             Path - путь

    Особенности работы с FTP серверами допускающими анонимный логин.

    ВНИМАНИЕ!

    Если Вы подключаетесь к FTP серверу допускающему анонимный логин,
    то у Вас возможно сообщение сервера о неудачном логине с
    использованием пустого пароля. В таком случае вместо анонимного
    логина попробуйте нормальный логин с использованием пары
    "логин - пароль" такого вида:

    Логин: Anonymous или для некоторых серверов anonimous (это зависит от настройки данного FTP сервера!)
    Пароль: user@server.com

    Данная комбинация "имя-пароль" является стандартной формой
    анонимного логина определённой в спецификациях протокола FTP и
    допускается на всех разрешающих анонимный логин FTP серверах.


   Например, следующие команды в Far'e позволят просматривать хранилище
   svn с исходными кодами Far:
     a. NetBox: http://farmanager.com/svn/trunk
     b. http://farmanager.com/svn/trunk

3. Ключи при использовании протоколов SFTP, SCP

   Файл с ключами (публичным и приватным) должен быть в формате Putty (.ppk).

4. Фичи

   Комбинация клавиш в панели:
   Ctrl+Alt+Ins: Копирование текущего URL в буфер обмена (вместе с паролем).
   Shift+Alt+Ins: Копирование текущего URL в буфер обмена (без пароля).

5. Установка

   Распакуйте содержимое архива в каталог плагинов Far (...Far\Plugins).

6. Сборка из исходников

   ### Требования

   * Visual Studio 2022 (с workload "Desktop development with C++")
   * CMake 3.15 или новее
   * Ninja (рекомендуется)

   ### Быстрая сборка (с использованием пакетных файлов)

   В репозитории есть пакетные файлы в корне, которые автоматизируют процесс сборки:

   - `build-all.bat` — сборка всех поддерживаемых платформ (x86, x64, ARM64)
   - `build-x64.bat` — сборка x64 с отладочной информацией (RelWithDebugInfo)
   - `build-x86.bat` — сборка x86 с отладочной информацией
   - `build-arm64.bat` — сборка ARM64 с отладочной информацией

   Просто запустите нужный пакетный файл из корня репозитория. Скрипты автоматически настроят окружение Visual Studio и вызовут CMake с подходящими параметрами.

   **Примечание:** Пакетные файлы по умолчанию используют путь к Visual Studio 2022 Professional. Если вы используете другую редакцию (например, Community) или другой путь установки, измените путь к `vcvarsall.bat` в соответствующем пакетном файле.

   ### Ручная сборка

   1. Откройте командную строку и настройте окружение Visual Studio:

      ```batch
      "%VS170COMNTOOLS%..\..\VC\vcvarsall.bat" x86_amd64
      ```

      Или используйте полный путь:

      ```batch
      "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvarsall.bat" x86_amd64
      ```

   2. Настройте и соберите с помощью CMake (пример для x64):

      ```batch
      cmake -S . -B build-RelWithDebugInfo -G "Ninja" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
      cmake --build build-RelWithDebugInfo -j
      ```

      Для генерации решения Visual Studio 2022:

      ```batch
      cmake -S . -B build-RelWithDebugInfo -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=RelWithDebugInfo -DOPT_CREATE_PLUGIN_DIR=ON
      cmake --build build-RelWithDebugInfo -j
      ```

      Собранный плагин будет находиться в папке `build-RelWithDebugInfo\Plugins\NetBox\x64\` (или соответствующей подпапке платформы).

Ссылки
------

* Основная страница проекта: [https://github.com/michaellukashov/Far-NetBox](https://github.com/michaellukashov/Far-NetBox)
* Форум Far Manager: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* Обсуждение Far-NetBox (на русском): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* Обсуждение Far-NetBox (на английском): [http://forum.farmanager.com/viewtopic.php?f=39&t=6638](http://forum.farmanager.com/viewtopic.php?f=39§t=6638)
* Последние сборки: <https://nightly.link/michaellukashov/Far-NetBox/workflows/release/main?preview>

Данный плагин предоставляется "as is" ("как есть"). Автор не несет
ответственности за последствия использования данного плагина.