/*  gptcom.c
    OpenAI API for a command line query to AI models
    Nov 2023 Michael Leidel

    TODO: take Gpt model code from ENV
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "JSON/cJSON.h"

char pmsg[6000] = {"\0"};
char prompt[5000] = {"\0"};
//char zendata[5000] = {"\0"}; // buffer to hold responses from dialogs
int i = 0;

struct string {
    char *ptr;
    size_t len;
};

void init_string(struct string *s) {
    s->len = 0;
    s->ptr = malloc(s->len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "malloc() failed\n");
        exit(EXIT_FAILURE);
    }
    s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
    /*
     This function repeatedly appends received data from the curl post
     to string struct s members ptr and len.
    */
    size_t new_len = s->len + size*nmemb;
    s->ptr = realloc(s->ptr, new_len+1);
    if (s->ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        exit(EXIT_FAILURE);
    }
    memcpy(s->ptr+s->len, ptr, size*nmemb);
    s->ptr[new_len] = '\0';
    s->len = new_len;

    return size*nmemb;
}

void zentext(char* content, char *title, int edit) {
    // argv[1] == 'p' use a GUI text box for prompt
    FILE *fp;
    char line[4096] = {'\0'};
    char cmd[128] = {'\0'};

    sprintf(cmd, "zenity --text-info --title='%s' --editable", title);
    strcpy(content, "\0");
    fp = popen(cmd, "r");
    if (fp == NULL) {
        puts("zentext fail");
        exit(EXIT_FAILURE);
    }
    do {
        fgets(line, 4096, fp);
        strcat(content, line);
    } while (!feof(fp));
    pclose(fp);
}

/*
    This code first prepares a curl POST request with the appropriate headers and body.
    Then it sends the request and stores the response in a string. Finally,
    it prints the response.
*/
int main(int argc, char *argv[]) {
    CURL *curl;
    CURLcode res;
    char* apikey;
    apikey = getenv("GPTKEY");
    char* gptmodel;
    gptmodel = getenv("GPTMOD");

    if (argc < 2) {
        zentext(prompt, "GptCom Prompt Edit", 1);
    } else {
    // Concatenate all arguments with a space in between
        for (i = 1; i < argc; i++) {
            strcat(prompt, argv[i]);
            if (i < argc - 1) {
                strcat(prompt, " ");
            }
        }
    }

    if (strlen(prompt) < 4) {  // no prompt then exit
        exit(EXIT_FAILURE);
    }

    printf("\nprompt:\n%s\n", prompt); // print the prompt to use

    // build the content POSTFIELD
    // strcpy(pmsg, "{ \"model\": \"gpt-3.5-turbo\", \


    sprintf(pmsg, "{ \"model\": \"%s\",", gptmodel);
    strcat(pmsg, "\"messages\": [{\"role\": \"user\", \"content\": \"");
    strcat(pmsg, prompt);
    strcat(pmsg, "\"}], \"temperature\": 0.7 }");
    //exit(EXIT_FAILURE);

    puts(pmsg);

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();

    if(curl) {
        struct string s;
        init_string(&s);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        char auth_header[128] = "Authorization: Bearer ";
        strcat(auth_header, apikey);
        headers = curl_slist_append(headers, auth_header);


        curl_easy_setopt(curl, CURLOPT_URL, "https://api.openai.com/v1/chat/completions");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pmsg);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
        
        res = curl_easy_perform(curl);

        /* Check for errors */ 
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        else {
            // printf("%s\nlength: %d\n", s.ptr, s.len);
            // parse the json response
            cJSON *json = cJSON_Parse(s.ptr);
            if (json == NULL) {  // process ERROR
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL) {
                    printf("Error: %s\n", error_ptr);
                }
                cJSON_Delete(json);
                return EXIT_FAILURE;
            }
            // access the JSON data
            cJSON *choices = cJSON_GetObjectItemCaseSensitive(json, "choices");
            cJSON *choice;
            cJSON_ArrayForEach(choice, choices) {
                cJSON *message = cJSON_GetObjectItemCaseSensitive(choice, "message");
                if (message != NULL) {
                    cJSON *content = cJSON_GetObjectItemCaseSensitive(message, "content");
                    if (cJSON_IsString(content) && (content->valuestring != NULL)) {
                        printf("\n\nresponse: %s\n", content->valuestring);
                    }
                }
            }

            // delete the JSON object
            cJSON_Delete(json);
        }

        /* always cleanup */ 
        curl_easy_cleanup(curl);
    } else {
        puts("curl failed to initialize");
    }

    curl_global_cleanup();

    return 0;
}
