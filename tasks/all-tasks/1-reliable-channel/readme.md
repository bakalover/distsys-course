# Надежный канал

Реализуйте надежный (_reliable_) канал для RPC поверх ненадежного (_fair-loss_).

## RPC

[RPC is Not Dead: Rise, Fall and the Rise of Remote Procedure Calls](http://dist-prog-book.com/chapter/1/rpc.html)

### Реализации

- [Golang rpc package](https://pkg.go.dev/net/rpc)
- [gRPC](https://grpc.io/about/), [Protocol Buffers](https://developers.google.com/protocol-buffers)
- [Cap’n Proto](https://capnproto.org/)

## Канал

Канал – абстракция соединения в RPC. Канал связывает клиента с сервером и позволяет
вызывать на этом сервере методы различных сервисов.

Канал представлен интерфейсом [`IChannel`](rpc/channel.hpp):

```cpp
struct IChannel {
  virtual ~IChannel() = default;

  // Асинхронный вызов метода удаленного сервиса
  virtual await::futures::Future<Message> Call(std::string method,
                                               Message request,
                                               CallOptions options) = 0;
};
```

- `method` – дескриптор вызываемого метода
- `request` – аргументы метода (в сериализованном виде – [`Message`](rpc/message.hpp))
- `options` – `StopToken` для асинхронной отмены запроса

### Синхронный / асинхронный API

Метод `Call` – _асинхронный_: он завершается не дожидаясь ответа от сервера и возвращает клиенту [`Future<T>`](https://gitlab.com/Lipovsky/await/-/blob/master/examples/futures/main.cpp) (далее – просто _фьюча_) – контейнер, в котором появится результат (ошибка или `Message` с ответом).

Клиент может подписаться на появление результата, вызвав у `Future` метод `Subscribe`.

Если клиент хочет выполнить вызов _синхронно_, то он может дождаться полученную
фючу, вызвав метод `GetResult` (если клиент – это поток) или воспользовавшись функцией [`Await`](https://gitlab.com/Lipovsky/await/-/blob/master/await/fibers/core/await.hpp) (если клиент – файбер).

Независимо от способа, фьюча "распакуется" в объект [`Result<T>`](https://gitlab.com/Lipovsky/wheels/-/blob/master/wheels/support/result.hpp), содержащий либо ответ сервиса (в виде `Message`), либо ошибку.

### Стабы

Разработчик в прикладном коде работает с каналами не напрямую, а через _стабы_ (_stub_).

Стаб – это представление удаленного сервиса в виде объекта. Пользователь вызывает методы стаба просто как методы объекта, а стаб уже отправляет запросы через канал.

Стаб для сервиса не требуется писать руками, он может быть сгенерирован автоматически по [декларативному описанию](https://developers.google.com/protocol-buffers/docs/proto3#services). 

## Надежный канал

Вам дан ненадежный канал: в `Call` он посылает в сеть один запрос, и если получил транспортную ошибку (разрыв соединения), то возвращает эту ошибку клиенту. 

Ваша задача – написать для такого канала декоратор – [`ReliableChannel`](rpc/reliable.cpp), 
который будет бесконечно ретраить запрос при транспортных ошибках (см. [`IsRetriableError`](rpc/errors.hpp)).

## Цикл ретраев

Цикл ретраев в канале можно написать двумя способами:

1) Непосредственно с помощью `while`, для этого понадобится отдельный файбер.
2) Можно обойтись без файбера и написать цикл ретраев целиком на коллбэках фьюч.

Попробуйте реализовать оба варианта =)

## Exponential Backoff + Jitter

Ретраить запрос нужно с задержками, для этого используйте класс [`Backoff`](rpc/backoff.hpp) и `ITimerService` рантайма (об этом ниже).

Для упрощения реализации мы не рандомизируем задержки, но в промышленной реализации это необходимо делать:

[AWS Architecture Blog / Exponential Backoff And Jitter](https://aws.amazon.com/blogs/architecture/exponential-backoff-and-jitter/)

## Cancellation

Рассмотрим сценарий: 

Мы получили запрос от клиента, и для ответа на него нам нужно отправить запрос к бэкэнду, 
которых у нас несколько. Подойдет любой, но не каждый может быть жив или способен быстро ответить. 

Поэтому мы отправляем запросы сразу на несколько и комбинируем полученные от каждого вызова
фьючи с помощью [`FirstOf`](https://gitlab.com/Lipovsky/await/-/blob/master/await/futures/combine/first_of.hpp). Полученную фьючу с первым ответом мы дождемся синхронно и после этого ответим клиенту.

Но, возможно, в фоне так и продолжает работать цикл ретраев, который пытается достучаться 
до безнадежного бэкэнда.

В таком случае нам нужно передать каналу, который ретраит вызов, сигнал об отмене асинхронной операции.

Этот сигнал представлен в виде экземпляра [`StopToken`](https://gitlab.com/Lipovsky/await/-/blob/master/await/context/stop_token.hpp). 
Клиент конструирует токен с помощью `StopSource` или `StopScope` и передает
его в метод `Call` канала через `CallOptions`. 

- С помощью `StopSource` можно вручную отправить сигнал отмены, вызвав метод `RequestStop`.
- С помощью `StopScope` можно связать ретраи с лексическим скоупом в тексте программы.

В случае отмены канал должен возвращать пользователю ошибку [`Cancelled()`](rpc/errors.hpp).

## Рантайм

Каналы не работают с потоками исполнения и часами напрямую. 
Время и потоки абстрагированы и представлены в виде сервисов:

- [`IExecutor`](https://gitlab.com/Lipovsky/await/-/blob/master/await/executors/executor.hpp) – исполняет _задачи_ – небольшие фрагменты синхронного кода. В нашем случае задачами будут шаги файберов и коллбэки фьюч.
- [`ITimerService`](https://gitlab.com/Lipovsky/await/-/blob/master/await/time/timer_service.hpp) – заводит таймеры.

Группу необходимых для работы сервисов мы назовем _рантаймом_. 

Каналы получают указатель на реализацию [`IRuntime`](rpc/runtime.hpp) и через него обращаются к конкретным сервисам.

В тестах используются два рантайма:

### `matrix::Matrix`

Детерминированная симуляция в одном потоке с виртуальным временем:
- Файберы и коллбэки фьюч планируются и запускаются с помощью `ManualExecutor`.
- Если в данный момент все файберы заблокированы в ожидании фьюч или мьютекстов, то время проматывается вперед до ближайшего события.

[Реализация](runtime/matrix/matrix.hpp)

### `mt::Runtime`

- Задачи запускаются пулом потоков.
- Таймерами управляет [`TimeKeeper`](https://gitlab.com/Lipovsky/await/-/blob/master/await/time/time_keeper.hpp).

[Реализация](runtime/mt/runtime.hpp)