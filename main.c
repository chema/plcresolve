// plcresolve -  PLC handle resolution for AT Protocol/Bluesky 
// Copyright (c) 2024 Chema Hernández Gil / AGPL-3.0 license
// https://github.com/chema/plcresolve

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define URL_WELL_KNOWN		"https://%s/.well-known/atproto-did"
#define URL_MAX_SIZE		256

// DID PLC Syntax based on https://web.plc.directory/spec/v0.1/did-plc: exactly 32 characters, "did:plc:" prefix+24 base 32 encoding set characters.
#define DID_PLC_SPEC_PATTERN		"^did:plc:[a-zA-Z2-7]{24}$"
#define DID_PLC_SPEC_SIZE			32

// Struct to hold CURL response data
struct curlResponse {
    char *data;
    size_t size;
};

// Callback function to handle incoming data
size_t curlReceiveData (void *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total_size = size * nmemb;
    struct curlResponse *response = (struct curlResponse *)userdata;

    // Allocate or expand memory to hold the response data
    char *new_data = realloc(response->data, response->size + total_size + 1);
    if (new_data == NULL) {
        fprintf(stderr, "Failed to allocate memory for response data\n");
        return 0;
    }

    response->data = new_data;
    memcpy(response->data + response->size, ptr, total_size);
    response->size += total_size;
    response->data[response->size] = '\0'; // Null-terminate the string

    return total_size;
}

const char *getWellKnownDID (const char *handle)
{
    CURL *curl;
    CURLcode res;
	char url[URL_MAX_SIZE]; // WHAT IS THE MAX SIZE?
    char errbuf[CURL_ERROR_SIZE];
	const char *DID = NULL;

    // Initialize libcurl
    curl = curl_easy_init();

    // PROVIDE A BUFFER TO STORE ERRORS IN
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
 
    // SET THE ERROR BUFFER AS EMPTY BEFORE PERFORMING A REQUEST
    errbuf[0] = 0;

    // Construct the URL using the URL_WELL_KNOWN
    snprintf(url, sizeof(url), "https://%s/.well-known/atproto-did", handle);
	
    if (curl) {
		struct curlResponse response = {NULL, 0};

       // Set curl options
        curl_easy_setopt(curl, CURLOPT_URL, url);                  			// URL to fetch
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlReceiveData );	// Callback function to store data
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);      			// Pass the response struct

        // Perform the file transfer
        res = curl_easy_perform(curl);

		if(res != CURLE_OK) {
	      fprintf(stderr, "%s\n", curl_easy_strerror(res)); // CHECK
		  size_t len = strlen(errbuf);
		  fprintf(stderr, "Error from libcurl: (%d) ", res);
		  if(len)
			fprintf(stderr, "%s%s", errbuf, ((errbuf[len - 1] != '\n') ? "\n" : ""));
		  else
			fprintf(stderr, "%s\n", curl_easy_strerror(res));
		  return NULL;
		}
		
		// CONFIRM HEADER AND CODE CHECK
		// ADD VALIDATOR
		
		if( response.size != DID_PLC_SPEC_SIZE) {
			printf("Response is not the correct size (32): %ld\n", response.size);
			return NULL;
		}
		
		// Output the received DID
		printf("Received DID: %s\n", response.data);
		printf("Received DID size: %ld\n", response.size);

		DID = strndup(response.data, response.size);
		free(response.data); // Free the allocated memory
    } else fprintf(stderr, "Failed to initialize curl\n");
	
	// Clean up
	curl_easy_cleanup(curl);
	return DID;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <Bluesky (AT Protocol) handle (domain name)>\n", argv[0]);
        return 1;
    }

    const char *handle = argv[1];

	const char *DID = getWellKnownDID (handle);
	if (DID) { // Check if DID is not NULL
        printf("Received Handle: %s\n", handle);
        printf("Received DID: %s\n", DID);

        // Free the allocated DID
        free((void *)DID);
    } else {
        fprintf(stderr, "Failed to retrieve valid DID for handle: %s\n", handle);
		return 1; // FIND CORRECT CODE
    }

    return 0;
}