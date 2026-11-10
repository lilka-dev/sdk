#pragma once

#include <stddef.h>
#include <stdint.h>

namespace lilka {

/// Таблиця значень з INI-файлу.
///
/// Зберігає пари ``[секція] ключ = значення`` у хеш-таблиці (ланцюжки, FNV-1a), тому пошук - O(1).
/// Секції та ключі нечутливі до регістру. Без залежностей: лише libc.
///
/// Формат:
///
/// .. code-block:: ini
///
///     ; коментар (також #)
///     [lora]
///     nss = 47        ; коментар після значення
///     name = "a ; b"  ; у лапках ; і # - частина значення
///
/// Ключі до першої секції потрапляють у секцію ``""``. Повторний ключ перезаписує попереднє значення,
/// тому кілька файлів можна завантажити один за одним: наступний перекриває попередній.
///
/// Ітерація (порядок не визначений):
///
/// .. code-block:: cpp
///
///     for (const lilka::IniTable::Entry& e : table) {
///         printf("[%s] %s = %s\n", e.section, e.key, e.value);
///     }
class IniTable {
public:
    struct Entry {
        const char* section;
        const char* key;
        const char* value;
    };

    class iterator {
    public:
        const Entry& operator*() const {
            return current;
        }
        const Entry* operator->() const {
            return &current;
        }
        iterator& operator++();
        bool operator==(const iterator& other) const {
            return node == other.node;
        }
        bool operator!=(const iterator& other) const {
            return node != other.node;
        }

    private:
        friend class IniTable;
        iterator(const IniTable* table, size_t bucket, const void* node);
        void load();

        const IniTable* table;
        size_t bucket;
        const void* node;
        Entry current;
    };
    using const_iterator = iterator;

    IniTable();
    ~IniTable();
    IniTable(const IniTable&) = delete;
    IniTable& operator=(const IniTable&) = delete;

    /// Розібрати INI-текст і додати значення до таблиці. Повертає кількість рядків з помилками
    /// (вони пропускаються, решта завантажується).
    int parse(const char* text, size_t length);
    int parse(const char* text);
    /// Завантажити файл (шлях на кшталт ``/sd/.lilka/config/modules.ini``). Повертає false, якщо файл не відкрився.
    bool load(const char* path, int* errors = nullptr);

    /// Встановити значення (копіюється). Повертає false, якщо не вистачило пам'яті.
    bool set(const char* section, const char* key, const char* value);
    /// Видалити значення. Повертає true, якщо ключ був.
    bool erase(const char* section, const char* key);
    void clear();

    bool has_key(const char* section, const char* key) const;
    iterator find(const char* section, const char* key) const;
    /// Значення або ``fallback``, якщо ключа немає.
    const char* get(const char* section, const char* key, const char* fallback = nullptr) const;
    /// Ціле число (десяткове або ``0x...``); ``fallback``, якщо ключа немає або значення не число.
    long getInt(const char* section, const char* key, long fallback) const;
    float getFloat(const char* section, const char* key, float fallback) const;
    /// ``true/yes/on/1`` або ``false/no/off/0``; інакше ``fallback``.
    bool getBool(const char* section, const char* key, bool fallback) const;

    iterator begin() const;
    iterator end() const;
    size_t size() const {
        return count;
    }
    bool empty() const {
        return count == 0;
    }

private:
    struct Node;
    Node* findNode(const char* section, const char* key, uint32_t hash) const;
    bool grow();

    Node** buckets;
    size_t bucketCount;
    size_t count;
};

} // namespace lilka
