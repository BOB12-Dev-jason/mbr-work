#include <iostream>
#include <fstream>

using namespace std;

// partition table entry.total 16 bytes.
typedef struct {
    uint8_t boot_flag;       // 0x08: boot (active), 0x00: non-boot(inactive)
    uint8_t chs_start[3];    // not used
    uint8_t type;            // partition type. 0x07: partition, 0x05: EBR
    uint8_t chs_end[3];      // not used
    uint32_t lba_start;    // lba start address. 4 byte
    uint32_t sector_num;   // lba number of sectors. 4 byte
}table_entry;


// MBR. total 512 byte. (1 sector)
typedef struct {

    uint8_t bootCode[446]; // bootCode 446 byte

    table_entry partition[4]; // partition table entry 16 * 4 byte

    uint8_t signature[2]; // signature 2 byte

}MBR;

void printEntry(table_entry *entry) {

    /*
    printf("%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        entry->boot_flag,
        entry->chs_start[0], entry->chs_start[1], entry->chs_start[2],
        entry->type,
        entry->chs_end[0], entry->chs_end[1], entry->chs_end[2],
        entry->lba_start[0], entry->lba_start[1], entry->lba_start[2], entry->lba_start[3],
        entry->sector_num[0], entry->sector_num[1], entry->sector_num[2], entry->sector_num[3]);
        */

    uint8_t boot_flag = entry->boot_flag;
    printf("boot flag: %02x ", boot_flag);
    if (boot_flag == 0x08)
        puts("active");
    else if (boot_flag == 0x00)
        puts("inactive");

    uint32_t lbaStart = entry->lba_start;
    printf("start LBA address: %x\n", lbaStart);

    uint32_t sectorNum = entry->sector_num;
    printf("number of sectors: %x\n", sectorNum);

    uint64_t partSize = sectorNum * 0x200;
    printf("partition size: %llu bytes\n", partSize);

}


int main() {
    
    ifstream source;

    source.open("data/mbr_128.dd", ios::binary | ios::in);

    if (!source) {
        cout << "파일 열기 오류" << endl;
        return -1;
    }

    MBR mbr;

    // 파일의 첫 부분으로 이동
    source.seekg(ios::beg);

    source.seekg(446);
    
    source.read((char*) mbr.partition, sizeof(mbr.partition)); // read 64 byte (16 * 4)

    int partitionNum = 1;
    // printf("%d\n", mbr.part_table_entry);
    for (int i = 0; i < 4; i++) {
        
        table_entry partition = mbr.partition[i];
        if (partition.type == 0x07) {
            printf("\npartition %d------------------------------\n", partitionNum++);
            printEntry(&partition);
            printf("real offset: %x\n", (partition.lba_start * 0x200));

            // print file system type
            uint64_t realOffset = partition.lba_start * 0x200;
            source.seekg(realOffset + 3);
            int8_t filename[5];
            source.read((char*)filename, 4);
            filename[4] = '\0';
            printf("file system type: %s\n", filename);

        }
        // print ebr
        else if (partition.type == 0x05) {

            uint64_t ebr_start_offset = partition.lba_start * 0x200;// 3C1 0000
            uint64_t cur_ebr_offset = partition.lba_start * 0x200;

            table_entry next_partition; // ebr partition entry 1
            table_entry next_ebr; // ebr partition entry 2

            source.seekg(ebr_start_offset + 446);
            source.read((char*)&next_partition, sizeof(next_partition)); // ebr의 첫번째 entry. 사용되는 파티션에 대한 정보.
            source.read((char*)&next_ebr, sizeof(next_ebr)); // ebr의 두 번째 entry. 다음 ebr에 대한 정보.
            
            printf("\npartition %d------------------------------\n", partitionNum++);
            printEntry(&next_partition);

            uint64_t next_partition_offset = cur_ebr_offset + next_partition.lba_start * 0x200;
            printf("real offset: %llx\n", next_partition_offset);
            int8_t filename[5];
            source.seekg(next_partition_offset + 3);
            source.read((char*)filename, 4);
            filename[4] = '\0';
            printf("file system type: %s\n", filename);


            uint64_t next_ebr_offset = ebr_start_offset + (next_ebr.lba_start * 0x200);

            // ebr을 통해 계산한 내용 반복
            while (next_ebr.lba_start != 0x00) {
                // 다음 ebr의 partition table entry로 이동
                source.seekg(next_ebr_offset + 446);
                source.read((char*)&next_partition, sizeof(next_partition));
                source.read((char*)&next_ebr, sizeof(next_ebr));

                cur_ebr_offset = next_ebr_offset;
                next_ebr_offset = ebr_start_offset + (next_ebr.lba_start * 0x200);

                printf("\npartition %d------------------------------\n", partitionNum++);
                printEntry(&next_partition);

                uint64_t next_partition_offset = cur_ebr_offset + next_partition.lba_start * 0x200;
                printf("real offset: %llx\n", next_partition_offset);
                int8_t filename[5];
                source.seekg(next_partition_offset + 3);
                source.read((char*)filename, 4);
                filename[4] = '\0';
                printf("file system type: %s\n", filename);

                
            }

        }
        else
            continue;
        
    }
    


}


