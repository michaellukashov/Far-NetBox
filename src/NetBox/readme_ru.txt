NetBox: SFTP/FTP/FTPS/SCP/WebDAV клиент для Far Manager 2.0
===============

1. Общие сведения о плагине
   Плагин реализует клиентскую часть протоколов SFTP, FTP, SCP, FTPS и WebDAV.
   SFTP, FTP, SCP, FTPS протоколы реализованы на основе плагина WinSCP (http://winscp.net/eng/download.php)
   WebDAV базируется на библиотеке libcurl (http://curl.haxx.se).
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

     b. (sftp|ftp|scp|ftps|http|https)://[[User]:[Password]@]HostName[:Port][/Path]
        где (sftp|ftp|scp|ftps|http|https) - имя протокола
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

Данный плагин предоставляется "as is" ("как есть"). Автор не несет 
ответственности за последствия использования данного плагина.
