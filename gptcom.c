/*  gptcom.c
    OpenAI API for a command line query to AI models
    Nov 2023 Michael Leidel

    requires ENV fields for: GPTKEY, GPTMOD, USER
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <curl/curl.h>
#include "JSON/cJSON.h"

#define RED "\033[0;31m"
#define GRN "\033[0;32m"
#define ORG "\033[0;33m"   // kind of brown
#define YEL "\033[33;1m"  // bright: Yellow
#define BLU "\033[34;1m" // bright: blue
#define DFT "\033[0m\n" // reset to default color

char prompt[5000] = {"\0"};
int i = 0;

struct string {
    char *ptr;
    size_t len;
};

void init_string(struct string *s) {
    // setup a pointer to memory in the heap
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
     using string struct s, members ptr and len (see init_string)
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

    int ret;
    char *helptext = "\n$ gptcom type prompt as args in the command line\n"
                    "    submits prompt from command line\n\n"
                    "$ gptcom {no arguments}\n"
                    "    opens GUI editor to compose prompt\n\n"
                    "$ gptcom -h\n"
                    "    prints this help message\n\n"
                    "gptcom expects two Environment variables:\n"
                    "    GPTKEY=\"your private Open API key\"\n"
                    "    GPTMOD=\"OpenAI model\" e.g. \"gpt-4\"\n\n"
                    "gptcom uses 'zenity' for the GUI text edit box.\n\n";

    if (argc == 2) {
        ret = strcmp(argv[1], "-h");
        if(ret == 0) {
            printf("%s%s", ORG, helptext);  // display help text and exit
            exit(EXIT_SUCCESS);
        }
    }

    CURL *curl;
    CURLcode res;
    char* apikey;
    apikey = getenv("GPTKEY");
    char* gptmodel;
    gptmodel = getenv("GPTMOD");
    char* user;
    user = getenv("USER");
    char path[128] = {"\0"};
    sprintf(path, "/home/%s/.config/gptcom.log", user);

    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );


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

    for(int i = 0; i < strlen(prompt); i++) {
        if(prompt[i] == '\"') {
            prompt[i] = '\'';
        }
    }

    printf("\n%s%s prompt:\n%s\n", GRN, gptmodel, prompt); // print the prompt to use

    // create the "data" json for the libcurl POSTFIELDS
    /* example:
    {
        "model": "gpt-3.5-turbo",
        "messages": [
          {
            "role": "system",
            "content": "You are a helpful assistant."
          },
          {
            "role": "user",
            "content": "Hello!"
          }
        ]
    }
    */

    cJSON *root = cJSON_CreateObject();
    cJSON *messages = cJSON_CreateArray();

    cJSON *system_message = cJSON_CreateObject();
    cJSON_AddStringToObject(system_message, "role", "system");
    cJSON_AddStringToObject(system_message, "content", "You are a helpful assistant.");

    cJSON *user_message = cJSON_CreateObject();
    cJSON_AddStringToObject(user_message, "role", "user");
    cJSON_AddStringToObject(user_message, "content", prompt);

    cJSON_AddItemToArray(messages, system_message);
    cJSON_AddItemToArray(messages, user_message);

    cJSON_AddStringToObject(root, "model", gptmodel);
    cJSON_AddItemToObject(root, "messages", messages);

    char *pmsg = cJSON_Print(root);  // POSTFIELDS


    FILE *appfile;  // open the log file
    if ((appfile = fopen(path, "ab")) == NULL) {
        puts("open_for_append: failed");
        exit(EXIT_FAILURE);
    }

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
                        printf("\n\nresponse:\n  %s\n", content->valuestring);
                        fprintf(appfile,"\n----- %s\n%s prompt:\n  %s",
                                        asctime (timeinfo),
                                        gptmodel,
                                        prompt);
                        fprintf(appfile,"\nresponse:\n  %s\n\n", content->valuestring);
                        fclose(appfile);
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
