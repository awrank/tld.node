tld
========


Установка
----

Загрузить исходные файлы модуля с Github на диск.

    $ npm install <путь к иcходным файлам>

Использование
----

подключить модуль к проекту:

```javascript
var tld_module = require('tld.node');
```


### function load(active, reserved)

Загрузка баз TLD и инициализация модуля

где параметры,
- active - активные домены
- reserved - зарезервироване домены (или домены в черном списке)

могут быть представлены как Buffer или Array.

Загрузка из файла:

```javascript
var active_tld = fs.readFileSync("base.dat");
var reserved_tld  = fs.readFileSync("guide.dat");
                    
tld_module.load(active_tld, reserved_tld);
```

Инициализация данными из массива:

```javascript
var active_tld = ['com','ua','com.ua','ru','*.ar','!uba.ar','рф'];
var reserved_tld = ['example.com'];
                            
tld_module.load(active_tld, reserved_tld);
```


### function tld(uri)

Проверка домена по базе TLD и выделения домена верхнего уровня и суб-доменов.

![tld](https://dl.dropbox.com/u/12394766/awrank/tld.png)

Возвращает в ответе функции объект в котором следующие поля:

- status 
  - 0 = Success
  - 1 = Invalid      _ /* TLD not acceptable (reserved, deprecated, etc.) */ _
  - 2 = Not TLD      _ /* no '.' in the URI */ _
  - 3 = Bad URI      _ /* URI includes two '.' one after another */ _
  - 4 = Not found 
- domain - выделенный домен верхнего уровня
- sub_domains[] - выделенные суб домены, массив (если нет суб доменов должен содержать пустой массив)


**примеры,**

```javascript
tld_module.tld("maps.google.com")
```

Результат функции будет:
- status = 0
- domain = "com"
- sub_domains = ["maps","google"]


```javascript
tld_module.tld("vasya.petrov.dn.ua")
```

Результат функции tld будет:
- status = 0
- domain = "dn.ua"
- sub_domains = ["vasya","petrov"]


```javascript
tld_module.tld("test.kk")
```

результат функции будет:
- status = 4

Тестирование
----
предварительное условие, [Vows](http://vowsjs.org/) фреймворк должен быть установлен

    $ cd tests 
    $ ./test.sh

