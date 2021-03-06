/*******************************************************************************
** TEXTURE (palette texture generation) - Michael Behrens 2021-2022
*******************************************************************************/

/*******************************************************************************
** main.c
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define PI      3.14159265358979323846f
#define TWO_PI  6.28318530717958647693f

enum
{
  SOURCE_APPROX_NES = 0,
  SOURCE_APPROX_NES_ROTATED,
  SOURCE_COMPOSITE_16_1X,
  SOURCE_COMPOSITE_16_1X_ROTATED,
  SOURCE_COMPOSITE_08_2X,
  SOURCE_COMPOSITE_32_2X,
  SOURCE_COMPOSITE_06_0p75X,
  SOURCE_COMPOSITE_24_0p75X,
  SOURCE_COMPOSITE_12_1p50X,
  SOURCE_COMPOSITE_48_1p50X,
  SOURCE_COMPOSITE_08_2p50X,
  SOURCE_COMPOSITE_32_2p50X,
  SOURCE_EGA_EXTENDED_08,
  SOURCE_EGA_EXTENDED_32,
  SOURCE_CGA0_EXTENDED_16,
  SOURCE_CGA1_EXTENDED_16
};

enum
{
  HUE_MODIFIER_FULL = 0,
  HUE_MODIFIER_LOWER_HALF,
  HUE_MODIFIER_UPPER_HALF
};

/* the table step is 1 / (n + 1), where */
/* n is the number of colors per hue    */
#define COMPOSITE_06_TABLE_STEP 0.142857142857143f  /* 1/7  */
#define COMPOSITE_08_TABLE_STEP 0.111111111111111f  /* 1/9  */
#define COMPOSITE_12_TABLE_STEP 0.076923076923077f  /* 1/13 */
#define COMPOSITE_16_TABLE_STEP 0.058823529411765f  /* 1/17 */
#define COMPOSITE_24_TABLE_STEP 0.04f               /* 1/25 */
#define COMPOSITE_32_TABLE_STEP 0.03030303030303f   /* 1/33 */
#define COMPOSITE_48_TABLE_STEP 0.020408163265306f  /* 1/49 */

/* the luma is the average of the low and high voltages */
/* for the 1st half of each table, the low value is 0   */
/* for the 2nd half of each table, the high value is 1  */
/* the saturation is half of the peak-to-peak voltage   */

/* for the nes tables, the numbers were obtained    */
/* from information on the nesdev wiki              */
/* (see the "NTSC video" and "PPU palettes" pages)  */
float S_nes_p_p[4] = {0.399f,   0.684f, 0.692f, 0.285f};
float S_nes_lum[4] = {0.1995f,  0.342f, 0.654f, 0.8575f};
float S_nes_sat[4] = {0.1995f,  0.342f, 0.346f, 0.1425f};

float S_approx_nes_p_p[4] = {0.4f, 0.7f,  0.7f,   0.3f};
float S_approx_nes_lum[4] = {0.2f, 0.35f, 0.65f,  0.85f};
float S_approx_nes_sat[4] = {0.2f, 0.35f, 0.35f,  0.15f};

float S_composite_06_lum[6];
float S_composite_06_sat[6];

float S_composite_08_lum[8];
float S_composite_08_sat[8];

float S_composite_12_lum[12];
float S_composite_12_sat[12];

float S_composite_16_lum[16];
float S_composite_16_sat[16];

float S_composite_24_lum[24];
float S_composite_24_sat[24];

float S_composite_32_lum[32];
float S_composite_32_sat[32];

float S_composite_48_lum[48];
float S_composite_48_sat[48];

int   G_source;
int   G_palette_size;

unsigned char*  G_palette_data;

float*  S_luma_table;
float*  S_saturation_table;
int     S_table_length;

#define TEXTURE_YIQ2RGB()                                                      \
  r = (int) (((y + (i * 0.956f) + (q * 0.619f)) * 255) + 0.5f);                \
  g = (int) (((y - (i * 0.272f) - (q * 0.647f)) * 255) + 0.5f);                \
  b = (int) (((y - (i * 1.106f) + (q * 1.703f)) * 255) + 0.5f);                \
                                                                               \
  if (r < 0)                                                                   \
    r = 0;                                                                     \
  else if (r > 255)                                                            \
    r = 255;                                                                   \
                                                                               \
  if (g < 0)                                                                   \
    g = 0;                                                                     \
  else if (g > 255)                                                            \
    g = 255;                                                                   \
                                                                               \
  if (b < 0)                                                                   \
    b = 0;                                                                     \
  else if (b > 255)                                                            \
    b = 255;

#define TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, r, g, b)                      \
  G_palette_data[3 * (index) + 0] = r;                                         \
  G_palette_data[3 * (index) + 1] = g;                                         \
  G_palette_data[3 * (index) + 2] = b;

/*******************************************************************************
** generate_voltage_tables()
*******************************************************************************/
short int generate_voltage_tables()
{
  int k;

  /* composite 06 tables */
  for (k = 0; k < 3; k++)
  {
    S_composite_06_lum[k] = (k + 1) * COMPOSITE_06_TABLE_STEP;
    S_composite_06_lum[5 - k] = 1.0f - S_composite_06_lum[k];

    S_composite_06_sat[k] = S_composite_06_lum[k];
    S_composite_06_sat[5 - k] = S_composite_06_sat[k];
  }

  /* composite 08 tables */
  for (k = 0; k < 4; k++)
  {
    S_composite_08_lum[k] = (k + 1) * COMPOSITE_08_TABLE_STEP;
    S_composite_08_lum[7 - k] = 1.0f - S_composite_08_lum[k];

    S_composite_08_sat[k] = S_composite_08_lum[k];
    S_composite_08_sat[7 - k] = S_composite_08_sat[k];
  }

  /* composite 12 tables */
  for (k = 0; k < 6; k++)
  {
    S_composite_12_lum[k] = (k + 1) * COMPOSITE_12_TABLE_STEP;
    S_composite_12_lum[11 - k] = 1.0f - S_composite_12_lum[k];

    S_composite_12_sat[k] = S_composite_12_lum[k];
    S_composite_12_sat[11 - k] = S_composite_12_sat[k];
  }

  /* composite 16 tables */
  for (k = 0; k < 8; k++)
  {
    S_composite_16_lum[k] = (k + 1) * COMPOSITE_16_TABLE_STEP;
    S_composite_16_lum[15 - k] = 1.0f - S_composite_16_lum[k];

    S_composite_16_sat[k] = S_composite_16_lum[k];
    S_composite_16_sat[15 - k] = S_composite_16_sat[k];
  }

  /* composite 24 tables */
  for (k = 0; k < 12; k++)
  {
    S_composite_24_lum[k] = (k + 1) * COMPOSITE_24_TABLE_STEP;
    S_composite_24_lum[23 - k] = 1.0f - S_composite_24_lum[k];

    S_composite_24_sat[k] = S_composite_24_lum[k];
    S_composite_24_sat[23 - k] = S_composite_24_sat[k];
  }

  /* composite 32 tables */
  for (k = 0; k < 16; k++)
  {
    S_composite_32_lum[k] = (k + 1) * COMPOSITE_32_TABLE_STEP;
    S_composite_32_lum[31 - k] = 1.0f - S_composite_32_lum[k];

    S_composite_32_sat[k] = S_composite_32_lum[k];
    S_composite_32_sat[31 - k] = S_composite_32_sat[k];
  }

  /* composite 48 tables */
  for (k = 0; k < 24; k++)
  {
    S_composite_48_lum[k] = (k + 1) * COMPOSITE_48_TABLE_STEP;
    S_composite_48_lum[47 - k] = 1.0f - S_composite_48_lum[k];

    S_composite_48_sat[k] = S_composite_48_lum[k];
    S_composite_48_sat[47 - k] = S_composite_48_sat[k];
  }

  return 0;
}

