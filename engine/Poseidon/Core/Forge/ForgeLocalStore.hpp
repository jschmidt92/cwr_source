#pragma once

#include <string>

namespace Poseidon::Forge
{
/// File-backed persistence used while Forge runs without its Arma extension / database.
///
/// Records are intentionally scoped by domain and player id rather than by a mission
/// save. A dedicated server supplies its __SERVER__ profile directory as root; a local
/// game can supply the active player's profile directory. Neither identifier is ever
/// treated as a path component until it has passed IsSafeIdentifier().
class LocalStore
{
  public:
    static constexpr int SchemaVersion = 1;

    /// Creates <profileDirectory>/Forge and writes schema.json when it is absent.
    /// Existing schema versions are preserved so a future migration can inspect them.
    explicit LocalStore(std::string profileDirectory);

    /// Store an opaque JSON document for one player/domain record. The write is atomic
    /// on supported platforms: a crash cannot leave a partially-written destination.
    bool Save(const std::string& domain, const std::string& playerId, const std::string& json) const;

    /// Load the exact JSON document stored by Save. Returns false for a missing or
    /// unreadable record; callers can then construct their normal default state.
    bool Load(const std::string& domain, const std::string& playerId, std::string& json) const;

    /// Remove a record. Missing records count as already deleted.
    bool Remove(const std::string& domain, const std::string& playerId) const;

    /// Valid identifiers are ASCII letters, digits, '-', '_' and '.'. This keeps all
    /// Forge data rooted under its profile directory, including on Windows.
    static bool IsSafeIdentifier(const std::string& value);

    /// The on-disk root: <profileDirectory>/Forge.
    const std::string& Root() const { return m_root; }

  private:
    std::string RecordPath(const std::string& domain, const std::string& playerId) const;
    bool EnsureInitialized() const;

    std::string m_root;
};
} // namespace Poseidon::Forge
