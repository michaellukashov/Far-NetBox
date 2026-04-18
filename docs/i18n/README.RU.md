Far-NetBox: клиент SFTP/FTP(S)/SCP/WebDAV/S3 для Far Manager 3.0 (x86/x64/ARM64)
==============

| Сборка        | Статус                                                                                                                                                                          |
|---------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| GitHub Actions| [![build](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml/badge.svg)](https://github.com/michaellukashov/Far-NetBox/actions/workflows/release.yml)|
| AppVeyor      | [![Build status](https://ci.appveyor.com/api/projects/status/91lhdjygkenumcmv?svg=true)](https://ci.appveyor.com/project/michaellukashov/far-netbox)                               |

На основе WinSCP версии 6.5.6 Copyright (c) 2000-2025 Martin Prikryl

На основе WinSCP как плагина для FAR: SFTP/FTP/SCP клиент для FAR версии 1.6.2 Copyright (c) 2000-2009 Martin Prikryl

Код SSH и SCP основан на PuTTY 0.81 Copyright (c) 1997-2024 Simon Tatham

Код FTP основан на FileZilla 2.2.32 Copyright (c) 2001-2007 Tim Kosse

Криптография основана на OpenSSL 3.3.7 Copyright (c) 1998-2025 The OpenSSL Project

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

6. Сборка

   Инструкции по сборке см. в [Руководстве разработчика](DEVELOPER.md).

Ссылки
------

* Основная страница проекта: [https://github.com/michaellukashov/Far-NetBox](https://github.com/michaellukashov/Far-NetBox)
* Форум Far Manager: [http://forum.farmanager.com/](http://forum.farmanager.com/)
* Обсуждение Far-NetBox (на русском): [http://forum.farmanager.com/viewtopic.php?f=5&t=6317](http://forum.farmanager.com/viewtopic.php?f=5&t=6317)
* Обсуждение Far-NetBox (на английском): [http://forum.farmanager.com/viewtopic.php?f=39§t=6638](http://forum.farmanager.com/viewtopic.php?f=39§t=6638)
* Последние сборки: <https://nightly.link/michaellukashov/Far-NetBox/workflows/release/main?preview>

Данный плагин предоставляется "as is" ("как есть"). Автор не несет
ответственности за последствия использования данного плагина.