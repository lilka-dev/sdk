---@meta

---Акселерометр і гіроскоп BMI260 (модуль 006 GYRO MAGNET) на шині I2C.
---@class imu
imu = {}

---@class imu.Options
---@field sda? integer (за замовчуванням 44)
---@field scl? integer (за замовчуванням 43)
---@field address? integer 0x68 або 0x69 (за замовчуванням - знайти автоматично)
---@field config? boolean|string ``true`` - вбудований конфігураційний блоб Bosch (за замовчуванням), шлях до файлу - завантажити з файлу, ``false`` - без блобу (не перевірено, чи дані тоді коректні)
---@field accel_range? integer g: 2, 4, 8 (за замовчуванням), 16
---@field gyro_range? integer °/с: 125, 250, 500, 1000, 2000 (за замовчуванням)
---@field accel_odr? integer Гц: 12, 25, 50, 100 (за замовчуванням), 200, 400, 800, 1600
---@field gyro_odr? integer Гц: 25, 50, 100, 200 (за замовчуванням), 400, 800, 1600, 3200

---@class imu.Data
---@field ax number g
---@field ay number g
---@field az number g
---@field gx number °/с
---@field gy number °/с
---@field gz number °/с

---@class imu.Sensor
local Sensor = {}

---@param options? imu.Options
---@return imu.Sensor|nil sensor
---@return string? error
---@usage
--- local sensor = assert(imu.new())
--- local d = sensor:read()
function imu.new(options) end

---@return imu.Data|nil
function Sensor:read() end

---@return number °C
function Sensor:temperature() end

---``true``, якщо блоб завантажено і чип його прийняв.
---@return boolean
function Sensor:initialized() end

---Звільняє I2C і контакти.
function Sensor:close() end

return imu
