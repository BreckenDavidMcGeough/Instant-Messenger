#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "serialize.h"


#define UBITNAME "breckenm"
#define UBITNAMELENGTH 8
/* Unpack the given packet into the buffer unpacked.  You can assume
 * that packed points to a packet buffer large enough to hold the
 * packet described therein, but you cannot assume that the packet is
 * otherwise valid.  You can assume that unpacked points to a character
 * buffer large enough to store the unpacked packet, if it is a valid
 * packet.
 *
 * Returns the packet type that was unpacked, or -1 if it is invalid.
 */
int unpack(char *unpacked, void *packed) { //works for status and message types
    int unpackedtype = *((int*)packed);
    if (unpackedtype == MESSAGE){

        //printf("its a message");

        size_t statuslength = *((size_t*)(packed+sizeof(int)+NAME_SIZE));

        if (statuslength == 0 || statuslength > MAX_MESSAGE_SIZE){
            return 1;
        }

        void* packed_ubit = packed+sizeof(int);
        int sizeubit = 0;
        for (int j = 0; *((char*)(packed_ubit+j)) != '\0'; j++){
            unpacked[j] = *((char*)(packed_ubit+j));
            sizeubit ++;
        }
        unpacked[sizeubit] = ':';
        unpacked[sizeubit+1] = ' ';
        void* packed_message = packed+sizeof(int)+NAME_SIZE+sizeof(size_t)+sizeof(size_t);
        int check = 0;
        for (int k = 0; k < statuslength; k++){
            if (*((char*)(packed_message+k)) != ' '){
                check ++;
            }
        }
        if (check == 0){
            return 1;
        }
        for (int i = 0; i < (int)statuslength; i++){
            unpacked[i+sizeubit+2] = *((char*)(packed_message+i));
        }

        unpacked[(int)statuslength+sizeubit+2] = '\0';

        //printf("%s", unpacked);
    }else if (unpackedtype == STATUS){

        //printf("its a status");
        size_t statuslength = *((size_t*)(packed+sizeof(int)+NAME_SIZE));

        if (statuslength == 0 || statuslength > MAX_MESSAGE_SIZE){
            return 1;
        }
        void* packed_ubit = packed+sizeof(int);
        int sizeubit = 0;
        for (int j = 0; *((char*)(packed_ubit+j)) != '\0'; j++){
            unpacked[j] = *((char*)(packed_ubit+j));
            sizeubit ++;
        }
        unpacked[sizeubit] = ' ';
        void* packed_status = packed+sizeof(int)+NAME_SIZE+sizeof(size_t)+sizeof(size_t);
        int check = 0;
        for (int k = 0; k < statuslength; k++){
            if (*((char*)(packed_status+k)) != ' '){
                check ++;
            }
        }
        if (check == 0){
            return 1;
        }
        for (int i = 0; i < (int)statuslength; i++){
            unpacked[i+sizeubit+1] = *((char*)(packed_status+i));
        }

        unpacked[(int)statuslength+sizeubit+1] = '\0';

        //printf("%s", unpacked);
    }else if (unpackedtype == LABELED){
        void* packed_ubit = packed+sizeof(int);
        int sizeubit = 0;
        for (int j = 0; *((char*)(packed_ubit+j)) != '\0'; j++){
            unpacked[j] = *((char*)(packed_ubit+j));
            sizeubit ++;
        }
        unpacked[sizeubit] = ':';
        unpacked[sizeubit+1] = ' ';
        unpacked[sizeubit+2] = '@';

        void* target = packed+sizeof(int)+NAME_SIZE+sizeof(size_t);
        size_t target_length = *((size_t*)(target));


        void* message = packed+sizeof(int)+NAME_SIZE;
        size_t message_length = *((size_t*)(message));

        if (message_length+target_length > MAX_MESSAGE_SIZE){
            return 1;
        }
        if (message_length == 0 || target_length == 0){
            return 1;
        }

        void* target_message = packed+sizeof(int)+NAME_SIZE+sizeof(size_t)+sizeof(size_t)+sizeof(size_t)+message_length;

        for (int i = 0; i < (int)target_length; i++){
            unpacked[i+sizeubit+3] = *((char*)(target_message+i));
        }

        unpacked[sizeubit+3+target_length] = ' ';

        void* message_message = packed+sizeof(int)+NAME_SIZE+sizeof(size_t)+sizeof(size_t)+sizeof(size_t);
        for (int k = 0; k < (int)message_length; k++){
            unpacked[k+sizeubit+3+(int)target_length+1] = *((char*)(message_message+k));
        }

        //printf("%s", unpacked);

        unpacked[(int)message_length+sizeubit+3+(int)target_length+1] = '\0';

    }else{
        return 1;
    }
    return unpackedtype;
}

/* Unpack the given packed packet into the given statistics structure.
 * You can assume that packed points to a packet buffer large enough to
 * hold the statistics packet, but you cannot assume that it is
 * otherwise valid.  You can assume that statistics points to a
 * statistics structure.
 *
 * Returns the packet type that was unpacked, or -1 if it is invalid.
 */
int unpack_statistics(struct statistics *statistics, void *packed) {
    int packettype = *(int*)packed;
    if (packettype == STATISTICS){
        void* ubit = packed+sizeof(int);
        int length = 0;
        for (int i = 0; *((char*)(ubit+i)) != '\0'; i ++){
            statistics->sender[i] = *((char*)(ubit+i));
            length ++;
        }
        statistics->sender[length] = '\0';

        //printf("%s\n", statistics->sender);

        int nlength = 0;
        void* mostactive = packed+sizeof(int)+NAME_SIZE;
        for (int j = 0; *((char*)(mostactive+j)) != '\0'; j ++){
            statistics->most_active[j] = *((char*)(mostactive+j));
            nlength ++;
        }
        statistics->most_active[nlength] = '\0';

        //printf("%s\n", statistics->most_active);

        void* macount = packed+sizeof(int)+NAME_SIZE+NAME_SIZE;

        statistics->most_active_count = *((int*)(macount));

        void* invacount = packed+sizeof(int)+NAME_SIZE+NAME_SIZE+sizeof(int); 
        
        statistics->invalid_count = *((long*)(invacount));

        void* refacount = packed+sizeof(int)+NAME_SIZE+NAME_SIZE+sizeof(int)+sizeof(long);    
        
        statistics->refresh_count = *((long*)(refacount));

        void* mescount = packed+sizeof(int)+NAME_SIZE+NAME_SIZE+sizeof(int)+sizeof(long)+sizeof(long);

        statistics->messages_count = *((int*)(mescount));     
    }else{
        return 1;
    }
    return packettype;
}
