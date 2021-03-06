#include "DBFactory.h"
#include "FileSystem.h"
#include "LevelDB.h"
#include "RocksDB.h"
#include "MemoryDB.h"
#include "libbrccore/Exceptions.h"

namespace dev
{
namespace db
{
namespace fs = boost::filesystem;
namespace po = boost::program_options;

auto g_kind = DatabaseKind::LevelDB;
fs::path g_dbPath;

/// A helper type to build the table of DB implementations.
///
/// More readable than std::tuple.
/// Fields are not initialized to allow usage of construction with initializer lists {}.
struct DBKindTableEntry
{
    DatabaseKind kind;
    char const* name;
};

/// The table of available DB implementations.
///
/// We don't use a map to avoid complex dynamic initialization. This list will never be long,
/// so linear search only to parse command line arguments is not a problem.
DBKindTableEntry dbKindsTable[] = {
    {DatabaseKind::LevelDB, "leveldb"},
    {DatabaseKind::RocksDB, "rocksdb"},
    {DatabaseKind::MemoryDB, "memorydb"},
};

void setDatabaseKindByName(std::string const& _name)
{
    for (auto& entry : dbKindsTable)
    {
        if (_name == entry.name)
        {
            g_kind = entry.kind;
            return;
        }
    }

    BOOST_THROW_EXCEPTION(
        brc::InvalidDatabaseKind() << errinfo_comment("invalid database name supplied: " + _name));
}

void setDatabaseKind(DatabaseKind _kind)
{
    g_kind = _kind;
}

void setDatabasePath(std::string const& _path)
{
    g_dbPath = fs::path(_path);
}

bool isDiskDatabase()
{
    switch (g_kind)
    {
        case DatabaseKind::LevelDB:
        case DatabaseKind::RocksDB:
            return true;
        default:
            return false;
    }
}
std::vector<DBVersion> getDBVersion()
{
    std::vector<DBVersion> versions;
    for (auto& entry : dbKindsTable)
    {
        if(entry.kind == DatabaseKind::LevelDB){
            DBVersion version;
            version.db_name = entry.name;
            version.db_majorVersion = leveldb::kMajorVersion;
            version.db_minorVersion = leveldb::kMinorVersion;
            versions.emplace_back(version);
            continue;
        }
        if(entry.kind == DatabaseKind::RocksDB){
            DBVersion version;
            version.db_name = entry.name;
            version.db_majorVersion = rocksdb::kMajorVersion;
            version.db_minorVersion = rocksdb::kMinorVersion;
            versions.emplace_back(version);
            continue;
        }
    }
    return versions;
}
DatabaseKind databaseKind()
{
    return g_kind;
}

fs::path databasePath()
{
    return g_dbPath.empty() ? getDataDir() : g_dbPath;
}

po::options_description databaseProgramOptions(unsigned _lineLength)
{
    // It must be a static object because boost expects const char*.
    static std::string const description = [] {
        std::string names;
        for (auto const& entry : dbKindsTable)
        {
            if (!names.empty())
                names += ", ";
            names += entry.name;
        }

        return "Select database implementation. Available options are: " + names + ".";
    }();

    po::options_description opts("DATABASE OPTIONS", _lineLength);
    auto add = opts.add_options();

    add("db",
        po::value<std::string>()->value_name("<name>")->default_value("leveldb")->notifier(
            setDatabaseKindByName),
        description.data());

    add("db-path",
        po::value<std::string>()
            ->value_name("<path>")
            ->default_value(getDataDir().string())
            ->notifier(setDatabasePath),
        "Database path (for non-memory database options)\n");

    return opts;
}

std::unique_ptr<DatabaseFace> DBFactory::create()
{
    return create(databasePath());
}

std::unique_ptr<DatabaseFace> DBFactory::create(fs::path const& _path)
{
    return create(g_kind, _path);
}

std::unique_ptr<DatabaseFace> DBFactory::create(DatabaseKind _kind)
{
    return create(_kind, databasePath());
}

std::unique_ptr<DatabaseFace> DBFactory::create(DatabaseKind _kind, fs::path const& _path)
{
    switch (_kind)
    {
    case DatabaseKind::LevelDB:
        return std::unique_ptr<DatabaseFace>(new LevelDB(_path));
        break;
    case DatabaseKind::RocksDB:
        return std::unique_ptr<DatabaseFace>(new RocksDB(_path));
        break;
    case DatabaseKind::MemoryDB:
        // Silently ignore path since the concept of a db path doesn't make sense
        // when using an in-memory database
        return std::unique_ptr<DatabaseFace>(new MemoryDB());
        break;
    default:
        assert(false);
        return {};
    }
}

DatabaseKind stringtToKind(std::string _dbName)
{
    if(_dbName == "leveldb")
    {
        return DatabaseKind::LevelDB;
    }else if(_dbName == "rocksdb")
    {
        return DatabaseKind::RocksDB;
    }else if(_dbName == "memorydb")
    {
        return DatabaseKind::MemoryDB;
    }
    return DatabaseKind::Null;
}


}  // namespace db
}  // namespace dev