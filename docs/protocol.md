# TextStalker Protocol v1

## Envelope

Все сообщения передаются в формате JSON и обязаны содержать общий envelope:

```json
{
  "protocol_version": "1.0",
  "message_type": "string",
  "request_id": "string",
  "payload": {}
}
```

Поля envelope:

| Поле | Тип | Обязательное | Описание |
| --- | --- | --- | --- |
| `protocol_version` | `string` | Да | Версия протокола. Для v1: `"1.0"`. |
| `message_type` | `string` | Да | Тип сообщения, например `move.request`, `move.response`, `error`. |
| `request_id` | `string` | Да | Идентификатор запроса для корреляции request/response (например UUID). |
| `payload` | `object` | Да | Данные конкретного сообщения в зависимости от `message_type`. |

## Пример request/response #1: Ход игрока

Request:

```json
{
  "protocol_version": "1.0",
  "message_type": "turn.move.request",
  "request_id": "d26edc1e-6d3b-4d9c-9e67-9fd5f0e21011",
  "payload": {
    "player_id": "p_102",
    "from": { "x": 11, "y": 4 },
    "to": { "x": 12, "y": 4 },
    "ap_cost": 1
  }
}
```

Response:

```json
{
  "protocol_version": "1.0",
  "message_type": "turn.move.response",
  "request_id": "d26edc1e-6d3b-4d9c-9e67-9fd5f0e21011",
  "payload": {
    "status": "ok",
    "remaining_ap": 3,
    "new_position": { "x": 12, "y": 4 }
  }
}
```

## Пример request/response #2: Использование предмета

Request:

```json
{
  "protocol_version": "1.0",
  "message_type": "item.use.request",
  "request_id": "a3a0da3e-0507-4744-94c1-7d9570b6b973",
  "payload": {
    "player_id": "p_102",
    "item_id": "medkit_basic",
    "target_id": "p_102"
  }
}
```

Response:

```json
{
  "protocol_version": "1.0",
  "message_type": "item.use.response",
  "request_id": "a3a0da3e-0507-4744-94c1-7d9570b6b973",
  "payload": {
    "status": "ok",
    "hp_before": 42,
    "hp_after": 67,
    "item_consumed": true
  }
}
```

## Примеры ошибок

Ошибка #1: невалидный ход (клетка недоступна)

```json
{
  "protocol_version": "1.0",
  "message_type": "error",
  "request_id": "d26edc1e-6d3b-4d9c-9e67-9fd5f0e21011",
  "payload": {
    "code": "MOVE_BLOCKED",
    "message": "Target tile is blocked.",
    "details": {
      "to": { "x": 12, "y": 4 }
    }
  }
}
```

Ошибка #2: неизвестный тип сообщения

```json
{
  "protocol_version": "1.0",
  "message_type": "error",
  "request_id": "57f4d133-8b4e-4f19-a536-cf4bd73473e1",
  "payload": {
    "code": "UNKNOWN_MESSAGE_TYPE",
    "message": "Message type is not supported.",
    "details": {
      "received_message_type": "artifact.scan.request"
    }
  }
}
```
