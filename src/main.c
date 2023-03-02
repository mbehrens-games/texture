/*******************************************************************************
** TEXTURE (palette texture generation) - Michael Behrens 2021-2023
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
  SOURCE_COMPOSITE_04_1X,
  SOURCE_COMPOSITE_04_1X_ROTATED,
  SOURCE_COMPOSITE_08_2X,
  SOURCE_COMPOSITE_08_2p50X,
  SOURCE_COMPOSITE_16_1X,
  SOURCE_COMPOSITE_16_1X_ROTATED,
  SOURCE_COMPOSITE_16_3X,
  SOURCE_COMPOSITE_16_3X_ROTATED,
  SOURCE_COMPOSITE_32_2X,
  SOURCE_COMPOSITE_32_2p50X
};

/* the table step is 1 / (n + 2), where */
/* n is the number of colors per hue    */
#define COMPOSITE_04_TABLE_STEP 0.166666666666667f  /* 1/6  */
#define COMPOSITE_08_TABLE_STEP 0.1f                /* 1/10 */
#define COMPOSITE_16_TABLE_STEP 0.055555555555556f  /* 1/18 */
#define COMPOSITE_32_TABLE_STEP 0.029411764705882f  /* 1/34 */

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

/* note that if we used a "composite 04" table, with  */
/* the table step being 1/(4 + 2) = 1/6, we would end */
/* up with another approximation of these values!     */
float S_approx_nes_p_p[4] = {0.4f, 0.7f,  0.7f,   0.3f};
float S_approx_nes_lum[4] = {0.2f, 0.35f, 0.65f,  0.85f};
float S_approx_nes_sat[4] = {0.2f, 0.35f, 0.35f,  0.15f};

float S_composite_04_lum[4];
float S_composite_04_sat[4];

float S_composite_08_lum[8];
float S_composite_08_sat[8];

float S_composite_16_lum[16];
float S_composite_16_sat[16];

float S_composite_32_lum[32];
float S_composite_32_sat[32];

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

  /* composite 04 tables */
  for (k = 0; k < 2; k++)
  {
    S_composite_04_lum[k] = (k + 1) * COMPOSITE_04_TABLE_STEP;
    S_composite_04_lum[3 - k] = 1.0f - S_composite_04_lum[k];

    S_composite_04_sat[k] = S_composite_04_lum[k];
    S_composite_04_sat[3 - k] = S_composite_04_sat[k];
  }

  /* composite 08 tables */
  for (k = 0; k < 4; k++)
  {
    S_composite_08_lum[k] = (k + 1) * COMPOSITE_08_TABLE_STEP;
    S_composite_08_lum[7 - k] = 1.0f - S_composite_08_lum[k];

    S_composite_08_sat[k] = S_composite_08_lum[k];
    S_composite_08_sat[7 - k] = S_composite_08_sat[k];
  }

  /* composite 16 tables */
  for (k = 0; k < 8; k++)
  {
    S_composite_16_lum[k] = (k + 1) * COMPOSITE_16_TABLE_STEP;
    S_composite_16_lum[15 - k] = 1.0f - S_composite_16_lum[k];

    S_composite_16_sat[k] = S_composite_16_lum[k];
    S_composite_16_sat[15 - k] = S_composite_16_sat[k];
  }

  /* composite 32 tables */
  for (k = 0; k < 16; k++)
  {
    S_composite_32_lum[k] = (k + 1) * COMPOSITE_32_TABLE_STEP;
    S_composite_32_lum[31 - k] = 1.0f - S_composite_32_lum[k];

    S_composite_32_sat[k] = S_composite_32_lum[k];
    S_composite_32_sat[31 - k] = S_composite_32_sat[k];
  }

  return 0;
}

