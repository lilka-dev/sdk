---@meta

---Приймач GNSS (модуль 005 GPS, u-blox NEO-M8N) на UART2.
---
---Об'єкт тримає контакти зайнятими, доки його не закрито (``close()``) або доки скрипт не завершився.
---@class gps
gps = {}

---@class gps.Options
---@field baud? integer швидкість UART (за замовчуванням 9600)
---@field rx? integer GPIO входу (TX модуля), за замовчуванням 44
---@field tx? integer|false GPIO виходу (RX модуля), за замовчуванням 43; ``false`` - лише прийом

---@class gps.Fix
---@field valid boolean є фіксація (RMC status = A)
---@field latitude number градуси, південь < 0
---@field longitude number градуси, захід < 0
---@field altitude number метри над рівнем моря
---@field speed number км/год
---@field course number градуси від півночі
---@field satellites integer супутники, що використовуються
---@field quality integer 0 - немає, 1 - GPS, 2 - DGPS, 4/5 - RTK, 6 - оцінка
---@field fix_type integer 1 - немає, 2 - 2D, 3 - 3D
---@field hdop number
---@field vdop number
---@field pdop number
---@field year integer UTC
---@field month integer
---@field day integer
---@field hour integer
---@field minute integer
---@field second integer
---@field millis integer
---@field updates integer кількість розібраних GGA/RMC

---@class gps.Satellite
---@field system string ``"gps"``, ``"glonass"``, ``"galileo"``, ``"beidou"`` або ``"other"``
---@field prn integer
---@field elevation integer градуси
---@field azimuth integer градуси
---@field snr? integer дБГц, відсутній, якщо супутник не відстежується

---@class gps.Receiver
local Receiver = {}

---Запускає UART і повертає об'єкт приймача.
---@param options? gps.Options
---@return gps.Receiver|nil receiver
---@return string? error
---@usage
--- local g = gps.new()
function gps.new(options) end

---Читає все, що надійшло з модуля, і розбирає NMEA. Викликайте часто (наприклад, у ``lilka.update``).
---@return integer sentences кількість нових речень
function Receiver:update() end

---Повертає останні відомі дані про розташування.
---@return gps.Fix
function Receiver:fix() end

---Повертає супутники в полі зору (з повідомлень GSV).
---@return gps.Satellite[]
function Receiver:satellites() end

---Останнє прийняте NMEA-речення (без ``$`` і кінця рядка) - для налагодження.
---@return string
function Receiver:last_sentence() end

---Надсилає сирі байти в модуль (наприклад, команду ``$PUBX``).
---@param data string
---@return boolean
function Receiver:write(data) end

---Надсилає UBX-повідомлення (контрольна сума додається автоматично).
---@param class integer
---@param id integer
---@param payload? string
---@return boolean
function Receiver:send_ubx(class, id, payload) end

---Зупиняє UART і звільняє контакти.
function Receiver:close() end

return gps