/*******************************************************************************
** set_voltage_table_pointers()
*******************************************************************************/
short int set_voltage_table_pointers()
{
  if (G_source == SOURCE_APPROX_NES)
  {
    S_luma_table = S_approx_nes_lum;
    S_saturation_table = S_approx_nes_sat;
    S_table_length = 4;
  }
  else if (G_source == SOURCE_APPROX_NES_ROTATED)
  {
    S_luma_table = S_approx_nes_lum;
    S_saturation_table = S_approx_nes_sat;
    S_table_length = 4;
  }
  else if (G_source == SOURCE_COMPOSITE_06_0p75X)
  {
    S_luma_table = S_composite_06_lum;
    S_saturation_table = S_composite_06_sat;
    S_table_length = 6;
  }
  else if ( (G_source == SOURCE_COMPOSITE_08_2X)    || 
            (G_source == SOURCE_COMPOSITE_08_2p50X) || 
            (G_source == SOURCE_EGA_EXTENDED_08))
  {
    S_luma_table = S_composite_08_lum;
    S_saturation_table = S_composite_08_sat;
    S_table_length = 8;
  }
  else if (G_source == SOURCE_COMPOSITE_12_1p50X)
  {
    S_luma_table = S_composite_12_lum;
    S_saturation_table = S_composite_12_sat;
    S_table_length = 12;
  }
  else if ( (G_source == SOURCE_COMPOSITE_16_1X)          || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)  || 
            (G_source == SOURCE_CGA0_EXTENDED_16)         || 
            (G_source == SOURCE_CGA1_EXTENDED_16))
  {
    S_luma_table = S_composite_16_lum;
    S_saturation_table = S_composite_16_sat;
    S_table_length = 16;
  }
  else if (G_source == SOURCE_COMPOSITE_24_0p75X)
  {
    S_luma_table = S_composite_24_lum;
    S_saturation_table = S_composite_24_sat;
    S_table_length = 24;
  }
  else if ( (G_source == SOURCE_COMPOSITE_32_2X)    || 
            (G_source == SOURCE_COMPOSITE_32_2p50X) || 
            (G_source == SOURCE_EGA_EXTENDED_32))
  {
    S_luma_table = S_composite_32_lum;
    S_saturation_table = S_composite_32_sat;
    S_table_length = 32;
  }
  else if (G_source == SOURCE_COMPOSITE_48_1p50X)
  {
    S_luma_table = S_composite_48_lum;
    S_saturation_table = S_composite_48_sat;
    S_table_length = 48;
  }
  else
  {
    printf("Cannot set voltage table pointers; invalid source specified.\n");
    return 1;
  }

  return 0;
}

/*******************************************************************************
** generate_palette_approx_nes()
*******************************************************************************/
short int generate_palette_approx_nes()
{
  int   m;
  int   n;
  int   k;

  float y;
  float i;
  float q;

  int   r;
  int   g;
  int   b;

  unsigned char gradients[13][4][3];

  /* generate greys */
  for (n = 0; n < 4; n++)
  {
    gradients[0][n][0] = (int) ((S_approx_nes_lum[n] * 255) + 0.5f);
    gradients[0][n][1] = (int) ((S_approx_nes_lum[n] * 255) + 0.5f);
    gradients[0][n][2] = (int) ((S_approx_nes_lum[n] * 255) + 0.5f);
  }

  /* generate each hue */
  for (m = 0; m < 12; m++)
  {
    for (n = 0; n < 4; n++)
    {
      if (G_source == SOURCE_APPROX_NES_ROTATED)
      {
        y = S_approx_nes_lum[n];
        i = S_approx_nes_sat[n] * cos(TWO_PI * ((m * 30) + 15) / 360.0f);
        q = S_approx_nes_sat[n] * sin(TWO_PI * ((m * 30) + 15) / 360.0f);
      }
      else
      {
        y = S_approx_nes_lum[n];
        i = S_approx_nes_sat[n] * cos(TWO_PI * (m * 30) / 360.0f);
        q = S_approx_nes_sat[n] * sin(TWO_PI * (m * 30) / 360.0f);
      }

      r = (int) (((y + (i * 0.956f) + (q * 0.619f)) * 255) + 0.5f);
      g = (int) (((y - (i * 0.272f) - (q * 0.647f)) * 255) + 0.5f);
      b = (int) (((y - (i * 1.106f) + (q * 1.703f)) * 255) + 0.5f);

      /* bound rgb values */
      if (r < 0)
        r = 0;
      else if (r > 255)
        r = 255;

      if (g < 0)
        g = 0;
      else if (g > 255)
        g = 255;

      if (b < 0)
        b = 0;
      else if (b > 255)
        b = 255;

      gradients[m + 1][n][0] = r;
      gradients[m + 1][n][1] = g;
      gradients[m + 1][n][2] = b;
    }
  }

  /* allocate palette data */
  /*G_palette_data = malloc(sizeof(GLubyte) * 4 * G_palette_size * G_palette_size);*/
  G_palette_data = malloc(sizeof(unsigned char) * 4 * G_palette_size * G_palette_size);

  /* initialize palette data */
  for (n = 0; n < G_palette_size * G_palette_size; n++)
  {
    G_palette_data[4 * n + 0] = 0;
    G_palette_data[4 * n + 1] = 0;
    G_palette_data[4 * n + 2] = 0;
    G_palette_data[4 * n + 3] = 0;
  }

  /* generate palette 0 */

  /* transparency color */
  G_palette_data[4 * (4 * 64 + 0) + 0] = 0;
  G_palette_data[4 * (4 * 64 + 0) + 1] = 0;
  G_palette_data[4 * (4 * 64 + 0) + 2] = 0;
  G_palette_data[4 * (4 * 64 + 0) + 3] = 0;

  /* black */
  G_palette_data[4 * (4 * 64 + 1) + 0] = 0;
  G_palette_data[4 * (4 * 64 + 1) + 1] = 0;
  G_palette_data[4 * (4 * 64 + 1) + 2] = 0;
  G_palette_data[4 * (4 * 64 + 1) + 3] = 255;

  /* greys */
  for (n = 0; n < 4; n++)
  {
    G_palette_data[4 * (4 * 64 + n + 2) + 0] = gradients[0][n][0];
    G_palette_data[4 * (4 * 64 + n + 2) + 1] = gradients[0][n][1];
    G_palette_data[4 * (4 * 64 + n + 2) + 2] = gradients[0][n][2];
    G_palette_data[4 * (4 * 64 + n + 2) + 3] = 255;
  }

  /* white */
  G_palette_data[4 * (4 * 64 + 6) + 0] = 255;
  G_palette_data[4 * (4 * 64 + 6) + 1] = 255;
  G_palette_data[4 * (4 * 64 + 6) + 2] = 255;
  G_palette_data[4 * (4 * 64 + 6) + 3] = 255;

  /* hues */
  for (m = 0; m < 12; m++)
  {
    for (n = 0; n < 4; n++)
    {
      G_palette_data[4 * (4 * 64 + 7 + 4 * m + n) + 0] = gradients[m + 1][n][0];
      G_palette_data[4 * (4 * 64 + 7 + 4 * m + n) + 1] = gradients[m + 1][n][1];
      G_palette_data[4 * (4 * 64 + 7 + 4 * m + n) + 2] = gradients[m + 1][n][2];
      G_palette_data[4 * (4 * 64 + 7 + 4 * m + n) + 3] = 255;
    }
  }

  /* generate lighting levels for palette 0 */
  for (k = 0; k < 8; k++)
  {
    if (k == 4)
      continue;

    /* copy transparency color */
    memcpy(&G_palette_data[4 * (k * 64 + 0)], &G_palette_data[4 * (4 * 64 + 0)], 4);

    /* shadows */
    if (k < 4)
    {
      /* greys */
      for (m = 0; m < 4 - k + 1; m++)
        memcpy(&G_palette_data[4 * (k * 64 + m + 1)], &G_palette_data[4 * (4 * 64 + 1)], 4);

      memcpy(&G_palette_data[4 * (k * 64 + 4 - k + 2)], &G_palette_data[4 * (4 * 64 + 2)], 4 * (k + 1));

      /* hues */
      for (m = 0; m < 12; m++)
      {
        for (n = 0; n < 4 - k; n++)
          memcpy(&G_palette_data[4 * (k * 64 + 4 * m + 7 + n)], &G_palette_data[4 * (4 * 64 + 1)], 4);

        if (k != 0)
          memcpy(&G_palette_data[4 * (k * 64 + 4 * m + 7 + 4 - k)], &G_palette_data[4 * (4 * 64 + 4 * m + 7)], 4 * k);
      }
    }
    /* highlights */
    else if (k > 4)
    {
      /* greys */
      for (m = 0; m < k - 4 + 1; m++)
        memcpy(&G_palette_data[4 * (k * 64 + (6 - m))], &G_palette_data[4 * (4 * 64 + 6)], 4);

      memcpy(&G_palette_data[4 * (k * 64 + 1)], &G_palette_data[4 * (4 * 64 + (k - 4) + 1)], 4 * ((8 - k) + 1));

      /* hues */
      for (m = 0; m < 12; m++)
      {
        for (n = 0; n < k - 4; n++)
          memcpy(&G_palette_data[4 * (k * 64 + 4 * m + 7 + (3 - n))], &G_palette_data[4 * (4 * 64 + 6)], 4);

        memcpy(&G_palette_data[4 * (k * 64 + 4 * m + 7)], &G_palette_data[4 * (4 * 64 + 4 * m + 7 + (k - 4))], 4 * (8 - k));
      }
    }
  }

  /* generate palettes 1 - 5 (shift by 2 each time) */
  for (m = 1; m < 6; m++)
  {
    for (n = 0; n < 8; n++)
    {
      /* transparency & greys */
      memcpy(&G_palette_data[4 * ((8 * m + n) * 64 + 0)], &G_palette_data[4 * ((8 * (m - 1) + n) * 64 + 0)], 4 * 7);

      /* shifted back colors */
      memcpy(&G_palette_data[4 * ((8 * m + n) * 64 + 7)], &G_palette_data[4 * ((8 * (m - 1) + n) * 64 + 15)], 4 * 4 * 10);

      /* cycled around colors */
      memcpy(&G_palette_data[4 * ((8 * m + n) * 64 + 47)], &G_palette_data[4 * ((8 * (m - 1) + n) * 64 + 7)], 4 * 4 * 2);
    }
  }

  /* generate palette 6 (greyscale) */
  for (m = 0; m < 8; m++)
  {
    memcpy(&G_palette_data[4 * ((48 + m) * 64 + 0)], &G_palette_data[4 * (m * 64 + 0)], 4 * 7);

    for (n = 0; n < 12; n++)
      memcpy(&G_palette_data[4 * ((48 + m) * 64 + 4 * n + 7)], &G_palette_data[4 * (m * 64 + 2)], 4 * 4);
  }

  /* generate palette 7 (inverted greyscale) */
  for (m = 0; m < 8; m++)
  {
    memcpy(&G_palette_data[4 * ((56 + m) * 64 + 0)], &G_palette_data[4 * (m * 64 + 0)], 4);

    for (n = 1; n < 7; n++)
    {
      G_palette_data[4 * ((56 + m) * 64 + n) + 0] = G_palette_data[4 * (m * 64 + (7 - n)) + 0];
      G_palette_data[4 * ((56 + m) * 64 + n) + 1] = G_palette_data[4 * (m * 64 + (7 - n)) + 1];
      G_palette_data[4 * ((56 + m) * 64 + n) + 2] = G_palette_data[4 * (m * 64 + (7 - n)) + 2];
      G_palette_data[4 * ((56 + m) * 64 + n) + 3] = G_palette_data[4 * (m * 64 + (7 - n)) + 3];
    }

    for (n = 0; n < 12; n++)
      memcpy(&G_palette_data[4 * ((56 + m) * 64 + 4 * n + 7)], &G_palette_data[4 * ((56 + m) * 64 + 2)], 4 * 4);
  }

  return 0;
}