/*******************************************************************************
** set_voltage_table_pointers()
*******************************************************************************/
short int set_voltage_table_pointers()
{
  if ((G_source == SOURCE_APPROX_NES) || 
      (G_source == SOURCE_APPROX_NES_ROTATED))
  {
    S_luma_table = S_approx_nes_lum;
    S_saturation_table = S_approx_nes_sat;
    S_table_length = 4;
  }
  else if ( (G_source == SOURCE_COMPOSITE_04_1X) || 
            (G_source == SOURCE_COMPOSITE_04_1X_ROTATED))
  {
    S_luma_table = S_composite_04_lum;
    S_saturation_table = S_composite_04_sat;
    S_table_length = 4;
  }
  else if ( (G_source == SOURCE_COMPOSITE_08_2X) || 
            (G_source == SOURCE_COMPOSITE_08_2p50X))
  {
    S_luma_table = S_composite_08_lum;
    S_saturation_table = S_composite_08_sat;
    S_table_length = 8;
  }
  else if ( (G_source == SOURCE_COMPOSITE_16_1X)          || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_16_3X)          || 
            (G_source == SOURCE_COMPOSITE_16_3X_ROTATED))
  {
    S_luma_table = S_composite_16_lum;
    S_saturation_table = S_composite_16_sat;
    S_table_length = 16;
  }
  else if ( (G_source == SOURCE_COMPOSITE_32_2X) || 
            (G_source == SOURCE_COMPOSITE_32_2p50X))
  {
    S_luma_table = S_composite_32_lum;
    S_saturation_table = S_composite_32_sat;
    S_table_length = 32;
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
** generate_palette_64_color()
*******************************************************************************/
short int generate_palette_64_color()
{
  #define PALETTE_SIZE  64

  #define TEXTURE_LEVELS_PER_PALETTE  (PALETTE_SIZE / 8)
  #define TEXTURE_BASE_LEVEL          (TEXTURE_LEVELS_PER_PALETTE / 2)

  int   num_hues;
  int   hue_step;

  int   num_gradients;
  int   starting_theta;

  int   num_shades;
  int   shade_step;
  int   rotation_step;

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

  int   source_base_index;
  int   dest_base_index;

  /* initialize number of hues and shades */
  if (G_source == SOURCE_COMPOSITE_04_1X)
  {
    num_hues = 12;
    starting_theta = 0;
    num_shades = 4;
  }
  else if (G_source == SOURCE_COMPOSITE_04_1X_ROTATED)
  {
    num_hues = 12;
    starting_theta = 15;
    num_shades = 4;
  }
  else
  {
    num_hues = 12;
    starting_theta = 0;
    num_shades = 4;
  }

  /* compute secondary variables */
  hue_step = 360 / num_hues;
  num_gradients = num_hues + 1;

  shade_step = num_shades / TEXTURE_BASE_LEVEL;
  rotation_step = num_hues / 6;

  /* allocate palette data */
  /*G_palette_data = malloc(sizeof(GLubyte) * 3 * PALETTE_SIZE * PALETTE_SIZE);*/
  G_palette_data = malloc(sizeof(unsigned char) * 3 * PALETTE_SIZE * PALETTE_SIZE);

  /* initialize palette data */
  for (k = 0; k < PALETTE_SIZE * PALETTE_SIZE; k++)
  {
    TEXTURE_SET_PALETTE_DATA_AT_INDEX(k, 0, 0, 0)
  }

  /* initialize palette 0 */
  for (m = TEXTURE_BASE_LEVEL; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      for (k = 0; k < num_shades; k++)
      {
        index = m * PALETTE_SIZE + n * num_shades + k;
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
      }
    }
  }

  /* initialize palette 7 (inverted greyscale) */
  for (m = 0; m < TEXTURE_BASE_LEVEL; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      for (k = 0; k < num_shades; k++)
      {
        index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE + n * num_shades + k;
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
      }
    }
  }

  /* generate palette 0 */
  for (n = 0; n < num_gradients; n++)
  {
    for (k = 0; k < num_shades; k++)
    {
      index = TEXTURE_BASE_LEVEL * PALETTE_SIZE + n * num_shades + k;

      y = S_luma_table[k];

      if (n == 0)
      {
        i = 0.0f;
        q = 0.0f;
      }
      else
      {
        i = S_saturation_table[k] * cos(TWO_PI * ((n - 1) * hue_step + starting_theta) / 360.0f);
        q = S_saturation_table[k] * sin(TWO_PI * ((n - 1) * hue_step + starting_theta) / 360.0f);
      }

      TEXTURE_YIQ2RGB()
      TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, r, g, b)
    }
  }

  /* shadows for palette 0 */
  for (m = 1; m < TEXTURE_BASE_LEVEL; m++)
  {
    source_base_index = TEXTURE_BASE_LEVEL * PALETTE_SIZE;
    dest_base_index = m * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n + (TEXTURE_BASE_LEVEL - m) * shade_step)], 
              &G_palette_data[3 * (source_base_index + num_shades * n)], 
              3 * m * shade_step);
    }
  }

  /* highlights for palette 0 */
  for (m = TEXTURE_BASE_LEVEL + 1; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    source_base_index = TEXTURE_BASE_LEVEL * PALETTE_SIZE;
    dest_base_index = m * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n)], 
              &G_palette_data[3 * (source_base_index + num_shades * n + (m - TEXTURE_BASE_LEVEL) * shade_step)], 
              3 * (TEXTURE_LEVELS_PER_PALETTE - m) * shade_step);
    }
  }

  /* compute palette 7 (inverted greyscale) */
  source_base_index = TEXTURE_BASE_LEVEL * PALETTE_SIZE;
  dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + TEXTURE_BASE_LEVEL) * PALETTE_SIZE;

  for (k = 0; k < num_shades; k++)
  {
    memcpy( &G_palette_data[3 * (dest_base_index + k)], 
            &G_palette_data[3 * (source_base_index + (num_shades - 1 - k))], 
            3);
  }

  for (n = 1; n < num_gradients; n++)
  {
    memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n)], 
            &G_palette_data[3 * (dest_base_index)], 
            3 * num_shades);
  }

  /* shadows for palette 7 */
  for (m = 1; m < TEXTURE_BASE_LEVEL; m++)
  {
    source_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + TEXTURE_BASE_LEVEL) * PALETTE_SIZE;
    dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n + (TEXTURE_BASE_LEVEL - m) * shade_step)], 
              &G_palette_data[3 * (source_base_index + num_shades * n)], 
              3 * m * shade_step);
    }
  }

  /* highlights for palette 7 */
  for (m = TEXTURE_BASE_LEVEL + 1; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    source_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + TEXTURE_BASE_LEVEL) * PALETTE_SIZE;
    dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n)], 
              &G_palette_data[3 * (source_base_index + num_shades * n + (m - TEXTURE_BASE_LEVEL) * shade_step)], 
              3 * (TEXTURE_LEVELS_PER_PALETTE - m) * shade_step);
    }
  }

  /* palettes 1-5: rotations */
  for (p = 1; p < 6; p++)
  {
    for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((p * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      /* greys */
      memcpy( &G_palette_data[3 * dest_base_index], 
              &G_palette_data[3 * source_base_index], 
              3 * num_shades);

      /* rotated hues */
      memcpy( &G_palette_data[3 * (dest_base_index + 1 * num_shades)], 
              &G_palette_data[3 * (source_base_index + (1 + p * rotation_step) * num_shades)], 
              3 * (6 - p) * rotation_step * num_shades);

      memcpy( &G_palette_data[3 * (dest_base_index + (1 + (6 - p) * rotation_step) * num_shades)], 
              &G_palette_data[3 * (source_base_index + num_shades)], 
              3 * p * rotation_step * num_shades);
    }
  }

  /* palette 6: greyscale */
  for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((6 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * (dest_base_index + (n * num_shades))], 
              &G_palette_data[3 * (source_base_index + (0 * num_shades))], 
              3 * num_shades);
    }
  }

  #undef PALETTE_SIZE

  #undef TEXTURE_LEVELS_PER_PALETTE
  #undef TEXTURE_BASE_LEVEL

  return 0;
}

