//Name: Nicholas Poage
//Cwid: 12250963
//Compile Instructions: gcc -Wall NicholasPoage_mm2.c -std=c99
//Run Instructions: ./a.exe InputFile.txt int
//Will output to output.txt
//Comments:Designed for Linux
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const int pageSize = 256;
const int pageTableEntries = 256;
const int bits = 8;
const int frameSize = 256;
#define TlbEntries  16//not sure why const int tlbEntries doesnt work
int tlbHead =  0; 
int tlbTail =  0; 
int tlbIndex = 0;
int tlbHit = 0;
int currentTime = 0;

typedef struct {
    int pageNumber;
    int frameNumber;
} TLBEntry;

TLBEntry tlb[TlbEntries];

int getPageNumber(unsigned int address) {
    unsigned int pageNumber = (address >>  bits) &  0xFF;
    //printf("Page number: %d\n", pageNumber);
    return pageNumber;
}

int getOffset(unsigned int address) {
    unsigned int offset = address &  0xFF;
    return offset;
}

int checkTLB(int pageNumber) {
    for (int i =  0; i < TlbEntries; i++) {
        if (tlb[i].pageNumber == pageNumber) {
            tlbHit++;
            return tlb[i].frameNumber;
        }
    }
    return -1;
}

void updateTLB(int pageNumber, int frameNumber) {
    if (tlbHead != tlbTail) {
        tlbHead = (tlbHead +  1) % TlbEntries;
    }

    tlb[tlbTail].pageNumber = pageNumber;
    tlb[tlbTail].frameNumber = frameNumber;

    tlbTail = (tlbTail +  1) % TlbEntries;
}


int getCurrentTime() {
    currentTime++;
    return currentTime;
}

void updateLastAccessTime(int frameNumber, int* lastAccessTime) {
    lastAccessTime[frameNumber] = getCurrentTime();
     //sleep(.5);
    //problem is time is returning the same time for every value
    //printf("last Access time at %d, %d\n", frameNumber, lastAccessTime[frameNumber]);
}

int findLRUFrame(int amountOfFrames, int* lastAccessTime) {
    int lruFrame =  0;
    int oldestAccessTime = lastAccessTime[0];

    for (int i =  1; i < amountOfFrames; i++) {
        if (lastAccessTime[i] < oldestAccessTime) {
            oldestAccessTime = lastAccessTime[i];
            lruFrame = i;
        }
    }

    return lruFrame;
}


