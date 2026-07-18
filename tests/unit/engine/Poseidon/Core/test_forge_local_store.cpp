#include <catch2/catch_test_macros.hpp>

#include <Poseidon/Core/Forge/ForgeLocalStore.hpp>

#include <chrono>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;

TEST_CASE("Forge local store persists isolated player-domain records", "[forge][persistence]")
{
    const fs::path root = fs::temp_directory_path() /
                          ("poseidon_forge_local_store_test_" +
                           std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code ec;
    fs::remove_all(root, ec);

    Poseidon::Forge::LocalStore store(root.string());
    REQUIRE(store.Save("actor", "76561198000000000", R"({"name":"Player One","cash":100})"));
    REQUIRE(store.Save("bank", "76561198000000000", R"({"balance":250})"));

    std::string actor;
    REQUIRE(store.Load("actor", "76561198000000000", actor));
    REQUIRE(actor == R"({"name":"Player One","cash":100})");
    REQUIRE(fs::is_regular_file(root / "Forge" / "schema.json"));

    std::string bank;
    REQUIRE(store.Load("bank", "76561198000000000", bank));
    REQUIRE(bank == R"({"balance":250})");
    REQUIRE(store.Remove("actor", "76561198000000000"));
    REQUIRE_FALSE(store.Load("actor", "76561198000000000", actor));

    fs::remove_all(root, ec);
}

TEST_CASE("Forge local store rejects unsafe path identifiers", "[forge][persistence]")
{
    REQUIRE(Poseidon::Forge::LocalStore::IsSafeIdentifier("actor.hot-v1"));
    REQUIRE_FALSE(Poseidon::Forge::LocalStore::IsSafeIdentifier("../actor"));
    REQUIRE_FALSE(Poseidon::Forge::LocalStore::IsSafeIdentifier("actor/other"));
    REQUIRE_FALSE(Poseidon::Forge::LocalStore::IsSafeIdentifier(""));
}