/*******************************************************************************
** generate_palette_composite_16_1x()
*******************************************************************************/
short int generate_palette_composite_16_1x()
{
  #define TEXTURE_NUM_HUES      12
  #define TEXTURE_THETA_STEP    (360 / TEXTURE_NUM_HUES)

  #define TEXTURE_NUM_GRADIENTS (TEXTURE_NUM_HUES + 1)
  #define TEXTURE_NUM_SHADES    16

  #define TEXTURE_NUM_LEVELS    (2 * TEXTURE_NUM_SHADES)
  #define TEXTURE_MID_LEVEL     (TEXTURE_NUM_LEVELS / 2)

  #define PALETTE_SIZE          256

  #define TEXTURE_NUM_ROTATIONS     6
  #define TEXTURE_NUM_HARD_TINTS    2

  #define TEXTURE_NUM_SUBPALETTES   (TEXTURE_NUM_ROTATIONS + TEXTURE_NUM_HARD_TINTS)

  #define TEXTURE_ROTATE_RIGHT(amount)                                   \
    (amount * (TEXTURE_NUM_HUES / TEXTURE_NUM_ROTATIONS))

  #define TEXTURE_ROTATE_LEFT(amount)                                    \
    ((TEXTURE_NUM_ROTATIONS - amount) * (TEXTURE_NUM_HUES / TEXTURE_NUM_ROTATIONS))

  int   m;
  int   n;
  int   k;

  int   p;

  float y;
  float i;
  float q;

  int   r;
  int   g;
  int   b;

  int   index;

  /* allocate palette data */
  /*G_palette_data = malloc(sizeof(GLubyte) * 3 * PALETTE_SIZE * PALETTE_SIZE);*/
  G_palette_data = malloc(sizeof(unsigned char) * 3 * PALETTE_SIZE * PALETTE_SIZE);

  /* initialize palette data */
  for (k = 0; k < PALETTE_SIZE * PALETTE_SIZE; k++)
  {
    index = k;
    TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 0, 0, 0)
  }

  /* initialize base subpalette */
  for (m = 0; m < TEXTURE_MID_LEVEL; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      for (k = 0; k < TEXTURE_NUM_SHADES; k++)
      {
        index = (TEXTURE_MID_LEVEL + m) * PALETTE_SIZE + n * TEXTURE_NUM_SHADES + k;
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
      }
    }
  }

  /* generate base subpalette */
  for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
  {
    for (k = 0; k < TEXTURE_NUM_SHADES; k++)
    {
      index = TEXTURE_MID_LEVEL * PALETTE_SIZE + n * TEXTURE_NUM_SHADES + k;

      y = S_luma_table[k];

      if (n == 0)
      {
        i = 0.0f;
        q = 0.0f;
      }
      else
      {
        i = S_saturation_table[k] * cos(TWO_PI * ((n - 1) * TEXTURE_THETA_STEP) / 360.0f);
        q = S_saturation_table[k] * sin(TWO_PI * ((n - 1) * TEXTURE_THETA_STEP) / 360.0f);
      }

      TEXTURE_YIQ2RGB()
      TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, r, g, b)
    }
  }

  /* shadows */
  for (m = 1; m < TEXTURE_MID_LEVEL; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      memcpy( &G_palette_data[3 * (m * PALETTE_SIZE + TEXTURE_NUM_SHADES * n + (TEXTURE_MID_LEVEL - m))], 
              &G_palette_data[3 * (TEXTURE_MID_LEVEL * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
              3 * m);
    }
  }

  /* highlights */
  for (m = TEXTURE_MID_LEVEL + 1; m < TEXTURE_NUM_LEVELS; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      memcpy( &G_palette_data[3 * (m * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
              &G_palette_data[3 * (TEXTURE_MID_LEVEL * PALETTE_SIZE + TEXTURE_NUM_SHADES * n + (m - TEXTURE_MID_LEVEL))], 
              3 * (TEXTURE_NUM_LEVELS - m));
    }
  }

  /* rotations */
  for (p = 1; p < TEXTURE_NUM_ROTATIONS; p++)
  {
    for (m = 0; m < TEXTURE_NUM_LEVELS; m++)
    {
      memcpy( &G_palette_data[3 * ((p * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE)], 
              &G_palette_data[3 * (m * PALETTE_SIZE)], 
              3 * TEXTURE_NUM_SHADES);

      memcpy( &G_palette_data[3 * ((p * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES)], 
              &G_palette_data[3 * (m * PALETTE_SIZE + (1 + TEXTURE_ROTATE_RIGHT(p)) * TEXTURE_NUM_SHADES)], 
              3 * TEXTURE_ROTATE_LEFT(p) * TEXTURE_NUM_SHADES);

      memcpy( &G_palette_data[3 * ((p * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + (1 + TEXTURE_ROTATE_LEFT(p)) * TEXTURE_NUM_SHADES)], 
              &G_palette_data[3 * (m * PALETTE_SIZE + TEXTURE_NUM_SHADES)], 
              3 * TEXTURE_ROTATE_RIGHT(p) * TEXTURE_NUM_SHADES);
    }
  }

  /* hard tints */
  for (m = 0; m < TEXTURE_NUM_LEVELS; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      /* blue tint */
      memcpy( &G_palette_data[3 * ((6 * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + n * TEXTURE_NUM_SHADES)], 
              &G_palette_data[3 * (m * PALETTE_SIZE + 6 * TEXTURE_NUM_SHADES)], 
              3 * TEXTURE_NUM_SHADES);

      /* red tint */
      memcpy( &G_palette_data[3 * ((7 * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + n * TEXTURE_NUM_SHADES)], 
              &G_palette_data[3 * (m * PALETTE_SIZE + 2 * TEXTURE_NUM_SHADES)], 
              3 * TEXTURE_NUM_SHADES);
    }
  }

  #undef TEXTURE_NUM_HUES
  #undef TEXTURE_THETA_STEP

  #undef TEXTURE_NUM_GRADIENTS
  #undef TEXTURE_NUM_SHADES

  #undef TEXTURE_NUM_LEVELS
  #undef TEXTURE_MID_LEVEL

  #undef PALETTE_SIZE

  #undef TEXTURE_NUM_ROTATIONS
  #undef TEXTURE_NUM_HARD_TINTS

  #undef TEXTURE_NUM_SUBPALETTES

  return 0;
}

/*******************************************************************************
** generate_palette_composite_08_2x()
*******************************************************************************/
short int generate_palette_composite_08_2x()
{
  #define TEXTURE_NUM_HUES      24
  #define TEXTURE_THETA_STEP    (360 / TEXTURE_NUM_HUES)

  #define TEXTURE_NUM_GRADIENTS (TEXTURE_NUM_HUES + 1)
  #define TEXTURE_NUM_SHADES    8

  #define TEXTURE_NUM_LEVELS    (2 * TEXTURE_NUM_SHADES)
  #define TEXTURE_MID_LEVEL     (TEXTURE_NUM_LEVELS / 2)

  #define PALETTE_SIZE          256

  #define TEXTURE_NUM_ROTATIONS     8
  #define TEXTURE_NUM_HARD_TINTS    0
  #define TEXTURE_NUM_SOFT_TINTS    1

  #define TEXTURE_NUM_SUBPALETTES   (TEXTURE_NUM_ROTATIONS + TEXTURE_NUM_HARD_TINTS)
  #define TEXTURE_NUM_BLENDS        (TEXTURE_NUM_SOFT_TINTS + 1)

  #define TEXTURE_TINT_HUES         5
  #define TEXTURE_TINT_INTERVAL     (TEXTURE_NUM_HUES / (TEXTURE_TINT_HUES + 1))

  #define TEXTURE_ROTATE_RIGHT(amount)                                   \
    (amount * (TEXTURE_NUM_HUES / TEXTURE_NUM_ROTATIONS))

  #define TEXTURE_ROTATE_LEFT(amount)                                    \
    ((TEXTURE_NUM_ROTATIONS - amount) * (TEXTURE_NUM_HUES / TEXTURE_NUM_ROTATIONS))

  int   m;
  int   n;
  int   k;

  int   p;
  int   st;

  float y;
  float i;
  float q;

  int   r;
  int   g;
  int   b;

  int   index;

  int   target_hue;
  int   tint_hue;

  int   delta;

  /* allocate palette data */
  /*G_palette_data = malloc(sizeof(GLubyte) * 3 * PALETTE_SIZE * PALETTE_SIZE);*/
  G_palette_data = malloc(sizeof(unsigned char) * 3 * PALETTE_SIZE * PALETTE_SIZE);

  /* initialize palette data */
  for (k = 0; k < PALETTE_SIZE * PALETTE_SIZE; k++)
  {
    index = k;
    TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 0, 0, 0)
  }

  /* initialize base subpalette */
  for (m = 0; m < TEXTURE_MID_LEVEL; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      for (k = 0; k < TEXTURE_NUM_SHADES; k++)
      {
        index = (TEXTURE_MID_LEVEL + m) * PALETTE_SIZE + n * TEXTURE_NUM_SHADES + k;
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
      }
    }
  }

  /* generate base subpalette */
  for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
  {
    for (k = 0; k < TEXTURE_NUM_SHADES; k++)
    {
      index = TEXTURE_MID_LEVEL * PALETTE_SIZE + n * TEXTURE_NUM_SHADES + k;

      y = S_luma_table[k];

      if (n == 0)
      {
        i = 0.0f;
        q = 0.0f;
      }
      else
      {
        i = S_saturation_table[k] * cos(TWO_PI * ((n - 1) * TEXTURE_THETA_STEP) / 360.0f);
        q = S_saturation_table[k] * sin(TWO_PI * ((n - 1) * TEXTURE_THETA_STEP) / 360.0f);
      }

      TEXTURE_YIQ2RGB()
      TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, r, g, b)
    }
  }

  /* shadows */
  for (m = 1; m < TEXTURE_MID_LEVEL; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      memcpy( &G_palette_data[3 * (m * PALETTE_SIZE + TEXTURE_NUM_SHADES * n + (TEXTURE_MID_LEVEL - m))], 
              &G_palette_data[3 * (TEXTURE_MID_LEVEL * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
              3 * m);
    }
  }

  /* highlights */
  for (m = TEXTURE_MID_LEVEL + 1; m < TEXTURE_NUM_LEVELS; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      memcpy( &G_palette_data[3 * (m * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
              &G_palette_data[3 * (TEXTURE_MID_LEVEL * PALETTE_SIZE + TEXTURE_NUM_SHADES * n + (m - TEXTURE_MID_LEVEL))], 
              3 * (TEXTURE_NUM_LEVELS - m));
    }
  }

  /* soft tints */
  for (st = 1; st < TEXTURE_NUM_BLENDS; st++)
  {
    /* initialize tint variables */
    if (st == 1)
      target_hue = 12; /* blue */
    else
      target_hue = 0;

    tint_hue = 0;
    delta = 0;

    /* tint colors */
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      /* determine tint hue */
      if (target_hue == 0)
        tint_hue = 0;
      else if ((n == 0) || (n == target_hue))
        tint_hue = target_hue;
      else
      {
        delta = n - target_hue;

        if (delta < 0)
          delta += TEXTURE_NUM_HUES;

        delta /= TEXTURE_TINT_INTERVAL;

        if (delta < (TEXTURE_TINT_HUES + 1) / 2)
          tint_hue = target_hue + delta;
        else
          tint_hue = target_hue - (TEXTURE_TINT_HUES - delta);

        if (tint_hue < 1)
          tint_hue += TEXTURE_NUM_HUES;
        else if (tint_hue >= TEXTURE_NUM_HUES + 1)
          tint_hue -= TEXTURE_NUM_HUES;
      }

      /* copy the appropriate hue from the base subpalette */
      for (m = 0; m < TEXTURE_NUM_LEVELS; m++)
      {
        memcpy( &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
                &G_palette_data[3 * (m * PALETTE_SIZE + TEXTURE_NUM_SHADES * tint_hue)], 
                3 * TEXTURE_NUM_SHADES);
      }
    }
  }

  /* rotations & hard tints */
  for (st = 0; st < TEXTURE_NUM_BLENDS; st++)
  {
    for (p = 1; p < TEXTURE_NUM_ROTATIONS; p++)
    {
      for (m = 0; m < TEXTURE_NUM_LEVELS; m++)
      {
        memcpy( &G_palette_data[3 * (((st * TEXTURE_NUM_SUBPALETTES + p) * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE)], 
                3 * TEXTURE_NUM_SHADES);

        memcpy( &G_palette_data[3 * (((st * TEXTURE_NUM_SUBPALETTES + p) * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + (1 + TEXTURE_ROTATE_RIGHT(p)) * TEXTURE_NUM_SHADES)], 
                3 * TEXTURE_ROTATE_LEFT(p) * TEXTURE_NUM_SHADES);

        memcpy( &G_palette_data[3 * (((st * TEXTURE_NUM_SUBPALETTES + p) * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + (1 + TEXTURE_ROTATE_LEFT(p)) * TEXTURE_NUM_SHADES)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES)], 
                3 * TEXTURE_ROTATE_RIGHT(p) * TEXTURE_NUM_SHADES);
      }
    }
  }

  #undef TEXTURE_NUM_HUES
  #undef TEXTURE_THETA_STEP

  #undef TEXTURE_NUM_GRADIENTS
  #undef TEXTURE_NUM_SHADES

  #undef TEXTURE_NUM_LEVELS
  #undef TEXTURE_MID_LEVEL

  #undef PALETTE_SIZE

  #undef TEXTURE_NUM_ROTATIONS
  #undef TEXTURE_NUM_HARD_TINTS
  #undef TEXTURE_NUM_SOFT_TINTS

  #undef TEXTURE_NUM_SUBPALETTES
  #undef TEXTURE_NUM_BLENDS 

  #undef TEXTURE_TINT_HUES
  #undef TEXTURE_TINT_INTERVAL

  return 0;
}

/*******************************************************************************
** generate_palette_composite_32_2x()
*******************************************************************************/
short int generate_palette_composite_32_2x()
{
  #define TEXTURE_NUM_HUES      24
  #define TEXTURE_THETA_STEP    (360 / TEXTURE_NUM_HUES)

  #define TEXTURE_NUM_GRADIENTS (TEXTURE_NUM_HUES + 1)
  #define TEXTURE_NUM_SHADES    32

  #define TEXTURE_NUM_LEVELS    (2 * TEXTURE_NUM_SHADES)
  #define TEXTURE_MID_LEVEL     (TEXTURE_NUM_LEVELS / 2)

  #define PALETTE_SIZE          1024

  #define TEXTURE_NUM_ROTATIONS     4
  #define TEXTURE_NUM_HARD_TINTS    0
  #define TEXTURE_NUM_SOFT_TINTS    3

  #define TEXTURE_NUM_SUBPALETTES   (TEXTURE_NUM_ROTATIONS + TEXTURE_NUM_HARD_TINTS)
  #define TEXTURE_NUM_BLENDS        (TEXTURE_NUM_SOFT_TINTS + 1)

  #define TEXTURE_TINT_HUES         5
  #define TEXTURE_TINT_INTERVAL     (TEXTURE_NUM_HUES / (TEXTURE_TINT_HUES + 1))

  #define TEXTURE_ROTATE_RIGHT(amount)                                   \
    (amount * (TEXTURE_NUM_HUES / TEXTURE_NUM_ROTATIONS))

  #define TEXTURE_ROTATE_LEFT(amount)                                    \
    ((TEXTURE_NUM_ROTATIONS - amount) * (TEXTURE_NUM_HUES / TEXTURE_NUM_ROTATIONS))

  int   m;
  int   n;
  int   k;

  int   p;
  int   st;

  float y;
  float i;
  float q;

  int   r;
  int   g;
  int   b;

  int   index;

  int   target_hue;
  int   tint_hue;

  int   delta;

  /* allocate palette data */
  /*G_palette_data = malloc(sizeof(GLubyte) * 3 * PALETTE_SIZE * PALETTE_SIZE);*/
  G_palette_data = malloc(sizeof(unsigned char) * 3 * PALETTE_SIZE * PALETTE_SIZE);

  /* initialize palette data */
  for (k = 0; k < PALETTE_SIZE * PALETTE_SIZE; k++)
  {
    index = k;
    TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 0, 0, 0)
  }

  /* initialize base subpalette */
  for (m = 0; m < TEXTURE_MID_LEVEL; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      for (k = 0; k < TEXTURE_NUM_SHADES; k++)
      {
        index = (TEXTURE_MID_LEVEL + m) * PALETTE_SIZE + n * TEXTURE_NUM_SHADES + k;
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
      }
    }
  }

  /* generate base subpalette */
  for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
  {
    for (k = 0; k < TEXTURE_NUM_SHADES; k++)
    {
      index = TEXTURE_MID_LEVEL * PALETTE_SIZE + n * TEXTURE_NUM_SHADES + k;

      y = S_luma_table[k];

      if (n == 0)
      {
        i = 0.0f;
        q = 0.0f;
      }
      else
      {
        i = S_saturation_table[k] * cos(TWO_PI * ((n - 1) * TEXTURE_THETA_STEP) / 360.0f);
        q = S_saturation_table[k] * sin(TWO_PI * ((n - 1) * TEXTURE_THETA_STEP) / 360.0f);
      }

      TEXTURE_YIQ2RGB()
      TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, r, g, b)
    }
  }

  /* shadows */
  for (m = 1; m < TEXTURE_MID_LEVEL; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      memcpy( &G_palette_data[3 * (m * PALETTE_SIZE + TEXTURE_NUM_SHADES * n + (TEXTURE_MID_LEVEL - m))], 
              &G_palette_data[3 * (TEXTURE_MID_LEVEL * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
              3 * m);
    }
  }

  /* highlights */
  for (m = TEXTURE_MID_LEVEL + 1; m < TEXTURE_NUM_LEVELS; m++)
  {
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      memcpy( &G_palette_data[3 * (m * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
              &G_palette_data[3 * (TEXTURE_MID_LEVEL * PALETTE_SIZE + TEXTURE_NUM_SHADES * n + (m - TEXTURE_MID_LEVEL))], 
              3 * (TEXTURE_NUM_LEVELS - m));
    }
  }

  /* soft tints */
  for (st = 1; st < TEXTURE_NUM_BLENDS; st++)
  {
    /* initialize tint variables */
    if (st == 1)
      target_hue = 8;   /* purple */
    else if (st == 2)
      target_hue = 12;  /* blue */
    else if (st == 3)
      target_hue = 10;   /* purple + blue */
    else
      target_hue = 0;

    tint_hue = 0;
    delta = 0;

    /* tint colors */
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      /* determine tint hue */
      if (target_hue == 0)
        tint_hue = 0;
      else if ((n == 0) || (n == target_hue))
        tint_hue = target_hue;
      else
      {
        delta = n - target_hue;

        if (delta < 0)
          delta += TEXTURE_NUM_HUES;

        delta /= TEXTURE_TINT_INTERVAL;

        if (delta < (TEXTURE_TINT_HUES + 1) / 2)
          tint_hue = target_hue + delta;
        else
          tint_hue = target_hue - (TEXTURE_TINT_HUES - delta);

        if (tint_hue < 1)
          tint_hue += TEXTURE_NUM_HUES;
        else if (tint_hue >= TEXTURE_NUM_HUES + 1)
          tint_hue -= TEXTURE_NUM_HUES;
      }

      /* copy the appropriate hue from the base subpalette */
      for (m = 0; m < TEXTURE_NUM_LEVELS; m++)
      {
        memcpy( &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
                &G_palette_data[3 * (m * PALETTE_SIZE + TEXTURE_NUM_SHADES * tint_hue)], 
                3 * TEXTURE_NUM_SHADES);
      }
    }
  }

  /* rotations & hard tints */
  for (st = 0; st < TEXTURE_NUM_BLENDS; st++)
  {
    for (p = 1; p < TEXTURE_NUM_ROTATIONS; p++)
    {
      for (m = 0; m < TEXTURE_NUM_LEVELS; m++)
      {
        memcpy( &G_palette_data[3 * (((st * TEXTURE_NUM_SUBPALETTES + p) * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE)], 
                3 * TEXTURE_NUM_SHADES);

        memcpy( &G_palette_data[3 * (((st * TEXTURE_NUM_SUBPALETTES + p) * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + (1 + TEXTURE_ROTATE_RIGHT(p)) * TEXTURE_NUM_SHADES)], 
                3 * TEXTURE_ROTATE_LEFT(p) * TEXTURE_NUM_SHADES);

        memcpy( &G_palette_data[3 * (((st * TEXTURE_NUM_SUBPALETTES + p) * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + (1 + TEXTURE_ROTATE_LEFT(p)) * TEXTURE_NUM_SHADES)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES)], 
                3 * TEXTURE_ROTATE_RIGHT(p) * TEXTURE_NUM_SHADES);
      }
    }
  }

  #undef TEXTURE_NUM_HUES
  #undef TEXTURE_THETA_STEP

  #undef TEXTURE_NUM_GRADIENTS
  #undef TEXTURE_NUM_SHADES

  #undef TEXTURE_NUM_LEVELS
  #undef TEXTURE_MID_LEVEL

  #undef PALETTE_SIZE

  #undef TEXTURE_NUM_ROTATIONS
  #undef TEXTURE_NUM_HARD_TINTS
  #undef TEXTURE_NUM_SOFT_TINTS

  #undef TEXTURE_NUM_SUBPALETTES
  #undef TEXTURE_NUM_BLENDS 

  #undef TEXTURE_TINT_HUES
  #undef TEXTURE_TINT_INTERVAL

  return 0;
}

#if 0
/*******************************************************************************
** generate_palette_composite_08_2x_alt()
*******************************************************************************/
short int generate_palette_composite_08_2x_alt()
{
  #define TEXTURE_NUM_HUES      24
  #define TEXTURE_THETA_STEP    (360 / TEXTURE_NUM_HUES)

  #define TEXTURE_NUM_GRADIENTS (TEXTURE_NUM_HUES + 1)
  #define TEXTURE_NUM_SHADES    8

  #define TEXTURE_NUM_LEVELS    (2 * TEXTURE_NUM_SHADES)
  #define TEXTURE_MID_LEVEL     (TEXTURE_NUM_LEVELS / 2)

  #define PALETTE_SIZE          256

  #define TEXTURE_NUM_ROTATIONS     8
  #define TEXTURE_NUM_HARD_TINTS    0
  #define TEXTURE_NUM_SOFT_TINTS    1

  #define TEXTURE_NUM_SUBPALETTES   (TEXTURE_NUM_ROTATIONS + TEXTURE_NUM_HARD_TINTS)
  #define TEXTURE_NUM_BLENDS        (TEXTURE_NUM_SOFT_TINTS + 1)

  #define TEXTURE_TINT_HUES         5
  #define TEXTURE_TINT_HUE_INTERVAL (360 / (TEXTURE_TINT_HUES + 1))
  #define TEXTURE_TINT_SAT_INTERVAL (360 / TEXTURE_NUM_SHADES)

  #define TEXTURE_ROTATE_RIGHT(amount)                                   \
    (amount * (TEXTURE_NUM_HUES / TEXTURE_NUM_ROTATIONS))

  #define TEXTURE_ROTATE_LEFT(amount)                                    \
    ((TEXTURE_NUM_ROTATIONS - amount) * (TEXTURE_NUM_HUES / TEXTURE_NUM_ROTATIONS))

  int   m;
  int   n;
  int   k;

  int   p;
  int   st;

  float y;
  float i;
  float q;

  int   r;
  int   g;
  int   b;

  int   index;

  int   target_theta;
  int   delta;

  int   tint_theta;
  int   tint_shift;

  /* allocate palette data */
  /*G_palette_data = malloc(sizeof(GLubyte) * 3 * PALETTE_SIZE * PALETTE_SIZE);*/
  G_palette_data = malloc(sizeof(unsigned char) * 3 * PALETTE_SIZE * PALETTE_SIZE);

  /* initialize palette data */
  for (k = 0; k < PALETTE_SIZE * PALETTE_SIZE; k++)
  {
    index = k;
    TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 0, 0, 0)
  }

  /* initialize base subpalettes */
  for (st = 0; st < TEXTURE_NUM_BLENDS; st++)
  {
    for (m = 0; m < TEXTURE_MID_LEVEL; m++)
    {
      for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
      {
        for (k = 0; k < TEXTURE_NUM_SHADES; k++)
        {
          index = (st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + TEXTURE_MID_LEVEL + m) * PALETTE_SIZE + n * TEXTURE_NUM_SHADES + k;
          TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
        }
      }
    }
  }

  /* generate base subpalettes */
  for (st = 0; st < TEXTURE_NUM_BLENDS; st++)
  {
    /* initialize tint variables */
    if (st == 1)
      target_theta = 11 * TEXTURE_THETA_STEP; /* blue */
    else
      target_theta = 0;

    delta = 0;

    tint_theta = 0;
    tint_shift = 1;

    /* generate subpalette */
    for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
    {
      for (k = 0; k < TEXTURE_NUM_SHADES; k++)
      {
        index = (st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + TEXTURE_MID_LEVEL) * PALETTE_SIZE + n * TEXTURE_NUM_SHADES + k;

        /* no tint */
        if (st == 0)
        {
          if (n == 0)
          {
            y = S_luma_table[k];
            i = 0.0f;
            q = 0.0f;
          }
          else
          {
            y = S_luma_table[k];
            i = S_saturation_table[k] * cos(TWO_PI * ((n - 1) * TEXTURE_THETA_STEP) / 360.0f);
            q = S_saturation_table[k] * sin(TWO_PI * ((n - 1) * TEXTURE_THETA_STEP) / 360.0f);
          }
        }
        /* soft tint */
        else
        {
          if (n == 0)
          {
            tint_theta = target_theta;
            tint_shift = (TEXTURE_NUM_SHADES / 2) / 2;
          }
          else
          {
            /* compute delta (difference between this hue's theta and the target theta) */
            if ((n - 1) * TEXTURE_THETA_STEP >= target_theta)
              delta = ((n - 1) * TEXTURE_THETA_STEP) - target_theta;
            else
              delta = target_theta - ((n - 1) * TEXTURE_THETA_STEP);

            if (delta > 180)
              delta = 360 - delta;

            if ((delta == 0) || (delta == 180))
            {
              tint_theta = target_theta;
              tint_shift = 0;
            }
            else
            {
              /* set tint theta and shift */
              if ((n - 1) * TEXTURE_THETA_STEP >= target_theta)
                tint_theta = target_theta + (delta / TEXTURE_TINT_HUE_INTERVAL) * TEXTURE_THETA_STEP;
              else
                tint_theta = target_theta - (delta / TEXTURE_TINT_HUE_INTERVAL) * TEXTURE_THETA_STEP;

              if (tint_theta < 0)
                tint_theta += 360;

              if (tint_theta >= 360)
                tint_theta -= 360;

              tint_shift = delta / TEXTURE_TINT_SAT_INTERVAL;
            }
          }

          y = S_luma_table[k];

          if (delta == 180)
          {
            i = 0.0f;
            q = 0.0f;
          }
          else if (k < TEXTURE_NUM_SHADES / 2)
          {
            if (k < tint_shift)
            {
              i = 0.0f;
              q = 0.0f;
            }
            else
            {
              i = S_saturation_table[k - tint_shift] * cos(TWO_PI * tint_theta / 360.0f);
              q = S_saturation_table[k - tint_shift] * sin(TWO_PI * tint_theta / 360.0f);
            }
          }
          else
          {
            if (TEXTURE_NUM_SHADES - 1 - k < tint_shift)
            {
              i = 0.0f;
              q = 0.0f;
            }
            else
            {
              i = S_saturation_table[k + tint_shift] * cos(TWO_PI * tint_theta / 360.0f);
              q = S_saturation_table[k + tint_shift] * sin(TWO_PI * tint_theta / 360.0f);
            }
          }
        }

        TEXTURE_YIQ2RGB()
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, r, g, b)
      }
    }
  }

  /* generate shadow & highlight levels */
  for (st = 0; st < TEXTURE_NUM_BLENDS; st++)
  {
    /* shadows */
    for (m = 1; m < TEXTURE_MID_LEVEL; m++)
    {
      for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
      {
        memcpy( &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES * n + (TEXTURE_MID_LEVEL - m))], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + TEXTURE_MID_LEVEL) * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
                3 * m);
      }
    }

    /* highlights */
    for (m = TEXTURE_MID_LEVEL + 1; m < TEXTURE_NUM_LEVELS; m++)
    {
      for (n = 0; n < TEXTURE_NUM_GRADIENTS; n++)
      {
        memcpy( &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES * n)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + TEXTURE_MID_LEVEL) * PALETTE_SIZE + TEXTURE_NUM_SHADES * n + (m - TEXTURE_MID_LEVEL))], 
                3 * (TEXTURE_NUM_LEVELS - m));
      }
    }
  }

  /* rotations & hard tints */
  for (st = 0; st < TEXTURE_NUM_BLENDS; st++)
  {
    for (p = 1; p < TEXTURE_NUM_ROTATIONS; p++)
    {
      for (m = 0; m < TEXTURE_NUM_LEVELS; m++)
      {
        memcpy( &G_palette_data[3 * (((st * TEXTURE_NUM_SUBPALETTES + p) * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE)], 
                3 * TEXTURE_NUM_SHADES);

        memcpy( &G_palette_data[3 * (((st * TEXTURE_NUM_SUBPALETTES + p) * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + (1 + TEXTURE_ROTATE_RIGHT(p)) * TEXTURE_NUM_SHADES)], 
                3 * TEXTURE_ROTATE_LEFT(p) * TEXTURE_NUM_SHADES);

        memcpy( &G_palette_data[3 * (((st * TEXTURE_NUM_SUBPALETTES + p) * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + (1 + TEXTURE_ROTATE_LEFT(p)) * TEXTURE_NUM_SHADES)], 
                &G_palette_data[3 * ((st * TEXTURE_NUM_SUBPALETTES * TEXTURE_NUM_LEVELS + m) * PALETTE_SIZE + TEXTURE_NUM_SHADES)], 
                3 * TEXTURE_ROTATE_RIGHT(p) * TEXTURE_NUM_SHADES);
      }
    }
  }

  #undef TEXTURE_NUM_HUES
  #undef TEXTURE_THETA_STEP

  #undef TEXTURE_NUM_GRADIENTS
  #undef TEXTURE_NUM_SHADES

  #undef TEXTURE_NUM_LEVELS
  #undef TEXTURE_MID_LEVEL

  #undef PALETTE_SIZE

  #undef TEXTURE_NUM_ROTATIONS
  #undef TEXTURE_NUM_HARD_TINTS
  #undef TEXTURE_NUM_SOFT_TINTS

  #undef TEXTURE_NUM_SUBPALETTES
  #undef TEXTURE_NUM_BLENDS 

  #undef TEXTURE_TINT_HUES
  #undef TEXTURE_TINT_HUE_INTERVAL
  #undef TEXTURE_TINT_SAT_INTERVAL

  return 0;
}
#endif

/*******************************************************************************
** write_tga_file()
*******************************************************************************/
short int write_tga_file(char* filename)
{
  FILE* fp_out;

  int   m;
  int   n;

  unsigned char image_id_field_length;
  unsigned char color_map_type;
  unsigned char image_type;
  unsigned char image_descriptor;

  unsigned char color_map_specification[5];

  short int     x_origin;
  short int     y_origin;

  short int     image_size;

  unsigned char pixel_bpp;
  short int     pixel_num_bytes;

  unsigned char output_buffer[3];

  /* make sure filename is valid */
  if (filename == NULL)
  {
    printf("Write TGA file failed: No filename specified.\n");
    return 1;
  }

  /* initialize parameters */
  image_id_field_length = 0;
  color_map_type = 0;
  image_type = 2;
  image_descriptor = 0x20;

  color_map_specification[0] = 0;
  color_map_specification[1] = 0;
  color_map_specification[2] = 0;
  color_map_specification[3] = 0;
  color_map_specification[4] = 0;

  x_origin = 0;
  y_origin = 0;

  if (G_palette_size == 64)
    image_size = 64;
  else if (G_palette_size == 256)
    image_size = 256;
  else if (G_palette_size == 1024)
    image_size = 1024;
  else
  {
    printf("Write TGA file failed: Unknown source specified.\n");
    return 1;
  }

  pixel_bpp = 24;
  pixel_num_bytes = 3;

  output_buffer[0] = 0;
  output_buffer[1] = 0;
  output_buffer[2] = 0;

  /* open file */
  fp_out = fopen(filename, "wb");

  /* if file did not open, return error */
  if (fp_out == NULL)
  {
    printf("Write TGA file failed: Unable to open output file.\n");
    return 1;
  }

  /* write image id field length */
  if (fwrite(&image_id_field_length, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write colormap type */
  if (fwrite(&color_map_type, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write image type */
  if (fwrite(&image_type, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write colormap specification */
  if (fwrite(color_map_specification, 1, 5, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write x origin */
  if (fwrite(&x_origin, 2, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write y origin */
  if (fwrite(&y_origin, 2, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write image width */
  if (fwrite(&image_size, 2, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write image height */
  if (fwrite(&image_size, 2, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write pixel bpp */
  if (fwrite(&pixel_bpp, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write image descriptor */
  if (fwrite(&image_descriptor, 1, 1, fp_out) < 1)
  {
    fclose(fp_out);
    return 1;
  }

  /* write palette data */
  if ((G_source == SOURCE_APPROX_NES) || 
      (G_source == SOURCE_APPROX_NES_ROTATED))
  {
    for (n = 0; n < G_palette_size; n++)
    {
      for (m = 0; m < G_palette_size; m++)
      {
        /* if this is a transparency color, just write out magenta */
        if (G_palette_data[4 * ((n * G_palette_size) + m) + 3] == 0)
        {
          output_buffer[2] = 255;
          output_buffer[1] = 0;
          output_buffer[0] = 255;
        }
        /* otherwise, write out this color */
        else
        {
          output_buffer[2] = G_palette_data[4 * ((n * G_palette_size) + m) + 0];
          output_buffer[1] = G_palette_data[4 * ((n * G_palette_size) + m) + 1];
          output_buffer[0] = G_palette_data[4 * ((n * G_palette_size) + m) + 2];
        }

        if (fwrite(output_buffer, 1, 3, fp_out) < 1)
        {
          fclose(fp_out);
          return 1;
        }
      }
    }
  }
  else
  {
    for (n = 0; n < G_palette_size; n++)
    {
      for (m = 0; m < G_palette_size; m++)
      {
        output_buffer[2] = G_palette_data[3 * ((n * G_palette_size) + m) + 0];
        output_buffer[1] = G_palette_data[3 * ((n * G_palette_size) + m) + 1];
        output_buffer[0] = G_palette_data[3 * ((n * G_palette_size) + m) + 2];

        if (fwrite(output_buffer, 1, 3, fp_out) < 1)
        {
          fclose(fp_out);
          return 1;
        }
      }
    }

  }

  /* close file */
  fclose(fp_out);

  return 0;
}

/*******************************************************************************
** main()
*******************************************************************************/
int main(int argc, char *argv[])
{
  int   i;

  char  output_tga_filename[64];

  /* initialization */
  G_palette_data = NULL;

  G_source = SOURCE_APPROX_NES;
  G_palette_size = 64;

  output_tga_filename[0] = '\0';

  /* generate voltage tables */
  generate_voltage_tables();

  /* read command line arguments */
  i = 1;

  while (i < argc)
  {
    /* source */
    if (!strcmp(argv[i], "-s"))
    {
      i++;

      if (i >= argc)
      {
        printf("Insufficient number of arguments. ");
        printf("Expected source name. Exiting...\n");
        return 0;
      }

      if (!strcmp("approx_nes", argv[i]))
        G_source = SOURCE_APPROX_NES;
      else if (!strcmp("approx_nes_rotated", argv[i]))
        G_source = SOURCE_APPROX_NES_ROTATED;
      else if (!strcmp("composite_16_1x", argv[i]))
        G_source = SOURCE_COMPOSITE_16_1X;
      else if (!strcmp("composite_16_1x_rotated", argv[i]))
        G_source = SOURCE_COMPOSITE_16_1X_ROTATED;
      else if (!strcmp("composite_08_2x", argv[i]))
        G_source = SOURCE_COMPOSITE_08_2X;
      else if (!strcmp("composite_32_2x", argv[i]))
        G_source = SOURCE_COMPOSITE_32_2X;
      else if (!strcmp("composite_06_0p75x", argv[i]))
        G_source = SOURCE_COMPOSITE_06_0p75X;
      else if (!strcmp("composite_24_0p75x", argv[i]))
        G_source = SOURCE_COMPOSITE_24_0p75X;
      else if (!strcmp("composite_12_1p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_12_1p50X;
      else if (!strcmp("composite_48_1p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_48_1p50X;
      else if (!strcmp("composite_08_2p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_08_2p50X;
      else if (!strcmp("composite_32_2p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_32_2p50X;
      else if (!strcmp("ega_extended_08", argv[i]))
        G_source = SOURCE_EGA_EXTENDED_08;
      else if (!strcmp("ega_extended_32", argv[i]))
        G_source = SOURCE_EGA_EXTENDED_32;
      else if (!strcmp("cga0_extended_16", argv[i]))
        G_source = SOURCE_CGA0_EXTENDED_16;
      else if (!strcmp("cga1_extended_16", argv[i]))
        G_source = SOURCE_CGA1_EXTENDED_16;
      else
      {
        printf("Unknown source %s. Exiting...\n", argv[i]);
        return 0;
      }

      i++;
    }
    else
    {
      printf("Unknown command line argument %s. Exiting...\n", argv[i]);
      return 0;
    }
  }

  /* set output filename */
  if (G_source == SOURCE_APPROX_NES)
    strncpy(output_tga_filename, "approx_nes.tga", 32);
  else if (G_source == SOURCE_APPROX_NES_ROTATED)
    strncpy(output_tga_filename, "approx_nes_rotated.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_16_1X)
    strncpy(output_tga_filename, "composite_16_1x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)
    strncpy(output_tga_filename, "composite_16_1x_rotated.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_08_2X)
    strncpy(output_tga_filename, "composite_08_2x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_32_2X)
    strncpy(output_tga_filename, "composite_32_2x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_06_0p75X)
    strncpy(output_tga_filename, "composite_06_0p75x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_24_0p75X)
    strncpy(output_tga_filename, "composite_24_0p75x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_12_1p50X)
    strncpy(output_tga_filename, "composite_12_1p50x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_48_1p50X)
    strncpy(output_tga_filename, "composite_48_1p50x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_08_2p50X)
    strncpy(output_tga_filename, "composite_08_2p50x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_32_2p50X)
    strncpy(output_tga_filename, "composite_32_2p50x.tga", 32);
  else if (G_source == SOURCE_EGA_EXTENDED_08)
    strncpy(output_tga_filename, "ega_extended_08.tga", 32);
  else if (G_source == SOURCE_EGA_EXTENDED_32)
    strncpy(output_tga_filename, "ega_extended_32.tga", 32);
  else if (G_source == SOURCE_CGA0_EXTENDED_16)
    strncpy(output_tga_filename, "cga0_extended_16.tga", 32);
  else if (G_source == SOURCE_CGA1_EXTENDED_16)
    strncpy(output_tga_filename, "cga1_extended_16.tga", 32);
  else
  {
    printf("Unable to set output filename: Unknown source. Exiting...\n");
    return 0;
  }

  /* set palette size */
  if ((G_source == SOURCE_APPROX_NES)         || 
      (G_source == SOURCE_APPROX_NES_ROTATED) || 
      (G_source == SOURCE_COMPOSITE_06_0p75X) || 
      (G_source == SOURCE_EGA_EXTENDED_08)    || 
      (G_source == SOURCE_CGA0_EXTENDED_16)   || 
      (G_source == SOURCE_CGA1_EXTENDED_16))
  {
    G_palette_size = 64;
  }
  else if ( (G_source == SOURCE_COMPOSITE_16_1X)          || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_08_2X)          || 
            (G_source == SOURCE_COMPOSITE_24_0p75X)       || 
            (G_source == SOURCE_COMPOSITE_12_1p50X)       || 
            (G_source == SOURCE_COMPOSITE_08_2p50X)       || 
            (G_source == SOURCE_EGA_EXTENDED_32))
  {
    G_palette_size = 256;
  }
  else if ( (G_source == SOURCE_COMPOSITE_32_2X)    || 
            (G_source == SOURCE_COMPOSITE_48_1p50X) || 
            (G_source == SOURCE_COMPOSITE_32_2p50X))
  {
    G_palette_size = 1024;
  }
  else
  {
    printf("Unable to set palette size: Unknown source. Exiting...\n");
    return 0;
  }

  /* set voltage table pointers */
  if (set_voltage_table_pointers())
  {
    printf("Error setting voltage table pointers. Exiting...\n");
    return 0;
  }

  /* generate palette */
  if ((G_source == SOURCE_APPROX_NES) || 
      (G_source == SOURCE_APPROX_NES_ROTATED))
  {
    if (generate_palette_approx_nes())
    {
      printf("Error generating texture. Exiting...\n");
      return 0;
    }
  }
  else if ( (G_source == SOURCE_COMPOSITE_16_1X) || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED))
  {
    if (generate_palette_composite_16_1x())
    {
      printf("Error generating texture. Exiting...\n");
      return 0;
    }
  }
  else if (G_source == SOURCE_COMPOSITE_08_2X)
  {
    if (generate_palette_composite_08_2x())
    {
      printf("Error generating texture. Exiting...\n");
      return 0;
    }
  }
  else if (G_source == SOURCE_COMPOSITE_32_2X)
  {
    if (generate_palette_composite_32_2x())
    {
      printf("Error generating texture. Exiting...\n");
      return 0;
    }
  }
  else
  {
    printf("Unable to generate texture: Invalid source. Exiting...\n");
    return 0;
  }

  /* write output tga file */
  write_tga_file(output_tga_filename);

  /* clear palette data */
  if (G_palette_data != NULL)
  {
    free(G_palette_data);
    G_palette_data = NULL;
  }

  return 0;
}
