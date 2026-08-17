#pragma once
#include <Manager/Database/Table/JsonPlayerTable.hpp>
#include <Manager/Database/Table/JsonWorldTable.hpp>

class Database {
public:
    bool Connect();

    JsonPlayerTable* GetPlayerTable();
    JsonWorldTable* GetWorldTable();

public:
    Database() = default;
    ~Database() = default;

private:
    JsonPlayerTable* m_pPlayerTable { nullptr };
    JsonWorldTable* m_pWorldTable { nullptr };
};

Database* GetDatabase();
