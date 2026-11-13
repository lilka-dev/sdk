---@meta

---LoRa-радіо SX1262 / SX1268 (модуль 003 LORA, Ebyte E22-900M / E22-400M) на шині SPI2.
---
---Перевіряйте, які частоти й потужності дозволені у вашій країні. Не передавайте без антени.
---@class lora
lora = {}

---@class lora.Options
---@field chip? string ``"sx1262"`` (E22-900M, 850-930 МГц, за замовчуванням) або ``"sx1268"`` (E22-400M, 410-493 МГц)
---@field frequency? integer Гц (за замовчуванням 868100000 або 433175000 для sx1268)
---@field sf? integer коефіцієнт розширення 5-12 (за замовчуванням 7)
---@field bandwidth? integer Гц: 7800, 10400, 15600, 20800, 31250, 41700, 62500, 125000 (за замовчуванням), 250000, 500000
---@field cr? integer coding rate 5-8 = 4/5...4/8 (за замовчуванням 5)
---@field power? integer дБм, -9...22 (за замовчуванням 0)
---@field sync_word? integer 0x12 - приватна мережа (за замовчуванням), 0x34 - LoRaWAN
---@field preamble? integer символів (за замовчуванням 8)
---@field crc? boolean (за замовчуванням true)
---@field invert_iq? boolean (за замовчуванням false)

---@class lora.Packet
---@field data string
---@field rssi number дБм
---@field snr number дБ
---@field crc_ok boolean

---@class lora.Radio
local Radio = {}

---@param options? lora.Options
---@return lora.Radio|nil radio
---@return string? error
---@usage
--- local radio = assert(lora.new({frequency = 868100000, sf = 9}))
--- radio:send("hello")
function lora.new(options) end

---Надсилає пакет (до 255 байт) і чекає завершення. Потім повертається в прийом, якщо він був увімкнений.
---@param data string|integer[]
---@param timeout_ms? integer (за замовчуванням 3000)
---@return boolean ok
---@return integer|string time_on_air_ms_or_error
function Radio:send(data, timeout_ms) end

---Вмикає безперервний прийом (``receive()`` теж вмикає його автоматично).
function Radio:listen() end

---Повертає прийнятий пакет або ``nil`` (без очікування).
---@return lora.Packet|nil
function Radio:receive() end

---Поточний RSSI каналу, дБм.
---@return number
function Radio:rssi() end

---@param hz integer
---@return boolean
function Radio:set_frequency(hz) end

---@param dbm integer -9...22
---@return boolean
function Radio:set_power(dbm) end

---Розрахунковий час у ефірі для пакета заданої довжини.
---@param length integer
---@return integer ms
function Radio:time_on_air(length) end

function Radio:standby() end

---Сон із збереженням налаштувань; будь-яка наступна дія будить радіо.
function Radio:sleep() end

---Присипляє радіо і звільняє SPI та контакти.
function Radio:close() end

return lora
