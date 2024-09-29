#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <json-c/json.h>
#include <ctype.h>

void printQuestion(){
    FILE* input;
    char word[150];
    printf("Problem Statement:\n");
    input = fopen("question.txt","r");
    if(input == NULL){
        printf("File not opened\n");
        return;
    }
    while(fscanf(input, "%150s", word) != EOF){
        printf("%s ", word);
    }
    printf("\n");
    fclose(input);
}

void printSampleTestCase() {
    FILE *fp = fopen("sampleTestCase.json", "r");
    if (fp == NULL) {
        printf("Json doesn't exist.\n");
        return;
    }

    char buffer[2048];
    fread(buffer, sizeof(char), 2048, fp);
    fclose(fp);

    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(buffer);

    struct json_object *number_of_test_cases;
    json_object_object_get_ex(parsed_json, "number_of_test_cases", &number_of_test_cases);
    
    printf("Sample test cases:\n");

    struct json_object *test_cases;
    json_object_object_get_ex(parsed_json, "test_cases", &test_cases);

    int array_len = json_object_array_length(test_cases);
    for (int i = 0; i < array_len; i++) {
        struct json_object *test_case = json_object_array_get_idx(test_cases, i);
        struct json_object *case_id;
        struct json_object *input;
        struct json_object *expected_output;

        json_object_object_get_ex(test_case, "case_id", &case_id);
        json_object_object_get_ex(test_case, "input", &input);
        json_object_object_get_ex(test_case, "expected_output", &expected_output);

        printf("Test Case %d:\n", json_object_get_int(case_id));
        printf("Input: %s\n", json_object_get_string(input));
        printf("Expected Output: %s\n\n", json_object_get_string(expected_output));
    }

    json_object_put(parsed_json);
}


int checkResult(const char *expected_output,int index) {
    FILE *file;
    char buffer[1024];
    int OutputCheck = 1;
    
    file = fopen("userResult.txt", "r");
    if (file == NULL) {
        printf("Cannot open userResult.txt\n");
        return 0;
    }
    fread(buffer,sizeof(char),1024,file);
    char* output = strtok(buffer,"\n###\n");
    for(int i = 1;i<index;i++){
    	output = strtok(NULL,"\n###\n");
    }
    printf("Your output\tExpected Output\n");
    printf("%s\t",output);
    printf("%s\n",expected_output);
    fclose(file);
    if(strcmp(output,expected_output)==0){
    	return 1;
    }
    else{
    	return 0;
    }
}


void checkTestCases() {


    FILE *fp = fopen("sampleTestCase.json", "r");
    if (fp == NULL) {
        printf("Json doesn't exist.\n");
        return;
    }

    char buffer[2048];
    fread(buffer, sizeof(char), 2048, fp);
    fclose(fp);

    struct json_object *parsed_json;
    parsed_json = json_tokener_parse(buffer);

    struct json_object *test_cases;
    json_object_object_get_ex(parsed_json, "test_cases", &test_cases);
    
    int array_len = json_object_array_length(test_cases);
    for (int i = 0; i < array_len; i++) {
        struct json_object *test_case = json_object_array_get_idx(test_cases, i);
        struct json_object *case_id;
        struct json_object *input;
        struct json_object *expected_output;

        json_object_object_get_ex(test_case, "case_id", &case_id);
        json_object_object_get_ex(test_case, "input", &input);
        json_object_object_get_ex(test_case, "expected_output", &expected_output);
        
        printf("Checking Test Case %d:\n", json_object_get_int(case_id));

        //For sreene. Here I have implemeneted a function checkResult() which directly gets from file
        // but we have to avoid that file and directly get the output from the compiler code
        
        int status = checkResult(json_object_get_string(expected_output),
        json_object_get_int(case_id));
        
        if (status == 1) {
            printf("Test Case %d Passed!\n", json_object_get_int(case_id));
        } else {
            printf("Test Case %d Failed!\n", json_object_get_int(case_id));
        }
    }

    json_object_put(parsed_json);
}



int main() {
    printQuestion();
    printSampleTestCase();
    //getCode();
    int pid = fork();
    /*if(pid>0){
    	wait(NULL);
    	checkTestCases();
    }
    else if(pid == 0){
    	execl("./compiler", "./compiler", NULL); 
    }*/

    return 0;
}

