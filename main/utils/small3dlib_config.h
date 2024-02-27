#ifndef _SMALL3DLIB_CONFIG_H_
#define _SMALL3DLIB_CONFIG_H_

//// Swadge-specific config

#include "hdw-tft.h"

#define S3L_MAX_PIXELS (TFT_WIDTH * TFT_HEIGHT)

void configureS3dScreen(int x, int y, int w, int h);
void configureS3dFramebuffer(int w, int h, paletteColor_t* fb);

//// End Swadge-specific config

#ifdef S3L_RESOLUTION_X
  #ifdef S3L_RESOLUTION_Y
    #define S3L_MAX_PIXELS (S3L_RESOLUTION_X * S3L_RESOLUTION_Y)
  #endif
#endif

#ifndef S3L_RESOLUTION_X
  #ifndef S3L_MAX_PIXELS
    #error Dynamic resolution set (S3L_RESOLUTION_X not defined), but\
           S3L_MAX_PIXELS not defined!
  #endif

  extern uint16_t S3L_resolutionX; /**< If a static resolution is not set with
                                       S3L_RESOLUTION_X, this variable can be
                                       used to change X resolution at runtime,
                                       in which case S3L_MAX_PIXELS has to be
                                       defined (to allocate zBuffer etc.)! */
  #define S3L_EXTERN_RESOLUTION_X 1
  #define S3L_RESOLUTION_X S3L_resolutionX
#endif

#ifndef S3L_RESOLUTION_Y
  #ifndef S3L_MAX_PIXELS
    #error Dynamic resolution set (S3L_RESOLUTION_Y not defined), but\
           S3L_MAX_PIXELS not defined!
  #endif

  extern uint16_t S3L_resolutionY; /**< Same as S3L_resolutionX, but for Y
                                       resolution. */
  #define S3L_EXTERN_RESOLUTION_Y 1
  #define S3L_RESOLUTION_Y S3L_resolutionY
#endif

#ifndef S3L_USE_WIDER_TYPES
  /** If true, the library will use wider data types which will largely supress
  many rendering bugs and imprecisions happening due to overflows, but this will
  also consumer more RAM and may potentially be slower on computers with smaller
  native integer. */
  #define S3L_USE_WIDER_TYPES 0
#endif

#ifndef S3L_SIN_METHOD
  /** Says which method should be used for computing sin/cos functions, possible
  values: 0 (lookup table, takes more program memory), 1 (Bhaskara's
  approximation, slower). This may cause the trigonometric functions give
  slightly different results. */
  #define S3L_SIN_METHOD 0
#endif

/** Units of measurement in 3D space. There is S3L_FRACTIONS_PER_UNIT in one
spatial unit. By dividing the unit into fractions we effectively achieve a
fixed point arithmetic. The number of fractions is a constant that serves as
1.0 in floating point arithmetic (normalization etc.). */

typedef
#if S3L_USE_WIDER_TYPES
  int64_t
#else
  int32_t
#endif
  S3L_Unit;

/** How many fractions a spatial unit is split into, i.e. this is the fixed
point scaling. This is NOT SUPPOSED TO BE REDEFINED, so rather don't do it
(otherwise things may overflow etc.). */
#define S3L_FRACTIONS_PER_UNIT 512
#define S3L_F S3L_FRACTIONS_PER_UNIT

typedef
#if S3L_USE_WIDER_TYPES
  int32_t
#else
  int16_t
#endif
  S3L_ScreenCoord;

typedef
#if S3L_USE_WIDER_TYPES
  uint32_t
#else
  uint16_t
#endif
  S3L_Index;

#ifndef S3L_NEAR_CROSS_STRATEGY
  /** Specifies how the library will handle triangles that partially cross the
  near plane. These are problematic and require special handling. Possible
  values:

    0: Strictly cull any triangle crossing the near plane. This will make such
       triangles disappear. This is good for performance or models viewed only
       from at least small distance.
    1: Forcefully push the vertices crossing near plane in front of it. This is
       a cheap technique that can be good enough for displaying simple
       environments on slow devices, but texturing and geometric artifacts/warps
       will appear.
    2: Geometrically correct the triangles crossing the near plane. This may
       result in some triangles being subdivided into two and is a little more
       expensive, but the results will be geometrically correct, even though
       barycentric correction is not performed so texturing artifacts will
       appear. Can be ideal with S3L_FLAT.
    3: Perform both geometrical and barycentric correction of triangle crossing
       the near plane. This is significantly more expensive but results in
       correct rendering. */
  #define S3L_NEAR_CROSS_STRATEGY 0
#endif

#ifndef S3L_FLAT
  /** If on, disables computation of per-pixel values such as barycentric
  coordinates and depth -- these will still be available but will be the same
  for the whole triangle. This can be used to create flat-shaded renders and
  will be a lot faster. With this option on you will probably want to use
  sorting instead of z-buffer. */
  #define S3L_FLAT 0
#endif

#if S3L_FLAT
  #define S3L_COMPUTE_DEPTH 0
  #define S3L_PERSPECTIVE_CORRECTION 0
  // don't disable z-buffer, it makes sense to use it with no sorting
#endif

