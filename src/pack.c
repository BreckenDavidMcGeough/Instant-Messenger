#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "serialize.h"


#define UBITNAME "breckenm"
#define UBITNAMELENGTH 8

/* Pack the user input provided in input into the appropriate message
 * type in the space provided by packed.  You can assume that input is a
 * NUL-terminated string, and that packed is a buffer of size
 * PACKET_SIZE.
 *
 * Returns the packet type for valid input, or -1 for invalid input.
 */
int pack(void *packed, char *input) {
    //determine packet type of input, then stride through void packed pointer and assign at each chunk value determined from input determined using standard format
    //these ifs dont handle invalid cases fix that shit later 
    int packettype = -1;
    if (input[0] == '/' && input[1] == 's' && input[2] == 't' && input[3] == 'a' && input[4] == 't' && input[5] == 's'){ //Statistiscs
        int check = 0;
        for (int k = 0; input[k] != '\0'; k++){
            if (input[k] != ' '){
                check ++;
            }
        }
        if (check == 0){
            return 1;
        }
        if (input[6] != '\0'){
            return 1;
        }



        packettype = STATISTICS;
        *((int*)(packed)) = packettype;

        void* packed_start_ubitname = packed+sizeof(int);
        char ubitname[8] = UBITNAME;
        for (int i = 0; i < UBITNAMELENGTH; i ++){
            *((char*)(packed_start_ubitname+i)) = ubitname[i];
        }
        for (int j = 8; j < NAME_SIZE; j++){
            *((char*)(packed_start_ubitname+j)) = '\0';
        }


    }else if (input[0] == '/' && input[1] == 'm' && input[2] == 'e' && input[3] == ' '){ //STATUS
        int check = 0;
        for (int k = 0; input[k] != '\0'; k++){
            if (input[k] != ' '){
                check ++;
            }
        }
        if (check == 0){
            return 1;
        }



        int start_status = 3;
        while (input[start_status] == ' '){
            start_status += 1;
        }
        //printf("%d\n", start_status);

        int packettype = STATUS;
        *((int*)(packed)) = packettype;

        void* packed_start_ubitname = packed+sizeof(int);
        char ubitname[8] = UBITNAME;
        for (int i = 0; i < UBITNAMELENGTH; i ++){
            *((char*)(packed_start_ubitname+i)) = ubitname[i];
        }
        for (int j = 8; j < NAME_SIZE; j++){
            *((char*)(packed_start_ubitname+j)) = '\0';
        }

        size_t length = 0;
        for (int i = start_status; input[i] != '\0'; i++){
            length++;
        }   
        if (length == 0 || length > MAX_MESSAGE_SIZE){
            return 1;
        }
        void* packed_start_status_length = packed_start_ubitname+NAME_SIZE;
        *((size_t*)(packed_start_status_length)) = length;
                
        void* packed_start_size = packed_start_status_length+sizeof(size_t);
        *((size_t*)(packed_start_size)) = 0;

        void* packed_start_status = packed_start_size+sizeof(size_t);

        for (int j = start_status; j < ((int)length+start_status); j++){
            *((char*)(packed_start_status+j-start_status)) = input[j];
            //printf("%c", *((char*)(packed_start_status+j-start_status)));
        }
    }else if (input[0] == '@' && input[1] != ' '){ //LABELED 
        int check = 0;
        for (int k = 0; input[k] != '\0'; k++){
            if (input[k] != ' '){
                check ++;
            }
        }
        if (check == 0){
            return 1;
        }



        packettype = LABELED;
        size_t size_target_name = 0;
        for (int i = 1; input[i] != ' '; i++){
            size_target_name++;
        }

        if (size_target_name > NAME_SIZE){
            return 1;
        }


        
        int start_message = 1+size_target_name;
        while (input[start_message] == ' '){
            start_message += 1;
        }

        *((int*)(packed)) = packettype;

        void* packed_start_ubitname = packed+sizeof(int);
        char ubitname[8] = UBITNAME;
        for (int i = 0; i < UBITNAMELENGTH; i ++){
            *((char*)(packed_start_ubitname+i)) = ubitname[i];
        }
        for (int j = 8; j < NAME_SIZE; j++){
            *((char*)(packed_start_ubitname+j)) = '\0';
        }

        //do same thing as status where get message length and message starting from first non space after target name. target lenght + 1 start
        //

        size_t message_length = 0;
        for (int i = start_message; input[i] != '\0'; i++){
            message_length++;
        }

        //printf("target length: %zu\n", size_target_name);

        //printf("message length: %zu\n", message_length);
        void* packed_ml = packed_start_ubitname + NAME_SIZE;

        *((size_t*)(packed_ml)) = message_length;

        void* packed_tl = packed_ml+sizeof(size_t);

        *((size_t*)(packed_tl)) = size_target_name;

        void* packed_zero = packed_tl + sizeof(size_t);

        *((size_t*)(packed_zero)) = 0;

        void* packed_message = packed_zero + sizeof(size_t);

        if (size_target_name+message_length > MAX_MESSAGE_SIZE){
            return 1;
        }
        if (size_target_name == 0 || message_length == 0){
            return 1;
        }

        for (int i = start_message; i < start_message+message_length; i++){
            *((char*)(packed_message+i-start_message)) = input[i];
            //printf("%c", *((char*)(packed_message+i-start_message)));
        }

        //printf("\n");


        void* packed_target = packed_message + message_length;

        for (int j = 1; j < 1+size_target_name; j++){
            *((char*)(packed_target+j-1)) = input[j];
            //printf("%c", *((char*)(packed_target+j-1)));
        }   

    }else{ //message
        int check = 0;
        for (int k = 0; input[k] != '\0'; k++){
            if (input[k] != ' '){
                check ++;
            }
        }
        if (check == 0){
            return 1;
        }



        packettype = MESSAGE;
        *((int*)(packed)) = packettype;

        void* packed_start_ubitname = packed+sizeof(int);
        char ubitname[8] = UBITNAME;
        for (int i = 0; i < UBITNAMELENGTH; i ++){
            *((char*)(packed_start_ubitname+i)) = ubitname[i];
        }
        for (int j = 8; j < NAME_SIZE; j++){
            *((char*)(packed_start_ubitname+j)) = '\0';
        }
        //^^optimize time complexity of this by having case for i >= 8 and modying void pointer to be end of ubtiname 

        size_t length = 0;
        for (int i = 0; input[i] != '\0'; i++){
            length++;
        }

        if (length == 0 || length > MAX_MESSAGE_SIZE){
            return 1;
        }
        void* packed_start_size = packed_start_ubitname+NAME_SIZE;
        *((size_t*)(packed_start_size)) = length;

        void* packed_start_zero = packed_start_size + sizeof(size_t);
        *((size_t*)(packed_start_zero)) = 0;

        void* packed_start_message = packed_start_zero+sizeof(size_t);

        for (int j = 0; input[j] != '\0'; j++){
            *((char*)(packed_start_message+j)) = input[j];
        }
    }
    return packettype;
}

/* Create a refresh packet for the given message ID.  You can assume
 * that packed is a buffer of size PACKET_SIZE.
 *
 * You should start by implementing this method!
 *
 * Returns the packet type.
 */
int pack_refresh(void *packed, int message_id) {
    int packettype = REFRESH;
    *(int*)packed = packettype;

    void* packed_ubitname = packed+sizeof(int);
    char ubitname[8] = UBITNAME;
    for (int i = 0; i < UBITNAMELENGTH; i++){
        *((char*)(packed_ubitname+i)) = ubitname[i];
    }
    for (int j = 8; j < NAME_SIZE; j++){
        *((char*)(packed_ubitname+j)) = '\0';
    }

    void* packed_message = packed_ubitname+NAME_SIZE;
    *(int*)packed_message = message_id;
    return packettype;
}
