#include "cbmp.h"
#include "gifdec.h"
#include <assert.h>
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <signal.h>
#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <unistd.h>     /* read, write, close */

void error(const char *msg) {
  perror(msg);
  exit(0);
}

int portno = 6969;
char *host = "5.59.97.201";
char *message_fmt = "POST /updatePixel? HTTP/1.0\r\n\
Host: %s\r\n\
Content-Type: application/json\r\n\
Content-Length: %lu\r\n\
\r\n\
%s";

/* create the socket */

char message[1024];
char response[1024];
char params[256];
struct hostent *server;
struct sockaddr_in serv_addr;
int sockfd, bytes, sent, received, total;

unsigned x;
unsigned y;

BMP *bimg;
gd_GIF *gimg;
unsigned w, h;
void quit(int sig) {
  /* close the socket */
  close(sockfd);
  exit(0);
}

void random_place() {
  while (1) {
  retry:;
    int _x = rand() % w + x;
    int _y = rand() % h + y;
    unsigned char r, g, b;
    get_pixel_rgb(bimg, _x, _y, &r, &g, &b);
    if (r == 255 && b == 255 && g == 255)
      goto retry;
    snprintf(params, 256,
             "{\"x\": \"%d\",\r\n\"y\": \"%d\",\r\n\"r\": \"%u\",\r\n\"g\": "
             "\"%u\",\r\n\"b\": \"%u\"}",
             _x, _y, r, g, b);
    size_t size = strlen(params);
    snprintf(message, 1024, message_fmt, host, size, params);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
      error("ERROR opening socket");

    /* lookup the ip address */
    server = gethostbyname(host);
    if (server == NULL)
      error("ERROR no such host");

    /* fill in the structure */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    /* connect the socket */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
      error("ERROR connecting");

    total = strlen(message);
    sent = 0;
    do {
      bytes = write(sockfd, message + sent, total - sent);
      if (bytes < 0)
        error("ERROR writing message to socket");
      if (bytes == 0)
        break;
      sent += bytes;
    } while (sent < total);

    /* receive the response */
    memset(response, 0, sizeof(response));
    total = sizeof(response) - 1;
    received = 0;
    do {
      bytes = read(sockfd, response + received, total - received);
      if (bytes < 0)
        error("ERROR reading response from socket");
      if (bytes == 0)
        break;
      received += bytes;
    } while (received < total);
    if (response[9] != '2') {
      printf("Failed retrying\n");
      goto retry;
    }

    printf("x: %d\ny: %d\nrgb: %u %u %u\n", _x, _y, r, g, b);

    close(sockfd);
  }
}

void linear_place() {
  /* lookup the ip address */
  server = gethostbyname(host);
  /* fill in the structure */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  if (server == NULL)
    error("ERROR no such host");
  while (1) {
    for (int _y = y; _y < w + y; _y++) {
      for (int _x = x; _x < h + x; _x++) {
      retry:;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
          error("ERROR opening socket");

        /* connect the socket */
        if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
            0)
          error("ERROR connecting");

        unsigned char r, g, b;
        get_pixel_rgb(bimg, _x - x, _y - y, &r, &g, &b);
        printf("x: %d\ny: %d\nrgb: %u %u %u\n", _x, _y, r, g, b);
        snprintf(
            params, 256,
            "{\"x\": \"%d\",\r\n\"y\": \"%d\",\r\n\"r\": \"%u\",\r\n\"g\": "
            "\"%u\",\r\n\"b\": \"%u\"}",
            _x, _y, r, g, b);
        size_t size = strlen(params);
        snprintf(message, 1024, message_fmt, host, size, params);
        total = strlen(message);
        sent = 0;
        do {
          bytes = write(sockfd, message + sent, total - sent);
          if (bytes < 0)
            error("ERROR writing message to socket");
          if (bytes == 0)
            break;
          sent += bytes;
        } while (sent < total);

        /* receive the response */
        memset(response, 0, sizeof(response));
        total = sizeof(response) - 1;
        received = 0;
        do {
          bytes = read(sockfd, response + received, total - received);
          if (bytes < 0)
            error("ERROR reading response from socket");
          if (bytes == 0)
            break;
          received += bytes;
        } while (received < total);
        if (response[9] != '2') {
          printf("Failed retrying\n");
          goto retry;
        }

        close(sockfd);
      }
    }
  }
}

void gif_linear_place() {
  /* lookup the ip address */
  server = gethostbyname(host);
  /* fill in the structure */
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
  if (server == NULL)
    error("ERROR no such host");
  while (1) {
    int frame = 0;
    while ((frame = gd_get_frame(gimg)) != 0) {

      for (int _y = y; _y < w + y; _y++) {
        for (int _x = x; _x < h + x; _x++) {
        retry:;
          sockfd = socket(AF_INET, SOCK_STREAM, 0);
          if (sockfd < 0)
            error("ERROR opening socket");

          /* connect the socket */
          if (connect(sockfd, (struct sockaddr *)&serv_addr,
                      sizeof(serv_addr)) < 0)
            error("ERROR connecting");

          unsigned char r, g, b;
          r = gimg->palette->colors[gimg->frame[_y * w + _x] + 0];
          g = gimg->palette->colors[gimg->frame[_y * w + _x] + 1];
          b = gimg->palette->colors[gimg->frame[_y * w + _x] + 2];
          printf("x: %d\ny: %d\nrgb: %u %u %u\n", _x, _y, r, g, b);
          snprintf(
              params, 256,
              "{\"x\": \"%d\",\r\n\"y\": \"%d\",\r\n\"r\": \"%u\",\r\n\"g\": "
              "\"%u\",\r\n\"b\": \"%u\"}",
              _x, _y, r, g, b);
          size_t size = strlen(params);
          snprintf(message, 1024, message_fmt, host, size, params);
          total = strlen(message);
          sent = 0;
          do {
            bytes = write(sockfd, message + sent, total - sent);
            if (bytes < 0)
              error("ERROR writing message to socket");
            if (bytes == 0)
              break;
            sent += bytes;
          } while (sent < total);

          /* receive the response */
          memset(response, 0, sizeof(response));
          total = sizeof(response) - 1;
          received = 0;
          do {
            bytes = read(sockfd, response + received, total - received);
            if (bytes < 0)
              error("ERROR reading response from socket");
            if (bytes == 0)
              break;
            received += bytes;
          } while (received < total);
          if (response[9] != '2') {
            printf("Failed retrying\n");
            goto retry;
          }

          close(sockfd);
        }
      }
    }
    gd_rewind(gimg);
  }
}

int main(int argc, char *argv[]) {

  signal(SIGINT, quit);
  /* first what are we going to send and where are we going to send it? */
  if (argc < 4) {
    puts("Parameters:  <image> <x> <y> <rand|linear>");
    exit(0);
  }

  char *ext = argv[1];
  while (*(++ext) != '.')
    ;

  if (strncmp(ext, ".bmp", 4) == 0) {
    x = atoi(argv[2]);
    y = atoi(argv[3]);
    bimg = bopen(argv[1]);
    if (!bimg) {
      puts("img don't exist\n");
      return 1;
    }
    w = get_width(bimg);
    h = get_height(bimg);
  } else if (strncmp(ext, ".gif", 4) == 0) {
    gimg = gd_open_gif(argv[1]);
    assert(gimg);
    w = gimg->width;
    h = gimg->height;
    gif_linear_place();
  }
  if (x + w > 256 || y + h > 256) {
    puts("Image too big");
    exit(-3);
  }

  if (strncmp(argv[4], "rand", 4) == 0) {
    random_place();
  } else {
    linear_place();
  }

  return 0;
}
