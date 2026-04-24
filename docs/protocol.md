# TextStalker Protocol v1

## Transport

Серверный протокол работает поверх TCP и использует length-prefixed protobuf frames:

1. `uint32` в big-endian с длиной protobuf-сообщения.
2. Сериализованный payload из `shared/src/protocol/message.proto`.

На чтение и запись сервер использует `s2d::protocol::ClientMessage` и `s2d::protocol::ServerMessage`.

## Message Schema

Актуальный контракт описан в `shared/src/protocol/message.proto`.

### ClientMessage

- `request_id`: `uint64` для корреляции request/response.
- `ping`: запрос ping/pong с полем `timestamp`.
- `state_snapshot`: запрос снимка текущего состояния.

### ServerMessage

- `request_id`: копируется из клиентского запроса.
- `status`: `STATUS_OK` или `STATUS_ERROR`.
- `pong`: ответ на `ping`.
- `state_snapshot`: снимок состояния сервера в JSON-строке.
- `error`: код и текст ошибки.

## Supported Requests

### Ping

Клиент отправляет:

```proto
ClientMessage {
  request_id: 1
  ping { timestamp: 1710000000 }
}
```

Сервер отвечает:

```proto
ServerMessage {
  request_id: 1
  status: STATUS_OK
  pong { timestamp: 1710000000 }
}
```

### State Snapshot

Клиент отправляет:

```proto
ClientMessage {
  request_id: 2
  state_snapshot {}
}
```

Сервер отвечает:

```proto
ServerMessage {
  request_id: 2
  status: STATUS_OK
  state_snapshot { state_json: "{\"world\":\"bootstrap\",\"players\":[]}" }
}
```

## Error Response

Если payload не задан или тип сообщения не поддерживается, сервер отвечает `ServerMessage` со статусом `STATUS_ERROR` и заполненным `error`.
