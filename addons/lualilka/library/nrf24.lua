---@meta

---Радіо 2.4 ГГц nRF24L01+ (модуль 004 RF) на шині SPI2.
---@class nrf24
nrf24 = {}

---@class nrf24.Options
---@field channel? integer 0-125, частота = 2400 + channel МГц (за замовчуванням 76)
---@field rate? integer 250000, 1000000 (за замовчуванням) або 2000000
---@field power? integer 0 (-18 дБм) ... 3 (0 дБм, за замовчуванням)
---@field payload? integer 1-32 (за замовчуванням 32), 0 - динамічна довжина
---@field auto_ack? boolean (за замовчуванням true)
---@field retries? integer 0-15 (за замовчуванням 15)
---@field retry_delay? integer мкс, 250-4000 (за замовчуванням 1500)
---@field address_width? integer 3-5 (за замовчуванням 5)
---@field crc? integer 0, 1 або 2 байти (за замовчуванням 2)

---@class nrf24.Radio
local Radio = {}

---@param options? nrf24.Options
---@return nrf24.Radio|nil radio
---@return string? error
---@usage
--- local radio = assert(nrf24.new({channel = 90}))
--- radio:open_tx("1Node")
--- radio:send("ping")
function nrf24.new(options) end

---Адреса передачі: рядок довжиною ``address_width`` або число.
---@param address string|integer
function Radio:open_tx(address) end

---Відкриває канал прийому 0-5. Для каналів 2-5 використовується лише перший байт адреси.
---@param pipe integer
---@param address string|integer
function Radio:open_rx(pipe, address) end

---@param pipe integer
function Radio:close_rx(pipe) end

---Вмикає прийом.
function Radio:listen() end

function Radio:stop_listening() end

---Надсилає пакет до 32 байт. З ``auto_ack`` повертає ``true`` лише після підтвердження.
---@param data string|integer[]
---@param timeout_ms? integer
---@return boolean
function Radio:send(data, timeout_ms) end

---Номер каналу, у якому є пакет, або ``nil``.
---@return integer|nil
function Radio:available() end

---Читає пакет: дані і номер каналу, або ``nil``.
---@return string|nil data
---@return integer? pipe
function Radio:read() end

---@param channel integer 0-125
function Radio:set_channel(channel) end

---``true``, якщо на поточному каналі є сигнал, сильніший за -64 дБм (у режимі прийому).
---@return boolean
function Radio:carrier() end

---Сканує всі 126 каналів і повертає кількість "влучань" на кожному (індекс 1 = канал 0).
---@param passes? integer 1-100 (за замовчуванням 1)
---@param dwell_us? integer 130-10000 (за замовчуванням 260)
---@return integer[]
function Radio:scan(passes, dwell_us) end

---Вимикає радіо і звільняє SPI та контакти.
function Radio:close() end

return nrf24
