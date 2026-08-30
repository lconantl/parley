Для генерации PDF-презентаций используется фасад `PdfGenerator`, который связывает тему оформления со структурой слайдов.

**Подготовка темы оформления**

* Тема задает палитру, метрики (отступы, сетку), пути к шрифтам и графическим ресурсам.


* Создание темы выполняется через фабричную функцию, требующую путь к корневой папке ассетов, например `CreateStrategyPartnersTheme("path/to/assets")`.


* Папка ассетов должна содержать подкаталоги `fonts` (например, `Verdana.ttf`) и `images` (`logo.png` и декорации).



**Сборка презентации (Deck)**

* Структура документа описывается объектом `Deck`, который содержит поля `title`, `author` и массив слайдов `std::vector<Slide>`.


* Контент каждого слайда задается через `std::variant`, поддерживающий несколько типов компоновки.


* Доступные макеты: `TitleSlideContent` (титульный), `CardGridSlideContent` (сетка карточек), `ColumnStripSlideContent` (вертикальные колонки), `BulletSlideContent` (список с опциональной иллюстрацией) и `ClosingSlideContent` (финальный слайд).



**Генерация файла**

* Запуск процесса сборки осуществляется вызовом `PdfGenerator::Generate(deck, theme, outputPath)`.


* В процессе генерации система сама рассчитает перенос слов, отрендерит команды через `PdfRenderer` и сохранит результат по указанному пути.



**Пример использования**

```cpp
#include "PdfGenerator.hpp"
#include "theme/StrategyPartnersTheme.hpp"

// 1. Инициализация темы
Theme theme = CreateStrategyPartnersTheme("./assets"); //[cite: 4]

// 2. Создание макета слайда
CardGridSlideContent content;
content.title = "Направления работы"; //[cite: 4]
content.columnCount = 3; //[cite: 4]
content.cards = { {"Стратегия", "Описание блока"} }; //[cite: 4]

// 3. Формирование презентации
Deck deck;
deck.title = "Обучающая презентация"; //[cite: 4]
deck.slides.push_back(Slide{content}); //[cite: 4]

// 4. Сохранение PDF
PdfGenerator::Generate(deck, theme, "output.pdf"); //[cite: 4]

```