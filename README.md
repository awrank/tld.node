tld
========


Установка
----


Использование
----

подключить модуль к проекту:

```javascript
var tld_module = require('tld.node');
```


### void load(active, reserved)

загрузка баз TLD и инициализация модуля

где параметры,
- active - активные домены
- reserved - зарезервироване домены (или домены в черном списке)

могут быть представлены как Buffer или Array.

загрузка из файла

```javascript
var active_tld = fs.readFileSync("base.dat");
var reserved_tld  = fs.readFileSync("guide.dat");
                    
tld_module.load(active_tld, reserved_tld);
```

загрузка данными из массива

```javascript
var active_tld = ['com','ua','com.ua','ru','*.ar','!uba.ar','рф'];
var reserved_tld = ['example.com'];
        	
tld_module.load(active_tld, reserved_tld);
```


### function tld(uri)

Проверка домена по базе TLD возвращает в ответе функции объект в котором следующие поля:

- status 
  - 0 = Success
  - 1 = Invalid      /* TLD not acceptable (reserved, deprecated, etc.) */
  - 2 = Not TLD   /* no '.' in the URI */
  - 3 = Bad URI   /* URI includes two '.' one after another */
  - 4 = Not found 
- domain - выделенный домен верхнего уровня
- sub_domains[] - выделенные суб домены, массив (если нет суб доменов должен содержать пустой массив)



**примеры,**

```javascript
tld_module.tld("maps.google.com")
```

результат функции будет:
- status = 0
- domain = "com"
- sub_domains = ["maps","google"]


```javascript
tld_module.tld("vasya.petrov.dn.ua")
```

результат функции tld будет:
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
предварительное условие: [Vows](http://vowsjs.org/) фреймворк должен быть установлен

```bash
cd tests 
./test.sh
```