#ifndef S3L_PERSPECTIVE_CORRECTION
  /** Specifies what type of perspective correction (PC) to use. Remember this
  is an expensive operation! Possible values:

  0: No perspective correction. Fastest, inaccurate from most angles.
  1: Per-pixel perspective correction, accurate but very expensive.
  2: Approximation (computing only at every S3L_PC_APPROX_LENGTHth pixel).
     Quake-style approximation is used, which only computes the PC after
     S3L_PC_APPROX_LENGTH pixels. This is reasonably accurate and fast. */
  #define S3L_PERSPECTIVE_CORRECTION 0
#endif

#ifndef S3L_PC_APPROX_LENGTH
  /** For S3L_PERSPECTIVE_CORRECTION == 2, this specifies after how many pixels
  PC is recomputed. Should be a power of two to keep up the performance.
  Smaller is nicer but slower. */
  #define S3L_PC_APPROX_LENGTH 32
#endif

#if S3L_PERSPECTIVE_CORRECTION
  #define S3L_COMPUTE_DEPTH 1 // PC inevitably computes depth, so enable it
#endif

#ifndef S3L_COMPUTE_DEPTH
  /** Whether to compute depth for each pixel (fragment). Some other options
  may turn this on automatically. If you don't need depth information, turning
  this off can save performance. Depth will still be accessible in
  S3L_PixelInfo, but will be constant -- equal to center point depth -- over
  the whole triangle. */
  #define S3L_COMPUTE_DEPTH 1
#endif

#ifndef S3L_Z_BUFFER
  /** What type of z-buffer (depth buffer) to use for visibility determination.
  Possible values:

  0: Don't use z-buffer. This saves a lot of memory, but visibility checking
     won't be pixel-accurate and has to mostly be done by other means (typically
     sorting).
  1: Use full z-buffer (of S3L_Units) for visibiltiy determination. This is the
     most accurate option (and also a fast one), but requires a big amount of
     memory.
  2: Use reduced-size z-buffer (of bytes). This is fast and somewhat accurate,
     but inaccuracies can occur and a considerable amount of memory is
     needed. */
  #define S3L_Z_BUFFER 0
#endif

#ifndef S3L_REDUCED_Z_BUFFER_GRANULARITY
  /** For S3L_Z_BUFFER == 2 this sets the reduced z-buffer granularity. */
  #define S3L_REDUCED_Z_BUFFER_GRANULARITY 5
#endif

#ifndef S3L_STENCIL_BUFFER
  /** Whether to use stencil buffer for drawing -- with this a pixel that would
  be resterized over an already rasterized pixel (within a frame) will be
  discarded. This is mostly for front-to-back sorted drawing. */
  #define S3L_STENCIL_BUFFER 0
#endif

#ifndef S3L_SORT
  /** Defines how to sort triangles before drawing a frame. This can be used to
  solve visibility in case z-buffer is not used, to prevent overwriting already
  rasterized pixels, implement transparency etc. Note that for simplicity and
  performance a relatively simple sorting is used which doesn't work completely
  correctly, so mistakes can occur (even the best sorting wouldn't be able to
  solve e.g. intersecting triangles). Note that sorting requires a bit of extra
  memory -- an array of the triangles to sort -- the size of this array limits
  the maximum number of triangles that can be drawn in a single frame
  (S3L_MAX_TRIANGES_DRAWN). Possible values:

  0: Don't sort triangles. This is fastest and doesn't use extra memory.
  1: Sort triangles from back to front. This can in most cases solve visibility
     without requiring almost any extra memory compared to z-buffer.
  2: Sort triangles from front to back. This can be faster than back to front
     because we prevent computing pixels that will be overwritten by nearer
     ones, but we need a 1b stencil buffer for this (enable S3L_STENCIL_BUFFER),
     so a bit more memory is needed. */
  #define S3L_SORT 0
#endif

#ifndef S3L_MAX_TRIANGES_DRAWN
  /** Maximum number of triangles that can be drawn in sorted modes. This
  affects the size of the cache used for triangle sorting. */
  #define S3L_MAX_TRIANGES_DRAWN 128
#endif

#ifndef S3L_NEAR
  /** Distance of the near clipping plane. Points in front or EXATLY ON this
  plane are considered outside the frustum. This must be >= 0. */
  #define S3L_NEAR (S3L_F / 4)
#endif

#if S3L_NEAR <= 0
#define S3L_NEAR 1 // Can't be <= 0.
#endif

#ifndef S3L_NORMAL_COMPUTE_MAXIMUM_AVERAGE
  /** Affects the S3L_computeModelNormals function. See its description for
  details. */
  #define S3L_NORMAL_COMPUTE_MAXIMUM_AVERAGE 6
#endif

#ifndef S3L_FAST_LERP_QUALITY
  /** Quality (scaling) of SOME (stepped) linear interpolations. 0 will most
  likely be a tiny bit faster, but artifacts can occur for bigger tris, while
  higher values can fix this -- in theory all higher values will have the same
  speed (it is a shift value), but it mustn't be too high to prevent
  overflow. */
  #define S3L_FAST_LERP_QUALITY 11
#endif

#endif // guard