/*******************************************************************************
** generate_palette_256_color()
*******************************************************************************/
short int generate_palette_256_color()
{
  #define PALETTE_SIZE  256

  #define TEXTURE_LEVELS_PER_PALETTE  (PALETTE_SIZE / 16)
  #define TEXTURE_BASE_LEVEL          (TEXTURE_LEVELS_PER_PALETTE / 2)

  int   num_hues;
  int   hue_step;

  int   num_gradients;
  int   starting_theta;

  int   num_shades;
  int   shade_step;
  int   rotation_step;

  int   fixed_hues_left;
  int   fixed_hues_right;

  int   tint_hue_red;
  int   tint_hue_blue;

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

  int   source_base_index;
  int   dest_base_index;

  /* initialize number of hues and shades */
  if (G_source == SOURCE_COMPOSITE_08_2X)
  {
    num_hues = 24;
    starting_theta = 0;
    num_shades = 8;

    fixed_hues_left = 1;
    fixed_hues_right = 2;

    tint_hue_red = 3;
    tint_hue_blue = 11;
  }
  else if (G_source == SOURCE_COMPOSITE_08_2p50X)
  {
    num_hues = 30;
    starting_theta = 0;
    num_shades = 8;

    fixed_hues_left = 1;
    fixed_hues_right = 3;

    tint_hue_red = 3;
    tint_hue_blue = 13;
  }
  else if (G_source == SOURCE_COMPOSITE_16_1X)
  {
    num_hues = 12;
    starting_theta = 0;
    num_shades = 16;

    fixed_hues_left = 1;
    fixed_hues_right = 1;

    tint_hue_red = 2;
    tint_hue_blue = 6;
  }
  else if (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)
  {
    num_hues = 12;
    starting_theta = 15;
    num_shades = 16;

    fixed_hues_left = 1;
    fixed_hues_right = 1;

    tint_hue_red = 1;
    tint_hue_blue = 5;
  }
  else
  {
    num_hues = 24;
    starting_theta = 0;
    num_shades = 8;

    fixed_hues_left = 1;
    fixed_hues_right = 2;

    tint_hue_red = 3;
    tint_hue_blue = 11;
  }

  /* compute secondary variables */
  hue_step = 360 / num_hues;
  num_gradients = num_hues + 1;

  shade_step = num_shades / TEXTURE_BASE_LEVEL;
  rotation_step = num_hues / 6;

  /* allocate palette data */
  /*G_palette_data = malloc(sizeof(GLubyte) * 3 * PALETTE_SIZE * PALETTE_SIZE);*/
  G_palette_data = malloc(sizeof(unsigned char) * 3 * PALETTE_SIZE * PALETTE_SIZE);

  /* initialize palette data */
  for (k = 0; k < PALETTE_SIZE * PALETTE_SIZE; k++)
  {
    TEXTURE_SET_PALETTE_DATA_AT_INDEX(k, 0, 0, 0)
  }

  /* initialize palette 0 */
  for (m = TEXTURE_BASE_LEVEL; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      for (k = 0; k < num_shades; k++)
      {
        index = m * PALETTE_SIZE + n * num_shades + k;
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
      }
    }
  }

  /* initialize palette 7 (inverted greyscale) */
  for (m = 0; m < TEXTURE_BASE_LEVEL; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      for (k = 0; k < num_shades; k++)
      {
        index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE + n * num_shades + k;
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
      }
    }
  }

  /* generate palette 0 */
  for (n = 0; n < num_gradients; n++)
  {
    for (k = 0; k < num_shades; k++)
    {
      index = TEXTURE_BASE_LEVEL * PALETTE_SIZE + n * num_shades + k;

      y = S_luma_table[k];

      if (n == 0)
      {
        i = 0.0f;
        q = 0.0f;
      }
      else
      {
        i = S_saturation_table[k] * cos(TWO_PI * ((n - 1) * hue_step + starting_theta) / 360.0f);
        q = S_saturation_table[k] * sin(TWO_PI * ((n - 1) * hue_step + starting_theta) / 360.0f);
      }

      TEXTURE_YIQ2RGB()
      TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, r, g, b)
    }
  }

  /* shadows for palette 0 */
  for (m = 1; m < TEXTURE_BASE_LEVEL; m++)
  {
    source_base_index = TEXTURE_BASE_LEVEL * PALETTE_SIZE;
    dest_base_index = m * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n + (TEXTURE_BASE_LEVEL - m) * shade_step)], 
              &G_palette_data[3 * (source_base_index + num_shades * n)], 
              3 * m * shade_step);
    }
  }

  /* highlights for palette 0 */
  for (m = TEXTURE_BASE_LEVEL + 1; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    source_base_index = TEXTURE_BASE_LEVEL * PALETTE_SIZE;
    dest_base_index = m * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n)], 
              &G_palette_data[3 * (source_base_index + num_shades * n + (m - TEXTURE_BASE_LEVEL) * shade_step)], 
              3 * (TEXTURE_LEVELS_PER_PALETTE - m) * shade_step);
    }
  }

  /* compute palette 7 (inverted greyscale) */
  source_base_index = TEXTURE_BASE_LEVEL * PALETTE_SIZE;
  dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + TEXTURE_BASE_LEVEL) * PALETTE_SIZE;

  for (k = 0; k < num_shades; k++)
  {
    memcpy( &G_palette_data[3 * (dest_base_index + k)], 
            &G_palette_data[3 * (source_base_index + (num_shades - 1 - k))], 
            3);
  }

  for (n = 1; n < num_gradients; n++)
  {
    memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n)], 
            &G_palette_data[3 * (dest_base_index)], 
            3 * num_shades);
  }

  /* shadows for palette 7 */
  for (m = 1; m < TEXTURE_BASE_LEVEL; m++)
  {
    source_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + TEXTURE_BASE_LEVEL) * PALETTE_SIZE;
    dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n + (TEXTURE_BASE_LEVEL - m) * shade_step)], 
              &G_palette_data[3 * (source_base_index + num_shades * n)], 
              3 * m * shade_step);
    }
  }

  /* highlights for palette 7 */
  for (m = TEXTURE_BASE_LEVEL + 1; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    source_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + TEXTURE_BASE_LEVEL) * PALETTE_SIZE;
    dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n)], 
              &G_palette_data[3 * (source_base_index + num_shades * n + (m - TEXTURE_BASE_LEVEL) * shade_step)], 
              3 * (TEXTURE_LEVELS_PER_PALETTE - m) * shade_step);
    }
  }

  /* palettes 1-5: rotations */
  for (p = 1; p < 6; p++)
  {
    for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((p * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      /* greys */
      memcpy( &G_palette_data[3 * dest_base_index], 
              &G_palette_data[3 * source_base_index], 
              3 * num_shades);

      /* rotated hues */
      memcpy( &G_palette_data[3 * (dest_base_index + 1 * num_shades)], 
              &G_palette_data[3 * (source_base_index + (1 + p * rotation_step) * num_shades)], 
              3 * (6 - p) * rotation_step * num_shades);

      memcpy( &G_palette_data[3 * (dest_base_index + (1 + (6 - p) * rotation_step) * num_shades)], 
              &G_palette_data[3 * (source_base_index + num_shades)], 
              3 * p * rotation_step * num_shades);
    }
  }

  /* palette 6: greyscale */
  for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((6 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * (dest_base_index + (n * num_shades))], 
              &G_palette_data[3 * (source_base_index + (0 * num_shades))], 
              3 * num_shades);
    }
  }

  /* palettes 8-12: alternate rotations (preserving flesh tones) */
  for (p = 1; p < 6; p++)
  {
    for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
    {
      /* copy non-rotated hues from palette 0 */
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = (((7 + p) * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      /* copying grey and the fixed hues on the left side */
      memcpy( &G_palette_data[3 * dest_base_index], 
              &G_palette_data[3 * source_base_index], 
              3 * num_shades * (1 + fixed_hues_left));

      /* copying the fixed hues on the right side */
      memcpy( &G_palette_data[3 * (dest_base_index + (1 + num_hues - fixed_hues_right) * num_shades)], 
              &G_palette_data[3 * (source_base_index + (1 + num_hues - fixed_hues_right) * num_shades)], 
              3 * num_shades * fixed_hues_right);

      /* copy rotated hues from the original rotated palette */
      source_base_index = ((p * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;
      dest_base_index = (((7 + p) * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * (dest_base_index + (1 + fixed_hues_left) * num_shades)], 
              &G_palette_data[3 * (source_base_index + (1 + fixed_hues_left) * num_shades)], 
              3 * num_shades * (num_hues - fixed_hues_left - fixed_hues_right));
    }
  }

  /* palette 13: alternate greyscale (preserving flesh tones) */
  for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    /* copy non-rotated hues from palette 0 */
    source_base_index = m * PALETTE_SIZE;
    dest_base_index = ((13 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    /* copying grey and the fixed hues on the left side */
    memcpy( &G_palette_data[3 * dest_base_index], 
            &G_palette_data[3 * source_base_index], 
            3 * num_shades * (1 + fixed_hues_left));

    /* copying the fixed hues on the right side */
    memcpy( &G_palette_data[3 * (dest_base_index + (1 + num_hues - fixed_hues_right) * num_shades)], 
            &G_palette_data[3 * (source_base_index + (1 + num_hues - fixed_hues_right) * num_shades)], 
            3 * num_shades * fixed_hues_right);

    /* copy greyscale hues from the original greyscale palette */
    source_base_index = ((6 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;
    dest_base_index = ((13 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    memcpy( &G_palette_data[3 * (dest_base_index + (1 + fixed_hues_left) * num_shades)], 
            &G_palette_data[3 * (source_base_index + (1 + fixed_hues_left) * num_shades)], 
            3 * num_shades * (num_hues - fixed_hues_left - fixed_hues_right));
  }

  /* palettes 14: red tint */
  for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((14 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * (dest_base_index + n * num_shades)], 
              &G_palette_data[3 * (source_base_index + tint_hue_red * num_shades)], 
              3 * num_shades);
    }
  }

  /* palettes 15: blue tint */
  for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((15 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * (dest_base_index + n * num_shades)], 
              &G_palette_data[3 * (source_base_index + tint_hue_blue * num_shades)], 
              3 * num_shades);
    }
  }

  #undef PALETTE_SIZE

  #undef TEXTURE_LEVELS_PER_PALETTE
  #undef TEXTURE_BASE_LEVEL

  return 0;
}

/*******************************************************************************
** generate_palette_1024_color()
*******************************************************************************/
short int generate_palette_1024_color()
{
  #define PALETTE_SIZE  1024

  #define TEXTURE_LEVELS_PER_PALETTE  (PALETTE_SIZE / 16)
  #define TEXTURE_BASE_LEVEL          (TEXTURE_LEVELS_PER_PALETTE / 2)

  int   num_hues;
  int   hue_step;

  int   num_gradients;
  int   starting_theta;

  int   num_shades;
  int   row_step;
  int   rotation_step;

  int   fixed_hues_left;
  int   fixed_hues_right;

  int   tint_hue_red;
  int   tint_hue_blue;

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

  int   source_base_index;
  int   dest_base_index;

  /* initialize number of hues and shades */
  if (G_source == SOURCE_COMPOSITE_32_2X)
  {
    num_hues = 24;
    starting_theta = 0;
    num_shades = 32;

    fixed_hues_left = 1;
    fixed_hues_right = 2;

    tint_hue_red = 3;
    tint_hue_blue = 11;
  }
  else if (G_source == SOURCE_COMPOSITE_32_2p50X)
  {
    num_hues = 30;
    starting_theta = 0;
    num_shades = 32;

    fixed_hues_left = 1;
    fixed_hues_right = 3;

    tint_hue_red = 3;
    tint_hue_blue = 13;
  }
  else if (G_source == SOURCE_COMPOSITE_16_3X)
  {
    num_hues = 36;
    starting_theta = 0;
    num_shades = 16;

    fixed_hues_left = 1;
    fixed_hues_right = 3;

    tint_hue_red = 4;
    tint_hue_blue = 16;
  }
  else if (G_source == SOURCE_COMPOSITE_16_3X_ROTATED)
  {
    num_hues = 36;
    starting_theta = 15;
    num_shades = 16;

    fixed_hues_left = 1;
    fixed_hues_right = 3;

    tint_hue_red = 1;
    tint_hue_blue = 13;
  }
  else
  {
    num_hues = 24;
    starting_theta = 0;
    num_shades = 32;

    fixed_hues_left = 1;
    fixed_hues_right = 2;

    tint_hue_red = 3;
    tint_hue_blue = 11;
  }

  /* compute secondary variables */
  hue_step = 360 / num_hues;
  num_gradients = num_hues + 1;

  row_step = TEXTURE_BASE_LEVEL / num_shades;
  rotation_step = num_hues / 6;

  /* allocate palette data */
  /*G_palette_data = malloc(sizeof(GLubyte) * 3 * PALETTE_SIZE * PALETTE_SIZE);*/
  G_palette_data = malloc(sizeof(unsigned char) * 3 * PALETTE_SIZE * PALETTE_SIZE);

  /* initialize palette data */
  for (k = 0; k < PALETTE_SIZE * PALETTE_SIZE; k++)
  {
    TEXTURE_SET_PALETTE_DATA_AT_INDEX(k, 0, 0, 0)
  }

  /* initialize palette 0 */
  for (m = TEXTURE_BASE_LEVEL; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      for (k = 0; k < num_shades; k++)
      {
        index = m * PALETTE_SIZE + n * num_shades + k;
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
      }
    }
  }

  /* initialize palette 7 (inverted greyscale) */
  for (m = 0; m < TEXTURE_BASE_LEVEL; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      for (k = 0; k < num_shades; k++)
      {
        index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE + n * num_shades + k;
        TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, 255, 255, 255)
      }
    }
  }

  /* generate palette 0 */
  for (n = 0; n < num_gradients; n++)
  {
    for (k = 0; k < num_shades; k++)
    {
      index = TEXTURE_BASE_LEVEL * PALETTE_SIZE + n * num_shades + k;

      y = S_luma_table[k];

      if (n == 0)
      {
        i = 0.0f;
        q = 0.0f;
      }
      else
      {
        i = S_saturation_table[k] * cos(TWO_PI * ((n - 1) * hue_step + starting_theta) / 360.0f);
        q = S_saturation_table[k] * sin(TWO_PI * ((n - 1) * hue_step + starting_theta) / 360.0f);
      }

      TEXTURE_YIQ2RGB()
      TEXTURE_SET_PALETTE_DATA_AT_INDEX(index, r, g, b)
    }
  }

  /* shadows for palette 0 */
  for (m = row_step; m < TEXTURE_BASE_LEVEL; m += row_step)
  {
    source_base_index = TEXTURE_BASE_LEVEL * PALETTE_SIZE;
    dest_base_index = m * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n + ((TEXTURE_BASE_LEVEL - m) / row_step))], 
              &G_palette_data[3 * (source_base_index + num_shades * n)], 
              3 * (m / row_step));
    }
  }

  /* highlights for palette 0 */
  for (m = TEXTURE_BASE_LEVEL + row_step; m < TEXTURE_LEVELS_PER_PALETTE; m += row_step)
  {
    source_base_index = TEXTURE_BASE_LEVEL * PALETTE_SIZE;
    dest_base_index = m * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n)], 
              &G_palette_data[3 * (source_base_index + num_shades * n + ((m - TEXTURE_BASE_LEVEL) / row_step))], 
              3 * ((TEXTURE_LEVELS_PER_PALETTE - m) / row_step));
    }
  }

  /* compute palette 7 (inverted greyscale) */
  source_base_index = TEXTURE_BASE_LEVEL * PALETTE_SIZE;
  dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + TEXTURE_BASE_LEVEL) * PALETTE_SIZE;

  for (k = 0; k < num_shades; k++)
  {
    memcpy( &G_palette_data[3 * (dest_base_index + k)], 
            &G_palette_data[3 * (source_base_index + (num_shades - 1 - k))], 
            3);
  }

  for (n = 1; n < num_gradients; n++)
  {
    memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n)], 
            &G_palette_data[3 * (dest_base_index)], 
            3 * num_shades);
  }

  /* shadows for palette 7 */
  for (m = row_step; m < TEXTURE_BASE_LEVEL; m += row_step)
  {
    source_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + TEXTURE_BASE_LEVEL) * PALETTE_SIZE;
    dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n + ((TEXTURE_BASE_LEVEL - m) / row_step))], 
              &G_palette_data[3 * (source_base_index + num_shades * n)], 
              3 * (m / row_step));
    }
  }

  /* highlights for palette 7 */
  for (m = TEXTURE_BASE_LEVEL + row_step; m < TEXTURE_LEVELS_PER_PALETTE; m += row_step)
  {
    source_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + TEXTURE_BASE_LEVEL) * PALETTE_SIZE;
    dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    for (n = 0; n < num_gradients; n++)
    {
      memcpy( &G_palette_data[3 * (dest_base_index + num_shades * n)], 
              &G_palette_data[3 * (source_base_index + num_shades * n + ((m - TEXTURE_BASE_LEVEL) / row_step))], 
              3 * ((TEXTURE_LEVELS_PER_PALETTE - m) / row_step));
    }
  }

  /* double rows for palettes 0 & 7 if necessary */
  if (row_step == 2)
  {
    for (m = row_step; m < TEXTURE_LEVELS_PER_PALETTE; m += row_step)
    {
      /* palette 0 */
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = (m + 1) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * dest_base_index], 
              &G_palette_data[3 * source_base_index], 
              3 * num_gradients * num_shades);

      /* palette 7 */
      source_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;
      dest_base_index = ((7 * TEXTURE_LEVELS_PER_PALETTE) + m + 1) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * dest_base_index], 
              &G_palette_data[3 * source_base_index], 
              3 * num_gradients * num_shades);
    }
  }

  /* palettes 1-5: rotations */
  for (p = 1; p < 6; p++)
  {
    for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((p * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      /* greys */
      memcpy( &G_palette_data[3 * dest_base_index], 
              &G_palette_data[3 * source_base_index], 
              3 * num_shades);

      /* rotated hues */
      memcpy( &G_palette_data[3 * (dest_base_index + 1 * num_shades)], 
              &G_palette_data[3 * (source_base_index + (1 + p * rotation_step) * num_shades)], 
              3 * (6 - p) * rotation_step * num_shades);

      memcpy( &G_palette_data[3 * (dest_base_index + (1 + (6 - p) * rotation_step) * num_shades)], 
              &G_palette_data[3 * (source_base_index + num_shades)], 
              3 * p * rotation_step * num_shades);
    }
  }

  /* palette 6: greyscale */
  for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((6 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * (dest_base_index + (n * num_shades))], 
              &G_palette_data[3 * (source_base_index + (0 * num_shades))], 
              3 * num_shades);
    }
  }

  /* palettes 8-12: alternate rotations (preserving flesh tones) */
  for (p = 1; p < 6; p++)
  {
    for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
    {
      /* copy non-rotated hues from palette 0 */
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = (((7 + p) * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      /* copying grey and the fixed hues on the left side */
      memcpy( &G_palette_data[3 * dest_base_index], 
              &G_palette_data[3 * source_base_index], 
              3 * num_shades * (1 + fixed_hues_left));

      /* copying the fixed hues on the right side */
      memcpy( &G_palette_data[3 * (dest_base_index + (1 + num_hues - fixed_hues_right) * num_shades)], 
              &G_palette_data[3 * (source_base_index + (1 + num_hues - fixed_hues_right) * num_shades)], 
              3 * num_shades * fixed_hues_right);

      /* copy rotated hues from the original rotated palette */
      source_base_index = ((p * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;
      dest_base_index = (((7 + p) * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * (dest_base_index + (1 + fixed_hues_left) * num_shades)], 
              &G_palette_data[3 * (source_base_index + (1 + fixed_hues_left) * num_shades)], 
              3 * num_shades * (num_hues - fixed_hues_left - fixed_hues_right));
    }
  }

  /* palette 13: alternate greyscale (preserving flesh tones) */
  for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    /* copy non-rotated hues from palette 0 */
    source_base_index = m * PALETTE_SIZE;
    dest_base_index = ((13 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    /* copying grey and the fixed hues on the left side */
    memcpy( &G_palette_data[3 * dest_base_index], 
            &G_palette_data[3 * source_base_index], 
            3 * num_shades * (1 + fixed_hues_left));

    /* copying the fixed hues on the right side */
    memcpy( &G_palette_data[3 * (dest_base_index + (1 + num_hues - fixed_hues_right) * num_shades)], 
            &G_palette_data[3 * (source_base_index + (1 + num_hues - fixed_hues_right) * num_shades)], 
            3 * num_shades * fixed_hues_right);

    /* copy greyscale hues from the original greyscale palette */
    source_base_index = ((6 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;
    dest_base_index = ((13 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

    memcpy( &G_palette_data[3 * (dest_base_index + (1 + fixed_hues_left) * num_shades)], 
            &G_palette_data[3 * (source_base_index + (1 + fixed_hues_left) * num_shades)], 
            3 * num_shades * (num_hues - fixed_hues_left - fixed_hues_right));
  }

  /* palettes 14: red tint */
  for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((14 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * (dest_base_index + n * num_shades)], 
              &G_palette_data[3 * (source_base_index + tint_hue_red * num_shades)], 
              3 * num_shades);
    }
  }

  /* palettes 15: blue tint */
  for (m = 0; m < TEXTURE_LEVELS_PER_PALETTE; m++)
  {
    for (n = 0; n < num_gradients; n++)
    {
      source_base_index = m * PALETTE_SIZE;
      dest_base_index = ((15 * TEXTURE_LEVELS_PER_PALETTE) + m) * PALETTE_SIZE;

      memcpy( &G_palette_data[3 * (dest_base_index + n * num_shades)], 
              &G_palette_data[3 * (source_base_index + tint_hue_blue * num_shades)], 
              3 * num_shades);
    }
  }

  #undef PALETTE_SIZE

  #undef TEXTURE_LEVELS_PER_PALETTE
  #undef TEXTURE_BASE_LEVEL

  return 0;
}

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
      else if (!strcmp("composite_04_1x", argv[i]))
        G_source = SOURCE_COMPOSITE_04_1X;
      else if (!strcmp("composite_04_1x_rotated", argv[i]))
        G_source = SOURCE_COMPOSITE_04_1X_ROTATED;
      else if (!strcmp("composite_08_2x", argv[i]))
        G_source = SOURCE_COMPOSITE_08_2X;
      else if (!strcmp("composite_08_2p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_08_2p50X;
      else if (!strcmp("composite_16_1x", argv[i]))
        G_source = SOURCE_COMPOSITE_16_1X;
      else if (!strcmp("composite_16_1x_rotated", argv[i]))
        G_source = SOURCE_COMPOSITE_16_1X_ROTATED;
      else if (!strcmp("composite_16_3x", argv[i]))
        G_source = SOURCE_COMPOSITE_16_3X;
      else if (!strcmp("composite_16_3x_rotated", argv[i]))
        G_source = SOURCE_COMPOSITE_16_3X_ROTATED;
      else if (!strcmp("composite_32_2x", argv[i]))
        G_source = SOURCE_COMPOSITE_32_2X;
      else if (!strcmp("composite_32_2p50x", argv[i]))
        G_source = SOURCE_COMPOSITE_32_2p50X;
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
  else if (G_source == SOURCE_COMPOSITE_04_1X)
    strncpy(output_tga_filename, "composite_04_1x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_04_1X_ROTATED)
    strncpy(output_tga_filename, "composite_04_1x_rotated.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_08_2X)
    strncpy(output_tga_filename, "composite_08_2x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_08_2p50X)
    strncpy(output_tga_filename, "composite_08_2p50x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_16_1X)
    strncpy(output_tga_filename, "composite_16_1x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_16_1X_ROTATED)
    strncpy(output_tga_filename, "composite_16_1x_rotated.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_16_3X)
    strncpy(output_tga_filename, "composite_16_3x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_16_3X_ROTATED)
    strncpy(output_tga_filename, "composite_16_3x_rotated.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_32_2X)
    strncpy(output_tga_filename, "composite_32_2x.tga", 32);
  else if (G_source == SOURCE_COMPOSITE_32_2p50X)
    strncpy(output_tga_filename, "composite_32_2p50x.tga", 32);
  else
  {
    printf("Unable to set output filename: Unknown source. Exiting...\n");
    return 0;
  }

  /* set palette size */
  if ((G_source == SOURCE_APPROX_NES)         || 
      (G_source == SOURCE_APPROX_NES_ROTATED) || 
      (G_source == SOURCE_COMPOSITE_04_1X)    || 
      (G_source == SOURCE_COMPOSITE_04_1X_ROTATED))
  {
    G_palette_size = 64;
  }
  else if ( (G_source == SOURCE_COMPOSITE_08_2X)    || 
            (G_source == SOURCE_COMPOSITE_08_2p50X) || 
            (G_source == SOURCE_COMPOSITE_16_1X)    || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED))
  {
    G_palette_size = 256;
  }
  else if ( (G_source == SOURCE_COMPOSITE_16_3X)          || 
            (G_source == SOURCE_COMPOSITE_16_3X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_32_2X)          || 
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
  else if ( (G_source == SOURCE_COMPOSITE_04_1X) || 
            (G_source == SOURCE_COMPOSITE_04_1X_ROTATED))
  {
    if (generate_palette_64_color())
    {
      printf("Error generating texture. Exiting...\n");
      return 0;
    }
  }
  else if ( (G_source == SOURCE_COMPOSITE_08_2X)    || 
            (G_source == SOURCE_COMPOSITE_08_2p50X) || 
            (G_source == SOURCE_COMPOSITE_16_1X)    || 
            (G_source == SOURCE_COMPOSITE_16_1X_ROTATED))
  {
    if (generate_palette_256_color())
    {
      printf("Error generating texture. Exiting...\n");
      return 0;
    }
  }
  else if ( (G_source == SOURCE_COMPOSITE_16_3X)          || 
            (G_source == SOURCE_COMPOSITE_16_3X_ROTATED)  || 
            (G_source == SOURCE_COMPOSITE_32_2X)          || 
            (G_source == SOURCE_COMPOSITE_32_2p50X))
  {
    if (generate_palette_1024_color())
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
