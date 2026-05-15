#include <cstdio>
#include <curl/curl.h>
int main() {
    printf("curl test start\n");
    CURLcode rc = curl_global_init(CURL_GLOBAL_DEFAULT);
    printf("curl_global_init: %d\n", rc);
    curl_global_cleanup();
    printf("curl test done\n");
    return 0;
}
