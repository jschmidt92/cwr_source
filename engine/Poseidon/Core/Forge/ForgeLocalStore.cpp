#include <Poseidon/Core/Forge/ForgeLocalStore.hpp>

#include <Poseidon/IO/Filesystem/Utf8Paths.hpp>

#include <cctype>
#include <chrono>
#include <filesystem>
#include <system_error>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Poseidon::Forge
{
namespace fs = std::filesystem;

namespace
{
fs::path FsPathFromUtf8(const std::string& path)
{
#ifdef _WIN32
    return fs::path(Utf8PathToWide(path.c_str()));
#else
    return fs::path(path);
#endif
}

std::string FsPathToUtf8(const fs::path& path)
{
#ifdef _WIN32
    return WidePathToUtf8(path.wstring().c_str());
#else
    return path.string();
#endif
}

bool ReplaceFileAtomically(const fs::path& source, const fs::path& destination)
{
#ifdef _WIN32
    // MoveFileExW replaces an existing destination as one operation, unlike a
    // remove-then-rename sequence that risks losing a player's record on a crash.
    return MoveFileExW(source.wstring().c_str(), destination.wstring().c_str(), MOVEFILE_REPLACE_EXISTING) != 0;
#else
    std::error_code ec;
    fs::rename(source, destination, ec); // POSIX rename replaces atomically.
    return !ec;
#endif
}

std::string SchemaDocument()
{
    return "{\n  \"format\": \"forge-local-store\",\n  \"schemaVersion\": " +
           std::to_string(LocalStore::SchemaVersion) + "\n}\n";
}
} // namespace

LocalStore::LocalStore(std::string profileDirectory)
{
    if (!profileDirectory.empty())
        m_root = FsPathToUtf8(FsPathFromUtf8(profileDirectory) / "Forge");
}

bool LocalStore::IsSafeIdentifier(const std::string& value)
{
    if (value.empty() || value == "." || value == "..")
        return false;

    for (unsigned char c : value)
    {
        if (!(std::isalnum(c) || c == '-' || c == '_' || c == '.'))
            return false;
    }
    return true;
}

std::string LocalStore::RecordPath(const std::string& domain, const std::string& playerId) const
{
    if (m_root.empty() || !IsSafeIdentifier(domain) || !IsSafeIdentifier(playerId))
        return {};
    return FsPathToUtf8(FsPathFromUtf8(m_root) / domain / (playerId + ".json"));
}

bool LocalStore::EnsureInitialized() const
{
    if (m_root.empty())
        return false;

    std::error_code ec;
    const fs::path rootPath = FsPathFromUtf8(m_root);
    fs::create_directories(rootPath, ec);
    if (ec || !fs::is_directory(rootPath, ec))
        return false;

    const fs::path schemaPath = rootPath / "schema.json";
    if (fs::exists(schemaPath, ec))
        return !ec;

    const std::string schema = SchemaDocument();
    const std::string schemaPathUtf8 = FsPathToUtf8(schemaPath);
    return WriteFileUtf8(schemaPathUtf8.c_str(), schema.data(), schema.size());
}

bool LocalStore::Save(const std::string& domain, const std::string& playerId, const std::string& json) const
{
    const std::string recordPath = RecordPath(domain, playerId);
    if (recordPath.empty() || json.empty() || !EnsureInitialized())
        return false;

    const fs::path destination(recordPath);
    std::error_code ec;
    fs::create_directories(destination.parent_path(), ec);
    if (ec)
        return false;

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path temporary = FsPathFromUtf8(FsPathToUtf8(destination) + ".tmp." + std::to_string(nonce));
    const std::string temporaryUtf8 = FsPathToUtf8(temporary);
    if (!WriteFileUtf8(temporaryUtf8.c_str(), json.data(), json.size()))
        return false;

    if (ReplaceFileAtomically(temporary, destination))
        return true;

    fs::remove(temporary, ec);
    return false;
}

bool LocalStore::Load(const std::string& domain, const std::string& playerId, std::string& json) const
{
    json.clear();
    const std::string recordPath = RecordPath(domain, playerId);
    if (recordPath.empty() || !EnsureInitialized())
        return false;

    const std::vector<char> bytes = ReadFileUtf8(recordPath.c_str());
    if (bytes.empty())
        return false;
    json.assign(bytes.begin(), bytes.end());
    return true;
}

bool LocalStore::Remove(const std::string& domain, const std::string& playerId) const
{
    const std::string recordPath = RecordPath(domain, playerId);
    if (recordPath.empty() || !EnsureInitialized())
        return false;

    std::error_code ec;
    fs::remove(FsPathFromUtf8(recordPath), ec);
    return !ec;
}
} // namespace Poseidon::Forge
