# Parley bot

Телеграм-бот для инвестиционного анализа российских компаний. Пользователь присылает ИНН — бот собирает данные из
реестров (DaData: ЕГРЮЛ, учредители, руководство, риски; Checko: бухгалтерская отчётность по годам), считает по ним
около сотни финансовых показателей, а те, что в РСБУ не раскрываются — амортизацию, размер рынка, клиентскую базу,
оценку стоимости — закрывает оценкой по формулам с коррекцией AI анализом. У каждого значения хранится происхождение и
уверенность, так что в отчёте видно, где факт из отчётности, где расчёт, а где приближение.

### Документация
1. Формулы всех показателей и специфика РСБУ: [`docs/finance-metrics.md`](docs/finance-metrics.md)
2. Справочник идентификаторов: [`docs/metrics-reference.md`](docs/metrics-reference.md)
3. Устройство PDF-рендера: [`docs/pdf-render.md`](docs/pdf-render.md)

На выходе три документа на выбор: подробный отчёт в Markdown, презентация due diligence в PDF и одностраничный
инвестиционный тизер. Презентацию и тизер можно попросить в обезличенном виде — тогда название, ИНН и имена
собственников заменяются на «Целевую компанию», а конкуренты нумеруются буквами. Доступ ограничен списком
`ALLOWED_USERS`, работает бот на long polling, так что открытых портов не требует. Один анализ занимает от одной до пяти
минут — большую часть времени занимают вызовы LLM.

### Развёртывание

Нужен C++20-компилятор и CMake не ниже 3.28; все библиотеки ставит vcpkg по манифесту, системные `-dev` пакеты не
требуются. На машине с 2 ГБ памяти заранее сделайте swap — сборка boost иначе падает по OOM.

```bash
sudo apt install -y build-essential cmake ninja-build pkg-config git curl \
                    zip unzip tar autoconf automake autoconf-archive libtool python3 locales

git clone https://github.com/microsoft/vcpkg ~/vcpkg && ~/vcpkg/bootstrap-vcpkg.sh -disableMetrics

cd parley
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_TOOLCHAIN_FILE=$HOME/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

Скопируйте `.env.mock` в `.env` и подставьте реальные ключи: токен бота, идентификаторы разрешённых пользователей,
доступы к Polza, Checko и DaData. Файл должен иметь права `600` и не попадать в git. Запускать бинарник нужно **из корня
репозитория** — `assets` со шрифтами и логотипами ищется относительно рабочего каталога, и без неё генерация PDF не
стартует. Локаль должна быть UTF-8 (`LANG=C.UTF-8`), иначе приложение падает на инициализации консоли.

```bash
cp .env.mock .env && chmod 600 .env
cd parley && ./build/parley
```

В продакшене заверните это в systemd-юнит с `WorkingDirectory` на корень репозитория, `EnvironmentFile` на `.env` и
`Restart=always` — тогда бот переживёт и падения, и перезагрузку сервера.

### Использование ядра без бота

Вся логика лежит в статической библиотеке `core`, телеграм — только один из возможных интерфейсов. Собрать анализ и
отрендерить его можно напрямую:

```cpp
#include "api/CheckoApiClient.hpp"
#include "api/DaDataApiClient.hpp"
#include "ai/PolzaClient.hpp"
#include "common/config/Config.hpp"
#include "view/MarkdownCompanyAnalyticsView.hpp"
#include "viewmodel/CompanyAnalyticsViewModel.hpp"

const auto config = Config::LoadFromEnv();

const auto checko = std::make_shared<CheckoApiClient>(config.GetCheckoApiKey());
const auto dadata = std::make_shared<DaDataApiClient>(
    config.GetDaDataApiKey(), config.GetDaDataSecretKey());
const auto polza = std::make_shared<PolzaClient>(
    config.GetPolzaBaseUrl(), config.GetPolzaApiKey(), config.GetPolzaModel());

const CompanyAnalyticsViewModel viewModel(dadata, checko, polza);
const CompanyAnalytics analytics = viewModel.Analyze("7707083893");

MetricFormatOptions options;
options.moneyInMillions = true;

std::cout << MarkdownCompanyAnalyticsView(options).Render(analytics);
std::cout << "Потрачено на оценку: " << polza->GetTotalCost() << " руб." << std::endl;
```

`Analyze` принимает вторым аргументом год, если нужен не последний доступный.
