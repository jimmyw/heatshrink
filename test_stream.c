#include "heatshrink_decoder.h"
#include "heatshrink_encoder.h"
#include <stdio.h>
#include <string.h>
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

struct buf {
  uint8_t comp[1000000];
  size_t comp_write_pos;
  size_t comp_read_pos;
};

void send_packet(heatshrink_encoder *hse, struct buf *b, uint8_t *input, size_t input_size);
void receive_package(heatshrink_decoder *hsd, struct buf *b, uint8_t *match, size_t match_size);

void send_packet(heatshrink_encoder *hse, struct buf *b, uint8_t *input, size_t input_size) {
  printf("send_package: comp_write_pos: %zu comp_read_pos: %zu\n",
         b->comp_write_pos, b->comp_read_pos);
  uint32_t sunk = 0;
  size_t total_poll = 0;
  size_t count = 0;

  while (sunk < input_size) {
    HSE_sink_res sink_res =
        heatshrink_encoder_sink(hse, &input[sunk], input_size - sunk, &count);
    if (sink_res == HSER_SINK_OK) {
      sunk += count;
      printf("SUNK %zd\n", count);
      if (sunk == input_size) {
        heatshrink_encoder_finish(hse);
      }
    } else {
      printf("sink error %d\n", sink_res);
      break;
    }

    HSE_poll_res pres;
    do { /* "turn the crank" */
      pres = heatshrink_encoder_poll(hse, &b->comp[b->comp_write_pos],
                                     sizeof(b->comp) - b->comp_write_pos, &count);
      printf("POLLED %zd pres: %d comp_write_pos: %zd\n", count, pres, b->comp_write_pos);
      b->comp_write_pos += count;
      total_poll += count;
     
    } while (pres == HSER_POLL_MORE);
  }
  printf("send_package: input bytes: %u  output bytes: %zu ratio: %f%%\n", sunk, total_poll, total_poll * 100.0/sunk);
}

void receive_package(heatshrink_decoder *hsd, struct buf *b, uint8_t *match, size_t match_size) {
  printf("receive_package: comp_read_pos: %zu comp_write_pos : %zu\n",
         b->comp_read_pos, b->comp_write_pos);



  size_t count = 0;
  uint8_t decompressed[10000];
  memset(decompressed,0, sizeof(decompressed));
  size_t decompressed_pos = 0;
  while (1) {
    HSD_sink_res sres = HSDR_SINK_OK;
    while (b->comp_read_pos < b->comp_write_pos && sres == HSDR_SINK_OK) {
      sres = heatshrink_decoder_sink(hsd, &b->comp[b->comp_read_pos],
                              b->comp_write_pos - b->comp_read_pos, &count);
      b->comp_read_pos += count;

      printf("SUNK %zd (%zu bytes left) sres: %d\n", count,
            b->comp_write_pos - b->comp_read_pos, sres);

      if (b->comp_read_pos == b->comp_write_pos) {

        HSD_finish_res fres = heatshrink_decoder_finish(hsd);
        printf("READ_FINISH fres=%d\n", fres);
        break;
      }
    }

    HSD_poll_res pres;
    do {
      pres = heatshrink_decoder_poll(hsd, &decompressed[decompressed_pos],
                                     sizeof(decompressed) - decompressed_pos,
                                     &count);

      decompressed_pos += count;

      printf("POLLED %zd (decompressed_pos: %zu) pres: %d\n", count,
             decompressed_pos, pres);
    } while (pres == HSDR_POLL_MORE);
    if (pres == HSDR_POLL_FINISHED)
      break;
  }
  for (size_t i = 0; i < decompressed_pos; i++)
    if (decompressed[i] == '\0')
      decompressed[i] = '@';
  printf("SIZE %zu expected %zu\n", decompressed_pos, match_size);
  printf("CONTENT: '%.*s' (%zu)\n", (int)decompressed_pos, (char *)decompressed,
         decompressed_pos);

  printf("MATCH: %s (%zu)\n",
         memcmp((char *)decompressed, (char *)match, match_size) == 0 ? "YES" : "NO", match_size);
}

char *input[] =  {
                  "/ESY5Q3DA1004 V3.04\n"
                  "\n"
                  "1-0:0.0.0*255(1ESY1160437500)\n"
                  "1-081.8.0*255(00000023,7246460*kWh)\n"
                  "1-0:21.7.0*255(000071.27*W)\n"
                  "1-0:41.7.0*255(000013.81*W)\n"
                  "1-0:61.7.0*255(000156.27*W)\n"
                  "1-0:1.7.0*255(000241.35*W)\n"
                  "1-0:96.5,5*255(80)\n"
                  "0-0:96.1.255*255(1ESY1160437500)\n"
                  "!\n"
                ,
                  "/ESY5Q3DA1004 V3.04\n"
                  "\n"
                  "1-0:0.0.0*255(1ESY1160437500)\n"
                  "1-081.8.0*255(00000024,7246460*kWh)\n"
                  "1-0:21.7.0*255(000072.27*W)\n"
                  "1-0:41.7.0*255(000014.81*W)\n"
                  "1-0:61.7.0*255(000156.27*W)\n"
                  "1-0:1.7.0*255(000242.35*W)\n"
                  "1-0:96.5,5*255(80)\n"
                  "0-0:96.1.255*255(1ESY1160437500)\n"
                  "!\n"
                ,
                  "/ESY5Q3DA1004 V3.04\n"
                  "\n"
                  "1-0:0.0.0*255(1ESY1160437500)\n"
                  "1-081.8.0*255(00000025,7246460*kWh)\n"
                  "1-0:21.7.0*255(000073.27*W)\n"
                  "1-0:41.7.0*255(000015.81*W)\n"
                  "1-0:61.7.0*255(000156.27*W)\n"
                  "1-0:1.7.0*255(000243.35*W)\n"
                  "1-0:96.5,5*255(80)\n"
                  "0-0:96.1.255*255(1ESY1160437500)\n"
                  "!\n"                  };

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  heatshrink_encoder hse;
  heatshrink_encoder_reset(&hse);
  heatshrink_decoder hsd;
  heatshrink_decoder_reset(&hsd);

  struct buf b;
  memset(&b, 0, sizeof(b));
  for (size_t i = 0; i < ARRAY_SIZE(input); i++) {
    printf("*********************** SEND PACKAGE %zu START ************************\n",i);
    send_packet(&hse, &b, (uint8_t *)input[i], strlen(input[i]));
    printf("*********************** SEND PACKAGE %zu DONE ************************\n",i);

    printf("*********************** RECV PACKAGE %zu START ************************\n",i);
    receive_package(&hsd, &b, (uint8_t *)input[i], strlen(input[i]));
    printf("*********************** RECV PACKAGE %zu DONE ************************\n",i);
  }

  return 0;
}
