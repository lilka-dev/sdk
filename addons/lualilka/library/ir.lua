---@meta

---ІЧ-передавач і приймач (модуль 002 IR: KY-005 + KY-022) на периферії RMT.
---
---Протоколи: NEC (8- і 16-бітна адреса), Samsung, Sony SIRC (12/15/20 біт). Невідомі протоколи можна приймати
---і відтворювати як сирі тривалості.
---@class ir
ir = {}

---@class ir.Options
---@field tx? integer|false GPIO передавача (за замовчуванням 43), ``false`` - без передавача
---@field rx? integer|false GPIO приймача (за замовчуванням 44), ``false`` - без приймача
---@field carrier? integer частота несучої, Гц (за замовчуванням 38000)

---@class ir.Code
---@field protocol string ``"NEC"``, ``"SAMSUNG"``, ``"SONY"`` або ``"UNKNOWN"``
---@field address? integer
---@field command? integer
---@field ["repeat"]? boolean кадр повторення NEC (кнопку утримують)
---@field bits? integer
---@field raw integer[] тривалості, мкс: імпульс, пауза, імпульс, ...

---@class ir.Transceiver
local Transceiver = {}

---@param options? ir.Options
---@return ir.Transceiver|nil transceiver
---@return string? error
---@usage
--- local remote = ir.new()
--- remote:send("NEC", 0x04, 0x08)
function ir.new(options) end

---Кодує й надсилає кадр.
---@param protocol string ``"NEC"``, ``"SAMSUNG"`` або ``"SONY"``
---@param address integer
---@param command integer
---@param repeats? integer скільки разів повторити (за замовчуванням 0, для SONY - 2)
---@return boolean
function Transceiver:send(protocol, address, command, repeats) end

---Надсилає сирі тривалості (до 380), наприклад ``raw`` з ``receive()``.
---@param durations integer[] мкс, починаючи з імпульсу
---@param carrier? integer Гц
---@return boolean
function Transceiver:send_raw(durations, carrier) end

---Повертає прийнятий кадр або ``nil`` (без очікування).
---@return ir.Code|nil
function Transceiver:receive() end

---Звільняє RMT і контакти.
function Transceiver:close() end

return ir