int main(int argc, char **argv) {
    FILE* inputFile;
    FILE* outputFile;
    FILE* storeFile;
    //FILE* debugFile;
    //debugFile = fopen("tesst.txt", "w");

    char line[100];
    int offset = -1;
    int address = -1;
    int pageNumber = -1;
    int frameNumber = 0;
    int pageTable[pageTableEntries];
    int physicalAddress;
    int value = 0;

    int numAddresses = 0;
    int numPageFaults = 0;
    double pageFaultRate = 0.0;
  
    for (int i = 0; i < pageTableEntries; i++) {
        pageTable[i] = -1;
    }

    if (argc != 3) {
        printf("Wrong number of arguments");
        return -1;
    }

    int amountOfFrames = atoi(argv[2]);
    
    int *frameAllocation = calloc(amountOfFrames, sizeof(int));
    if (frameAllocation == NULL) {
        perror("Failed to allocate memory for frameAllocation");
        return  1;
    }

    char *physicalMemory = calloc(256 * frameSize, sizeof(char));
    if (physicalMemory == NULL) {
        perror("Failed to allocate memory for physicalMemory");
        free(frameAllocation);
        return  1;
    }

    int lastAccessTime[amountOfFrames];
    for (int i =  0; i < amountOfFrames; i++) {
        lastAccessTime[i] = -1;
    }

    inputFile = fopen(argv[1], "r");
    outputFile = fopen("output.txt", "w");
    storeFile = fopen("BACKING_STORE.bin", "rb");

    fprintf(outputFile, "# of frames: %d\n", amountOfFrames);

    if (storeFile == NULL) {
        perror("Error opening backing store file");
        free(frameAllocation);
        free(physicalMemory);
        return  1;
    }
    

    while (fgets(line, sizeof(line), inputFile) != NULL) {
        numAddresses++;
        address = atoi(line);
        offset = getOffset(address);
        pageNumber = getPageNumber(address);
        frameNumber = checkTLB(pageNumber);

        //printf("PAGE NUMBER: %d\n", pageNumber);
        //printf("Offset: %d\n", offset);
        //printf("ADDRESS: %s", line);
        //problem when access pagenumber through tlb or pageTable they are using those page numbers and the times need to be updates\d
        if (frameNumber != -1) {
            //fprintf(debugFile, "found in tlb: %d\n", address);
            physicalAddress = frameNumber * frameSize + offset;
            value = physicalMemory[physicalAddress];
            updateLastAccessTime(frameNumber, lastAccessTime);
            //printf ("Physical Address: %d\n", physicalAddress);
            //printf("Value: %d\n", value);
            //printf("Current index: %d\n\n", numAddresses);
        } else {
            frameNumber = pageTable[pageNumber];
            if (frameNumber != -1) {
                //printf("Address: %d PAGE NUMBER: %d Num Adrress: %d\n",address, pageTable[pageNumber], numAddresses);
                //fprintf(debugFile, "Found in page table: %d\n", address);
                physicalAddress = frameNumber * frameSize + offset;
                updateLastAccessTime(frameNumber, lastAccessTime);
                updateTLB(pageNumber, frameNumber);
                value = physicalMemory[physicalAddress];
            }
            else {
                numPageFaults++;
                frameNumber = findLRUFrame(amountOfFrames, lastAccessTime);
                //fprintf(debugFile,"Page Fault at: %d\n", address);
                updateLastAccessTime(frameNumber, lastAccessTime);
                int oldPageNumber = -1;
                for (int i =  0; i < pageTableEntries; i++) {
                    if (pageTable[i] == frameNumber) {
                        oldPageNumber = i;
                        break;
                    }
                }
                if (oldPageNumber != -1) {
                    pageTable[oldPageNumber] = -1;
                }
                //removed used in mm1
                /*for (frameNumber =  0; frameNumber < amountOfFrames; frameNumber++) {
                    if (frameAllocation[frameNumber] ==  0) {
                        frameAllocation[frameNumber] =  1;
                        break;
                    }//here will need to replace frame if full
                }*/

                if (fseek(storeFile, pageNumber * pageSize, SEEK_SET) !=  0) {
                    printf("Failed to seek to page %d\n", pageNumber);
                    perror("Error seeking in backing store file");
                    return  0;
                }

                char page[pageSize];
                size_t bytes_read = fread(page,  1, pageSize, storeFile);

                if (bytes_read != pageSize) {
                    printf("Error reading page %d from backing store file\n", pageNumber);
                    perror("Error reading from backing store file");
                    return  0;
                }

                memcpy(physicalMemory + frameNumber * frameSize, page, pageSize);
                pageTable[pageNumber] = frameNumber;
                updateTLB(pageNumber, frameNumber);
                physicalAddress = frameNumber * frameSize + offset;
                value = physicalMemory[physicalAddress];
                //printf("Address: %d PAGE NUMBER: %d Num Adrress: %d\n",address, pageTable[pageNumber], numAddresses);

            }

        }
        fprintf(outputFile, "Virtual address: %d Physical address: %d, Value: %d\n", address, physicalAddress, value);
    }   

    pageFaultRate = ((double)numPageFaults)/numAddresses;
    double tlbHitRate = ((double)tlbHit)/numAddresses;

    fprintf(outputFile, "Number of Translated Addresses = %d\n", numAddresses);
    fprintf(outputFile, "Page Faults = %d\n", numPageFaults);
    fprintf(outputFile, "Page Fault Rate = %.3f\n", pageFaultRate);
    fprintf(outputFile, "TLB Hits = %d\n",tlbHit);
    fprintf(outputFile, "TLB Hit Rate = %.3f", tlbHitRate);

    fclose(outputFile);
    fclose(inputFile);
    fclose(storeFile);
    free(frameAllocation);
    free(physicalMemory);
    return  0;
}

