#include "ini.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

namespace lilka {

// One allocation per entry: the node header followed by "section\0key\0value\0".
struct IniTable::Node {
    Node* next;
    uint32_t hash;
    const char* key; // points into data
    const char* value; // points into data
    char section[1]; // data starts here
};

static const size_t INITIAL_BUCKETS = 16;
static const size_t MAX_LINE = 256;

static uint32_t hashPart(uint32_t h, const char* s) {
    for (; *s; s++) {
        h ^= (uint8_t)tolower((unsigned char)*s);
        h *= 16777619u;
    }
    return h;
}

static uint32_t hashKey(const char* section, const char* key) {
    uint32_t h = hashPart(2166136261u, section);
    h ^= 0xFF; // separator: "a" + "bc" != "ab" + "c"
    h *= 16777619u;
    return hashPart(h, key);
}

// ---- iterator ----

IniTable::iterator::iterator(const IniTable* t, size_t b, const void* n) : table(t), bucket(b), node(n), current() {
    load();
}

void IniTable::iterator::load() {
    if (node) {
        const Node* n = static_cast<const Node*>(node);
        current = {n->section, n->key, n->value};
    }
}

IniTable::iterator& IniTable::iterator::operator++() {
    const Node* n = static_cast<const Node*>(node);
    if (n && n->next) {
        node = n->next;
    } else {
        node = nullptr;
        while (++bucket < table->bucketCount) {
            if (table->buckets[bucket]) {
                node = table->buckets[bucket];
                break;
            }
        }
    }
    load();
    return *this;
}

// ---- table ----

IniTable::IniTable() : buckets(nullptr), bucketCount(0), count(0) {
}

IniTable::~IniTable() {
    clear();
    free(buckets);
}

void IniTable::clear() {
    for (size_t i = 0; i < bucketCount; i++) {
        Node* n = buckets[i];
        while (n) {
            Node* next = n->next;
            free(n);
            n = next;
        }
        buckets[i] = nullptr;
    }
    count = 0;
}

IniTable::Node* IniTable::findNode(const char* section, const char* key, uint32_t hash) const {
    if (!bucketCount) return nullptr;
    for (Node* n = buckets[hash % bucketCount]; n; n = n->next) {
        if (n->hash == hash && !strcasecmp(n->section, section) && !strcasecmp(n->key, key)) return n;
    }
    return nullptr;
}

bool IniTable::grow() {
    size_t newCount = bucketCount ? bucketCount * 2 : INITIAL_BUCKETS;
    Node** fresh = static_cast<Node**>(calloc(newCount, sizeof(Node*)));
    if (!fresh) return false;
    for (size_t i = 0; i < bucketCount; i++) {
        Node* n = buckets[i];
        while (n) {
            Node* next = n->next;
            size_t b = n->hash % newCount;
            n->next = fresh[b];
            fresh[b] = n;
            n = next;
        }
    }
    free(buckets);
    buckets = fresh;
    bucketCount = newCount;
    return true;
}

bool IniTable::set(const char* section, const char* key, const char* value) {
    if (!section) section = "";
    if (!key || !value) return false;
    if ((count + 1) * 4 > bucketCount * 3 && !grow() && !bucketCount) return false;
    uint32_t hash = hashKey(section, key);
    size_t ls = strlen(section) + 1, lk = strlen(key) + 1, lv = strlen(value) + 1;
    Node* n = static_cast<Node*>(malloc(sizeof(Node) + ls + lk + lv));
    if (!n) return false;
    n->next = nullptr;
    char* data = n->section;
    memcpy(data, section, ls);
    memcpy(data + ls, key, lk);
    memcpy(data + ls + lk, value, lv);
    n->key = data + ls;
    n->value = data + ls + lk;
    n->hash = hash;

    // Replace an existing entry in place in its chain.
    Node** link = &buckets[hash % bucketCount];
    while (*link) {
        Node* old = *link;
        if (old->hash == hash && !strcasecmp(old->section, section) && !strcasecmp(old->key, key)) {
            n->next = old->next;
            *link = n;
            free(old);
            return true;
        }
        link = &old->next;
    }
    n->next = buckets[hash % bucketCount];
    buckets[hash % bucketCount] = n;
    count++;
    return true;
}

bool IniTable::erase(const char* section, const char* key) {
    if (!bucketCount) return false;
    if (!section) section = "";
    uint32_t hash = hashKey(section, key);
    Node** link = &buckets[hash % bucketCount];
    while (*link) {
        Node* n = *link;
        if (n->hash == hash && !strcasecmp(n->section, section) && !strcasecmp(n->key, key)) {
            *link = n->next;
            free(n);
            count--;
            return true;
        }
        link = &n->next;
    }
    return false;
}

bool IniTable::has_key(const char* section, const char* key) const {
    if (!section) section = "";
    return findNode(section, key, hashKey(section, key)) != nullptr;
}

IniTable::iterator IniTable::find(const char* section, const char* key) const {
    if (!section) section = "";
    uint32_t hash = hashKey(section, key);
    const Node* n = findNode(section, key, hash);
    return n ? iterator(this, hash % bucketCount, n) : end();
}

const char* IniTable::get(const char* section, const char* key, const char* fallback) const {
    if (!section) section = "";
    const Node* n = findNode(section, key, hashKey(section, key));
    return n ? n->value : fallback;
}

long IniTable::getInt(const char* section, const char* key, long fallback) const {
    const char* v = get(section, key);
    if (!v || !*v) return fallback;
    char* tail;
    long result = strtol(v, &tail, 0);
    return *tail ? fallback : result;
}

float IniTable::getFloat(const char* section, const char* key, float fallback) const {
    const char* v = get(section, key);
    if (!v || !*v) return fallback;
    char* tail;
    float result = strtof(v, &tail);
    return *tail ? fallback : result;
}

bool IniTable::getBool(const char* section, const char* key, bool fallback) const {
    const char* v = get(section, key);
    if (!v) return fallback;
    if (!strcasecmp(v, "true") || !strcasecmp(v, "yes") || !strcasecmp(v, "on") || !strcmp(v, "1")) return true;
    if (!strcasecmp(v, "false") || !strcasecmp(v, "no") || !strcasecmp(v, "off") || !strcmp(v, "0")) return false;
    return fallback;
}

IniTable::iterator IniTable::begin() const {
    for (size_t b = 0; b < bucketCount; b++) {
        if (buckets[b]) return iterator(this, b, buckets[b]);
    }
    return end();
}

IniTable::iterator IniTable::end() const {
    return iterator(this, bucketCount, nullptr);
}

// ---- parser ----

static char* trim(char* s) {
    while (isspace((unsigned char)*s))
        s++;
    char* e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

// Parses one line in place. Returns false on a syntax error.
static bool parseLine(IniTable& table, char* line, char* section, size_t sectionSize) {
    char* s = trim(line);
    if (!*s || *s == ';' || *s == '#') return true;
    if (*s == '[') {
        char* close = strchr(s, ']');
        if (!close) return false;
        const char* rest = trim(close + 1);
        if (*rest && *rest != ';' && *rest != '#') return false;
        *close = 0;
        const char* name = trim(s + 1);
        if (strlen(name) >= sectionSize) return false;
        strcpy(section, name);
        return true;
    }
    char* eq = strchr(s, '=');
    if (!eq) return false;
    *eq = 0;
    const char* key = trim(s);
    if (!*key) return false;
    char* value = trim(eq + 1);
    if (*value == '"') {
        char* close = strchr(value + 1, '"');
        if (!close) return false;
        const char* rest = trim(close + 1);
        if (*rest && *rest != ';' && *rest != '#') return false;
        *close = 0;
        value++;
    } else {
        // Inline comment: ';' or '#' at the start or after whitespace.
        for (char* p = value; *p; p++) {
            if ((*p == ';' || *p == '#') && (p == value || isspace((unsigned char)p[-1]))) {
                *p = 0;
                break;
            }
        }
        value = trim(value);
    }
    return table.set(section, key, value);
}

int IniTable::parse(const char* text, size_t length) {
    char section[64] = "";
    char line[MAX_LINE];
    int errors = 0;
    size_t pos = 0;
    while (pos < length) {
        size_t n = 0;
        bool tooLong = false;
        while (pos < length && text[pos] != '\n') {
            if (n < MAX_LINE - 1) {
                line[n++] = text[pos];
            } else {
                tooLong = true;
            }
            pos++;
        }
        pos++; // skip '\n'
        line[n] = 0;
        if (n && line[n - 1] == '\r') line[n - 1] = 0;
        if (tooLong || !parseLine(*this, line, section, sizeof(section))) errors++;
    }
    return errors;
}

int IniTable::parse(const char* text) {
    return parse(text, strlen(text));
}

bool IniTable::load(const char* path, int* errors) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;
    fseek(f, 0, SEEK_END);
    long fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* text = fileSize > 0 ? static_cast<char*>(malloc(fileSize)) : nullptr;
    bool ok = fileSize == 0 || (text && fread(text, 1, fileSize, f) == (size_t)fileSize);
    fclose(f);
    if (ok && fileSize > 0) {
        int e = parse(text, fileSize);
        if (errors) *errors = e;
    }
    free(text);
    return ok;
}

} // namespace lilka
