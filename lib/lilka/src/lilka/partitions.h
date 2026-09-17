#pragma once
#include <Arduino.h>
#include <esp_partition.h>
#include <vector>
#ifndef LILKA_PART_CHUNK_SIZE
#    define LILKA_PART_CHUNK_SIZE 512
#endif
namespace lilka {

typedef bool (*onPartitionChunkClbk)(void* ctx, const String& filename, size_t offset, size_t fSize);

// TODO: Documentation on Partitions

class Partition {
public:
    Partition(const esp_partition_t* partition);
    // Operations:
    bool flash(const String& filename, onPartitionChunkClbk chunkClbk, void* clbkData);
    bool backup(const String& filename, onPartitionChunkClbk chunkClbk, void* clbkData);
    // Accessors:
    esp_flash_t* getFlashChip();
    esp_partition_type_t getType();
    esp_partition_subtype_t getSubtype();
    uint32_t getAddress();
    const char* getLabel();
    bool getEncrypted();

private:
    // flash_chip
    // type
    // subtype
    // address
    // size
    // label
    // encrypted
    const esp_partition_t* partition;
};

class PartitionList {
public:
    PartitionList();

    // Acesses partition by it's index
    Partition* operator[](size_t index);

    // Accesses partition by it's label
    Partition* operator[](const String& rval);

    // Iterator and size
    const std::vector<Partition*>::iterator begin();
    const std::vector<Partition*>::iterator end();
    size_t size();

private:
    std::vector<Partition*> parts;
};

extern PartitionList partitions;
} // namespace lilka
