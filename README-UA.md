# Вбудоване ПЗ для Svitlo Pulse - датчика на базі ESP32-C3 для Svitlobot (в процесі розробки)

[EN](./README-UA.md) | UA

Це прошивка для простого датчика мережі, збудованого на основі плати ESP32-C3 Super Mini. Вона інтегрується з https://svitlobot.in.ua/ - безкоштовним волонтерським проєктом для моніторингу стану електропостачання. Пристрій відправляє пінг до серверу Svitlobot кожну хвилину. Коли мережа вимикається, пристрій припиняє відправку пінгів, таким чином Svitlobot виявляє відключення електроенергії через певний час.

## Як цим користуватися ?

Детальні інструкції по збірці та налаштуванню ви зможете знайти у [Wiki](https://github.com/SvitloPulse/esp32-firmware/wiki) (див. розділи у боковому меню).

1. Придбайте плату ESP32-C3 Super Mini, або іншу сумісну ESP32-C3 плату з можливістю підключення до ПК по USB.
1. Завантажте останню версію вбудованого ПЗ у пристрій (див. сторінку з релізами, скоро буде) одним з доступних вам способів: 
    - через Svitlo Pulse Веб Інсталятор (скоро буде)
    - [ESPHome Web Installer](https://web.esphome.io/)
    - Або ви можете зібрати та завантажити ПЗ самостійно, див. нижче.
1. Налаштувати пристрій за допомогою Svitlo Pulse Веб Інсталятор (скоро буде) або Андроід застосунку (скоро буде)
1. Підключіть пристрій до 5В USB зярядного пристрою від смартфона.

## Як зрозуміти, що все працює коректно

Опис нижче стосується плати ESP32-C3 Super Mini.

1. Після ввімкнення, пристрій швидко блимне 5 разів синім світлодіодом.
1. Якщо ви встановили прошивку з параметарми по замовчуванню (зі сторінки релізів, чи зібрали самостійно, не змінюючи конфігурацію збірки) через [ESPHome Web Installer](https://web.esphome.io/) або іншим способом, то пристрій перейде в режим очікування налаштування, та буде швидко блимати світлодіодом увесь час, поки його не налаштують за допомогою Андроід застосунку (скоро буде), чи Svitlo Pulse Веб Інсталятор (скоро буде)
1. Після налаштування (або якщо налаштування були вказані при збірці), через 1 хвилину пристрій відправить перший пінг, і після цього увімкнеться синій світлодіод, який буде горіти постійно.
1. Якщо синій світлодіод не горить, це значить, що пристрою не вдалося відправити пінг на сервер Світлобот. Можливо, проблеми з доступом в Інтернет.

## Інструкція зі збірки ПЗ

1. Встановіть середовище розробки VSCode.
1. Встановіть розширення Platformio.
1. Склонуйте цей репозиторій.
1. Відкрийте його у VSCode.
1. За потреби, змініть конфігурацію ПЗ для збірки, див. нижче.
1. Використовуйте вбудовані команди Platformio, доступні в VSCode, щоб зібрати та завантажити ПЗ у пристрій.

### Конфігурація збірки

З різних причин вам може знадобитися змінити налаштування ПЗ по замовчуванню, наприклад, одразу при збірці задати назву і пароль WiFi мережі чи ключа Світлобота.

Будь ласка, прегляньте приклад налаштувань і документацію до них у файлі ``./env.example``.
Також, ви можете змінити ті самі налаштування у файлі ``./src/defconfig.hpp``.
