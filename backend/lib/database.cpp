#include "database.hpp"

DatabaseImpl::DatabaseImpl(const std::string& file)
	: m_database(file, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE)
{
	m_database.exec("create table if not exists tasks (ID integer, Name text)");
}
