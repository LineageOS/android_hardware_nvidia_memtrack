#include "Memtrack.h"

#include <fstream>

#include <linux/nvmap.h>

#define NVMAP_DEBUGFS_DIR "/sys/kernel/debug/nvmap/handles_by_pid/"

namespace android {
namespace hardware {
namespace memtrack {
namespace V1_0 {
namespace implementation {

using ::android::hardware::memtrack::V1_0::MemtrackFlag;

bool Memtrack::valid_type(MemtrackType requested_type, uint32_t entry_type) {
    switch (requested_type) {
        case MemtrackType::GL:
            if (entry_type == 0x0100 || (entry_type >= 0x0701 && entry_type <= 0x0705))
                return true;
            break;

        default:
            break;
    };

    return false;
}

void Memtrack::getNvmapMemory(int32_t pid, MemtrackType type, hidl_vec<MemtrackRecord> records) {
    struct nvmap_debugfs_handles_header hdr;
    struct nvmap_debugfs_handles_entry  entry;

    records.resize(8);
    records[0].flags = MemtrackFlag::SMAPS_ACCOUNTED   | MemtrackFlag::PRIVATE    | MemtrackFlag::DEDICATED | MemtrackFlag::NONSECURE;
    records[1].flags = MemtrackFlag::SMAPS_UNACCOUNTED | MemtrackFlag::PRIVATE    | MemtrackFlag::DEDICATED | MemtrackFlag::NONSECURE;
    records[2].flags = MemtrackFlag::SMAPS_ACCOUNTED   | MemtrackFlag::SHARED_PSS | MemtrackFlag::DEDICATED | MemtrackFlag::NONSECURE;
    records[3].flags = MemtrackFlag::SMAPS_UNACCOUNTED | MemtrackFlag::SHARED_PSS | MemtrackFlag::DEDICATED | MemtrackFlag::NONSECURE;
    records[4].flags = MemtrackFlag::SMAPS_ACCOUNTED   | MemtrackFlag::PRIVATE    | MemtrackFlag::SYSTEM    | MemtrackFlag::NONSECURE;
    records[5].flags = MemtrackFlag::SMAPS_UNACCOUNTED | MemtrackFlag::PRIVATE    | MemtrackFlag::SYSTEM    | MemtrackFlag::NONSECURE;
    records[6].flags = MemtrackFlag::SMAPS_ACCOUNTED   | MemtrackFlag::SHARED_PSS | MemtrackFlag::SYSTEM    | MemtrackFlag::NONSECURE;
    records[7].flags = MemtrackFlag::SMAPS_UNACCOUNTED | MemtrackFlag::SHARED_PSS | MemtrackFlag::SYSTEM    | MemtrackFlag::NONSECURE;

    std::ifstream ifnode(NVMAP_DEBUGFS_DIR + std::to_string(pid), std::ifstream::binary);
    if (ifnode.is_open()) {
        ifnode.read((char*)&hdr, sizeof(struct nvmap_debugfs_handles_header));
        if (ifnode && hdr.version == 1) {
            while (ifnode.read((char*)&entry, sizeof(struct nvmap_debugfs_handles_entry)) &&
                   valid_type(type, entry.flags >> 16)) {
                if ((entry.base == 0) && (entry.share_count > 1)) {
                     records[6].sizeInBytes += (entry.mapped_size / entry.share_count);
                     records[7].sizeInBytes += ((entry.size - entry.mapped_size) / entry.share_count);
                } else if ((entry.base == 0) && (entry.share_count <= 1)) {
                     records[4].sizeInBytes += entry.mapped_size;
                     records[5].sizeInBytes += (entry.size - entry.mapped_size);
		} else if ((entry.base != 0) && (entry.share_count > 1)) {
                     records[2].sizeInBytes += (entry.mapped_size / entry.share_count);
                     records[3].sizeInBytes += ((entry.size - entry.mapped_size) / entry.share_count);
                } else {
                     records[0].sizeInBytes += entry.mapped_size;
                     records[1].sizeInBytes += (entry.size - entry.mapped_size);
                }
	    }
        }
    }
}

}  // namespace implementation
}  // namespace V1_0
}  // namespace memtrack
}  // namespace hardware
}  // namespace android
