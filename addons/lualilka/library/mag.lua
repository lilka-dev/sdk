---@meta

---3D-датчик магнітного поля TMAG5273 (модуль 006 GYRO MAGNET) на шині I2C.
---
---Поруч із платою Лілки поле спотворене, тому перед використанням як компаса потрібне калібрування.
---@class mag
mag = {}

---@class mag.Options
---@field sda? integer (за замовчуванням 44)
---@field scl? integer (за замовчуванням 43)
---@field address? integer (за замовчуванням 0x35)
---@field range? string ``"low"`` (±40 мТл для A1, за замовчуванням) або ``"high"`` (±80 мТл)
---@field averaging? integer 0-5: 1x ... 32x (за замовчуванням 5)

---@class mag.Data
---@field x number мкТл
---@field y number мкТл
---@field z number мкТл

---@class mag.Sensor
local Sensor = {}

---@param options? mag.Options
---@return mag.Sensor|nil sensor
---@return string? error
function mag.new(options) end

---Поле з урахуванням зміщення.
---@return mag.Data|nil
function Sensor:read() end

---Напрямок у площині X/Y, 0-360° (тримайте Лілку горизонтально).
---@return number|nil
function Sensor:heading() end

---@return number °C
function Sensor:temperature() end

---Задає зміщення (hard-iron), яке віднімається від кожного вимірювання.
---@param x number мкТл
---@param y number мкТл
---@param z number мкТл
function Sensor:set_offset(x, y, z) end

---@return number x
---@return number y
---@return number z
function Sensor:get_offset() end

---Починає калібрування: обертайте Лілку в усіх напрямках, викликаючи ``read()``; зміщення оновлюється автоматично.
function Sensor:start_calibration() end

---Завершує калібрування і повертає отримане зміщення.
---@return number x
---@return number y
---@return number z
function Sensor:stop_calibration() end

---Звільняє I2C і контакти.
function Sensor:close() end

return mag
