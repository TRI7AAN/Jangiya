// scan_registry CLI — Phase 2 registry scanner driver.
// Usage: scan_registry <dir>
// Prints the valid registry as JSON to stdout. Prints skips, rejections
// (with specific reasons), and a per-domain count summary to stderr.
// Exit 0 when nothing was rejected; exit 1 on any rejection.

#include <iostream>
#include <string>
#include <vector>

#include "jocky/stdlib/script_metadata.hpp"

namespace {

const std::vector<std::string>& domain_order() {
    static const std::vector<std::string> order = {
        "recon",      "netforensics", "hostforensics",
        "timeline",   "compliance",   "report",
    };
    return order;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: scan_registry <dir>\n";
        return 2;
    }
    jocky::ScanResult result = jocky::scan_registry_dir(argv[1]);
    std::cout << jocky::registry_to_json(result) << "\n";
    for (const std::string& skipped : result.skipped) {
        std::cerr << "SKIP " << skipped << "\n";
    }
    for (const std::string& rejection : result.rejections) {
        std::cerr << "REJECT " << rejection << "\n";
    }
    std::cerr << "SUMMARY " << result.registry.size()
              << " registered, " << result.rejections.size()
              << " rejected, " << result.skipped.size() << " skipped\n";
    for (const std::string& domain : domain_order()) {
        int count = 0;
        auto found = result.per_domain_counts.find(domain);
        if (found != result.per_domain_counts.end()) {
            count = found->second;
        }
        std::cerr << "SUMMARY " << domain << ": ";
        if (count == 0) {
            std::cerr << "0 registered (Phase 9 pending)";
        } else {
            std::cerr << count << " registered";
        }
        std::cerr << "\n";
    }
    return result.rejections.empty() ? 0 : 1;
}
