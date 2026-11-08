---@meta

---Шина CAN на вбудованому контролері TWAI (модуль 008 CAN, трансивер SN65HVD232).
---@class can
can = {}

---@class can.Options
---@field bitrate? integer 25000, 50000, 100000, 125000, 250000, 500000 (за замовчуванням), 800000 або 1000000
---@field mode? string ``"normal"`` (за замовчуванням), ``"listen"`` (лише прийом, без ACK) або ``"no_ack"`` (самотест)
---@field tx? integer GPIO TX (за замовчуванням 43)
---@field rx? integer GPIO RX (за замовчуванням 44)
---@field filter? can.Filter

---@class can.Filter
---@field code? integer acceptance code (за замовчуванням 0)
---@field mask? integer acceptance mask, 1 = біт не перевіряється (за замовчуванням 0xFFFFFFFF - приймати все)
---@field single? boolean один 32-бітний фільтр (за замовчуванням true)

---@class can.SendOptions
---@field extended? boolean 29-бітний ідентифікатор (за замовчуванням - якщо id > 0x7FF)
---@field rtr? boolean remote frame; тоді ``data`` - запитувана довжина (число)
---@field timeout? integer мс очікування місця в черзі (за замовчуванням 100)

---@class can.Message
---@field id integer
---@field data string до 8 байт
---@field dlc integer
---@field extended boolean
---@field rtr boolean

---@class can.Bus
local Bus = {}

---Запускає контролер CAN.
---@param options? can.Options
---@return can.Bus|nil bus
---@return string? error
---@usage
--- local bus = can.new({bitrate = 500000})
--- bus:send(0x123, "\xAB\xCD")
function can.new(options) end

---Ставить кадр у чергу на передачу.
---@param id integer
---@param data? string|integer[] до 8 байт
---@param options? can.SendOptions
---@return boolean ok
---@return string? error
function Bus:send(id, data, options) end

---Повертає прийнятий кадр або ``nil``.
---@param timeout_ms? integer скільки чекати (за замовчуванням 0)
---@return can.Message|nil
function Bus:receive(timeout_ms) end

---Стан контролера: ``state`` (``"stopped"``, ``"running"``, ``"bus_off"``, ``"recovering"``), лічильники помилок і черг.
---@return table|nil
function Bus:status() end

---Виходить зі стану BUS_OFF або перезапускає зупинений контролер.
---@return boolean
function Bus:recover() end

---Зупиняє контролер і звільняє контакти.
function Bus:close() end

return can
