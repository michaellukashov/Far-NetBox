Плагин "NetBox" для Far Manager 2.0
***********************************

FTP/FTPS/SFTP/WebDAV клиент для Far.

1. Общие сведения о плагине
   Плагин реализует клиентскую часть протоколов FTP, FTPS, SFTP и WebDAV.
   FTP и WebDAV базируется на библиотеке libcurl (http://curl.haxx.se).
   SFTP реализован на базе библиотеки libssh2 (http://www.libssh2.org).
   Парсер xml работает через библиотеку TinyXML
   (http://sourceforge.net/projects/tinyxml).

2. Использование префикса командной строки
   Доступ к удалённому серверу возможен как через собственное хранилище
   сессий, так и через префикс.
   Возможны два варианта использования префикса:
     a. NetBox:Protocol://[[User]:[Password]@]HostName[:Port][/Path]
        где Protocol - имя протокола (ftp/ftps/sftp/http/https)
            User - имя пользователя
            Password - пароль пользователя
            HostName - имя хоста
            Port - номер порта
            Path - путь

     b. (ftp|sftp|http|https)://[[User]:[Password]@]HostName[:Port][/Path]
        где (ftp|sftp|http|https) - имя протокола
            User - имя пользователя
            Password - пароль пользователя
            HostName - имя хоста
            Port - номер порта
            Path - путь

   Например, следующие команды в Far'e позволят просматривать хранилище
   svn с исходными кодами Far:
     a. NetBox: http://farmanager.com/svn/trunk
     b. http://farmanager.com/svn/trunk

3. Ключи при использовании протокола SFTP
   Файл с ключами (публичным и приватным) должен быть в формате OpenSSL.
   Если вы используете putty или основанный на нем WinSCP, экcпортируйте
   ключи программой puttigen из формата Putty в формат OpenSSL.

4. Фичи
   Комбинация клавиш в панели:
   Ctrl+Alt+Ins: Копирование текущего URL в буфер обмена (вместе с паролем).
   Shift+Alt+Ins: Копирование текущего URL в буфер обмена (без пароля).

5. Установка:
   Распакуйте содержимое архива в каталог плагинов Far (...Far\Plugins).

6. Хозяйке на заметку:
   Данный плагин предоставляется "as is" ("как есть"). Автор не несет 
   ответственности за последствия использования данного плагина.

Артём Сеничев (artemsen@gmail.com)
              http://code.google.com/p/farplugs
Михаил Лукашов (michael.lukashov@gmail.com)
               https://github.com/michaellukashov/Far-NetBox
