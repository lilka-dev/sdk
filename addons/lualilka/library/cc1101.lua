---@meta

---Субгігагерцове радіо CC1101 (модуль 004 RF з CC1101) на шині SPI2.
---
---Діапазони 300-348, 387-464 і 779-928 МГц. Пакети змінної довжини до 61 байта.
---Перевіряйте, які частоти дозволені у вашій країні.
---@class cc1101
cc1101 = {}

---@class cc1101.Options
---@field frequency? number МГц (за замовчуванням 433.92)
---@field modulation? string ``"gfsk"`` (за замовчуванням), ``"2fsk"``, ``"ask"`` (OOK), ``"4fsk"`` або ``"msk"``
---@field rate? integer бод, 600-500000 (за замовчуванням 38400)
---@field deviation? integer Гц (за замовчуванням 20000)
---@field bandwidth? integer Гц, смуга приймача (за замовчуванням 100000)
---@field power? integer дБм, -30...10 (за замовчуванням 0)
---@field sync_word? integer (за замовчуванням 0xD391)
---@field crc? boolean (за замовчуванням true)

---@class cc1101.Packet
---@field data string
---@field rssi number дБм
---@field lqi integer
---@field crc_ok boolean

---@class cc1101.Radio
local Radio = {}

---@param options? cc1101.Options
---@return cc1101.Radio|nil radio
---@return string? error
---@usage
--- local radio = assert(cc1101.new({frequency = 433.92}))
--- radio:send("hello")
function cc1101.new(options) end

---Надсилає пакет до 61 байта.
---@param data string|integer[]
---@param timeout_ms? integer (за замовчуванням 500)
---@return boolean
function Radio:send(data, timeout_ms) end

---Вмикає прийом (``receive()`` теж вмикає його автоматично).
function Radio:listen() end

---Повертає прийнятий пакет або ``nil`` (без очікування).
---@return cc1101.Packet|nil
function Radio:receive() end

---Поточний RSSI, дБм.
---@return number
function Radio:rssi() end

---@param mhz number
---@return boolean
function Radio:set_frequency(mhz) end

---@param dbm integer
function Radio:set_power(dbm) end

function Radio:idle() end

---Звільняє SPI і контакти.
function Radio:close() end

return cc1101
