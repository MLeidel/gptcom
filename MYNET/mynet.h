// mynet.h
#ifndef MYNET
#define MYNET

/***
* mynet.h
*
* bool webpage(char * url)
* bool webget(char * url)
* bool webpost(char * url, char * vp_data)
* bool webbrowser(const char * url)
*
* NOTE:
* requires -
*   sudo apt-get install libcurl4-openssl-dev
*
* NOTE: includes myc.h and curl.h
*   compiling: gcc $1.c -l curl -o $1
*
***/

#include <myc.h>
#include <curl/curl.h>

/***
 *                 __
 *     _    _____ / /  ___  ___ ____ ____
 *    | |/|/ / -_) _ \/ _ \/ _ `/ _ `/ -_)
 *    |__,__/\__/_.__/ .__/\_,_/\_, /\__/
 *                  /_/        /___/
 *
 * example:
 * webpage("https://someurl.com");
 * int totbytes will contain the bytesize of webpage_content
 * url can accommodate different protocols, like ftp
 * Recomend using Python or Ruby if possible
 */

char * webpage_content;
int totbytes = 0;
int bufsize = 0;

size_t load_content(char *buffer, size_t itemsize, size_t nitems, void* nop) {
  size_t bytes = itemsize * nitems;
  totbytes += bytes;
  if (totbytes > bufsize) {
    fprintf(stderr, "url buffer over %d - aborting\n", bufsize);
    exit(false);
  }
  strcat(webpage_content, buffer);
  return bytes;
}

bool webpage(char * mybuffer, int sz, char * url) {
  CURL *curl;
  CURLcode res;

  webpage_content = mybuffer;  // set to global pointer
  bufsize = sz;
  totbytes = 0;  // initialize globals

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();
  if(curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, load_content);

    res = curl_easy_perform(curl);

    if(CURLE_OK != res) {
      fprintf(stderr, "download problem: %s\n", curl_easy_strerror(res));
      exit(false);
    }
    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    return true;
  } else {
    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    fprintf(stderr, "curl init failed: %s\n", curl_easy_strerror(res));
    return false;
  }
}


/***
 *                 __            __
 *     _    _____ / /  ___ ____ / /_
 *    | |/|/ / -_) _ \/ _ `/ -_) __/
 *    |__,__/\__/_.__/\_, /\__/\__/
 *                   /___/
 *
 * example:
 * webget("https://myurl.com?key=value&key=value...etc.")
 * NOTE: reqstring must be urlencoded properly
 * success returns true, failure return false
 */
bool webget(char * reqstring) {
  CURL *curl;
  CURLcode res;

  curl_global_init(CURL_GLOBAL_ALL);

  curl = curl_easy_init();
  if(curl) {
    //----------------------------NOTE: requestring must be urlencoded
    curl_easy_setopt(curl, CURLOPT_URL, reqstring);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      return false;
    } else {
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      return true;
    }
  }
}


/***
 *                 __                 __
 *     _    _____ / /  ___  ___  ___ / /_
 *    | |/|/ / -_) _ \/ _ \/ _ \(_-</ __/
 *    |__,__/\__/_.__/ .__/\___/___/\__/
 *                  /_/
 *
 * example:
 * webpost(https://myurl.com", "key=value&key=value...etc.")
 * success returns true, failure return false
 */
bool webpost(char * url, char * vp_data) {
  CURL *curl;
  CURLcode res;

  /* In windows, this will init the winsock stuff */
  curl_global_init(CURL_GLOBAL_ALL);
  curl = curl_easy_init();
  if(curl) {
    //-----------------------------------NOTE: vp_data must be urlencoded
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, vp_data);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    res = curl_easy_perform(curl);
    /* Check for errors */
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      return false;
    } else {
      curl_easy_cleanup(curl);
      curl_global_cleanup();
      return true;
    }
  }
}


/***
 *                 __   __
 *     _    _____ / /  / /  _______ _    _____ ___ ____
 *    | |/|/ / -_) _ \/ _ \/ __/ _ \ |/|/ (_-</ -_) __/
 *    |__,__/\__/_.__/_.__/_/  \___/__,__/___/\__/_/
 *
 * example:
 * webbrowser(https://sample.com)
 * Linux only - uses xdg-open
 */
bool webbrowser(const char * url) {
  char action[1000];

  memset(action, 0, 1000);
  strcpy(action, "xdg-open ");
  strcat(action, url);
  strcat(action, " &");
  int rsp = system(action);
  if (rsp != 0) {
      return false;
  }
    return true;
}

#endif
