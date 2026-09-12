#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "jocky/compiler/embedder.hpp"

int main() {
    if (jocky::sha256_bytes("") !=
            "e3b0c44298fc1c149afbf4c8996fb924"
            "27ae41e4649b934ca495991b7852b855" ||
        jocky::sha256_bytes("abc") !=
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad") {
        std::cerr << "SHA-256 known-vector mismatch\n";
        return 1;
    }

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "jocky-embedder-stale.sh";
    {
        std::ofstream out(path, std::ios::binary);
        out << "#!/bin/sh\nexit 0\n";
    }
    jocky::ScriptMetadata metadata;
    metadata.function = "jky_recon_stale_fixture";
    metadata.script_path = path.string();
    metadata.source_sha256 = jocky::sha256_file(path.string());
    {
        std::ofstream out(path, std::ios::binary | std::ios::app);
        out << "# changed after scan\n";
    }

    jocky::ShakeResult shaken;
    shaken.scripts.push_back(jocky::ResolvedScript{metadata, "direct call"});
    try {
        (void)jocky::embed_scripts(shaken);
        std::filesystem::remove(path);
        std::cerr << "stale digest was accepted\n";
        return 1;
    } catch (const jocky::EmbedError& ex) {
        std::filesystem::remove(path);
        return std::string(ex.what()).find("refusing stale embed") !=
                       std::string::npos
                   ? 0
                   : 1;
    }
}
