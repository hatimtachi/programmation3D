#version 330
uniform sampler2D myTexture;
uniform vec4 lumpos;
uniform int sky, fog,eau;
uniform sampler1D degrade;
uniform float cycle;
in vec2 vsoTexCoord;
in vec3 vsoNormal;
in vec4 vsoModPosition;
in vec3 vsoPosition;

out vec4 fragColor;

uniform sampler2D permTexture;
uniform sampler2D gradTexture;


#define ONE 0.00390625
#define ONEHALF 0.001953125


float fade(float t) {
  // return t*t*(3.0-2.0*t); // Old fade, yields discontinuous second derivative
  return t*t*t*(t*(t*6.0-15.0)+10.0); // Improved fade, yields C2-continuous noise
}

/*
 * Efficient simplex indexing functions by Bill Licea-Kane, ATI. Thanks!
 * (This was originally implemented as a texture lookup. Nice to avoid that.)
 */
void simplex( const in vec3 P, out vec3 offset1, out vec3 offset2 )
{
  vec3 offset0;
 
  vec2 isX = step( P.yz, P.xx );         // P.x >= P.y ? 1.0 : 0.0;  P.x >= P.z ? 1.0 : 0.0;
  offset0.x  = dot( isX, vec2( 1.0 ) );  // Accumulate all P.x >= other channels in offset.x
  offset0.yz = 1.0 - isX;                // Accumulate all P.x <  other channels in offset.yz

  float isY = step( P.z, P.y );          // P.y >= P.z ? 1.0 : 0.0;
  offset0.y += isY;                      // Accumulate P.y >= P.z in offset.y
  offset0.z += 1.0 - isY;                // Accumulate P.y <  P.z in offset.z

  offset2 = clamp(   offset0, 0.0, 1.0 );
  // offset2 contains 1 in each channel that was 1 or 2
  offset1 = clamp( --offset0, 0.0, 1.0 );
  // offset1 contains 1 in the single channel that was 1
}

float noise(vec2 P)
{
  vec2 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled and offset for texture lookup
  vec2 Pf = fract(P);             // Fractional part for interpolation

  // Noise contribution from lower left corner
  vec2 grad00 = texture(permTexture, Pi).rg * 4.0 - 1.0;
  float n00 = dot(grad00, Pf);

  // Noise contribution from lower right corner
  vec2 grad10 = texture(permTexture, Pi + vec2(ONE, 0.0)).rg * 4.0 - 1.0;
  float n10 = dot(grad10, Pf - vec2(1.0, 0.0));

  // Noise contribution from upper left corner
  vec2 grad01 = texture(permTexture, Pi + vec2(0.0, ONE)).rg * 4.0 - 1.0;
  float n01 = dot(grad01, Pf - vec2(0.0, 1.0));

  // Noise contribution from upper right corner
  vec2 grad11 = texture(permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
  float n11 = dot(grad11, Pf - vec2(1.0, 1.0));

  // Blend contributions along x
  vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade(Pf.x));

  // Blend contributions along y
  float n_xy = mix(n_x.x, n_x.y, fade(Pf.y));

  // We're done, return the final noise value.
  return n_xy;
}


/*
 * 3D classic noise. Slower, but a lot more useful than 2D noise.
 */
float noise(vec3 P)
{
  vec3 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled so +1 moves one texel
                                  // and offset 1/2 texel to sample texel centers
  vec3 Pf = fract(P);     // Fractional part for interpolation

  // Noise contributions from (x=0, y=0), z=0 and z=1
  float perm00 = texture(permTexture, Pi.xy).a ;
  vec3  grad000 = texture(permTexture, vec2(perm00, Pi.z)).rgb * 4.0 - 1.0;
  float n000 = dot(grad000, Pf);
  vec3  grad001 = texture(permTexture, vec2(perm00, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n001 = dot(grad001, Pf - vec3(0.0, 0.0, 1.0));

  // Noise contributions from (x=0, y=1), z=0 and z=1
  float perm01 = texture(permTexture, Pi.xy + vec2(0.0, ONE)).a ;
  vec3  grad010 = texture(permTexture, vec2(perm01, Pi.z)).rgb * 4.0 - 1.0;
  float n010 = dot(grad010, Pf - vec3(0.0, 1.0, 0.0));
  vec3  grad011 = texture(permTexture, vec2(perm01, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n011 = dot(grad011, Pf - vec3(0.0, 1.0, 1.0));

  // Noise contributions from (x=1, y=0), z=0 and z=1
  float perm10 = texture(permTexture, Pi.xy + vec2(ONE, 0.0)).a ;
  vec3  grad100 = texture(permTexture, vec2(perm10, Pi.z)).rgb * 4.0 - 1.0;
  float n100 = dot(grad100, Pf - vec3(1.0, 0.0, 0.0));
  vec3  grad101 = texture(permTexture, vec2(perm10, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n101 = dot(grad101, Pf - vec3(1.0, 0.0, 1.0));

  // Noise contributions from (x=1, y=1), z=0 and z=1
  float perm11 = texture(permTexture, Pi.xy + vec2(ONE, ONE)).a ;
  vec3  grad110 = texture(permTexture, vec2(perm11, Pi.z)).rgb * 4.0 - 1.0;
  float n110 = dot(grad110, Pf - vec3(1.0, 1.0, 0.0));
  vec3  grad111 = texture(permTexture, vec2(perm11, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float n111 = dot(grad111, Pf - vec3(1.0, 1.0, 1.0));

  // Blend contributions along x
  vec4 n_x = mix(vec4(n000, n001, n010, n011),
                 vec4(n100, n101, n110, n111), fade(Pf.x));

  // Blend contributions along y
  vec2 n_xy = mix(n_x.xy, n_x.zw, fade(Pf.y));

  // Blend contributions along z
  float n_xyz = mix(n_xy.x, n_xy.y, fade(Pf.z));

  // We're done, return the final noise value.
  return n_xyz;
}


float noise(vec4 P)
{
  vec4 Pi = ONE*floor(P)+ONEHALF; // Integer part, scaled so +1 moves one texel
                                  // and offset 1/2 texel to sample texel centers
  vec4 Pf = fract(P);      // Fractional part for interpolation

  float perm00xy = texture(permTexture, Pi.xy).a ;
  float perm00zw = texture(permTexture, Pi.zw).a ;
  vec4 grad0000 = texture(gradTexture, vec2(perm00xy, perm00zw)).rgba * 4.0 -1.0;
  float n0000 = dot(grad0000, Pf);

  float perm01zw = texture(permTexture, Pi.zw  + vec2(0.0, ONE)).a ;
  vec4  grad0001 = texture(gradTexture, vec2(perm00xy, perm01zw)).rgba * 4.0 - 1.0;
  float n0001 = dot(grad0001, Pf - vec4(0.0, 0.0, 0.0, 1.0));

  float perm10zw = texture(permTexture, Pi.zw  + vec2(ONE, 0.0)).a ;
  vec4  grad0010 = texture(gradTexture, vec2(perm00xy, perm10zw)).rgba * 4.0 - 1.0;
  float n0010 = dot(grad0010, Pf - vec4(0.0, 0.0, 1.0, 0.0));

  float perm11zw = texture(permTexture, Pi.zw  + vec2(ONE, ONE)).a ;
  vec4  grad0011 = texture(gradTexture, vec2(perm00xy, perm11zw)).rgba * 4.0 - 1.0;
  float n0011 = dot(grad0011, Pf - vec4(0.0, 0.0, 1.0, 1.0));

  float perm01xy = texture(permTexture, Pi.xy + vec2(0.0, ONE)).a ;
  vec4  grad0100 = texture(gradTexture, vec2(perm01xy, perm00zw)).rgba * 4.0 - 1.0;
  float n0100 = dot(grad0100, Pf - vec4(0.0, 1.0, 0.0, 0.0));

  vec4  grad0101 = texture(gradTexture, vec2(perm01xy, perm01zw)).rgba * 4.0 - 1.0;
  float n0101 = dot(grad0101, Pf - vec4(0.0, 1.0, 0.0, 1.0));

  vec4  grad0110 = texture(gradTexture, vec2(perm01xy, perm10zw)).rgba * 4.0 - 1.0;
  float n0110 = dot(grad0110, Pf - vec4(0.0, 1.0, 1.0, 0.0));

  vec4  grad0111 = texture(gradTexture, vec2(perm01xy, perm11zw)).rgba * 4.0 - 1.0;
  float n0111 = dot(grad0111, Pf - vec4(0.0, 1.0, 1.0, 1.0));

  float perm10xy = texture(permTexture, Pi.xy + vec2(ONE, 0.0)).a ;
  vec4  grad1000 = texture(gradTexture, vec2(perm10xy, perm00zw)).rgba * 4.0 - 1.0;
  float n1000 = dot(grad1000, Pf - vec4(1.0, 0.0, 0.0, 0.0));

  vec4  grad1001 = texture(gradTexture, vec2(perm10xy, perm01zw)).rgba * 4.0 - 1.0;
  float n1001 = dot(grad1001, Pf - vec4(1.0, 0.0, 0.0, 1.0));

  vec4  grad1010 = texture(gradTexture, vec2(perm10xy, perm10zw)).rgba * 4.0 - 1.0;
  float n1010 = dot(grad1010, Pf - vec4(1.0, 0.0, 1.0, 0.0));

  vec4  grad1011 = texture(gradTexture, vec2(perm10xy, perm11zw)).rgba * 4.0 - 1.0;
  float n1011 = dot(grad1011, Pf - vec4(1.0, 0.0, 1.0, 1.0));

  float perm11xy = texture(permTexture, Pi.xy + vec2(ONE, ONE)).a ;
  vec4  grad1100 = texture(gradTexture, vec2(perm11xy, perm00zw)).rgba * 4.0 - 1.0;
  float n1100 = dot(grad1100, Pf - vec4(1.0, 1.0, 0.0, 0.0));

  vec4  grad1101 = texture(gradTexture, vec2(perm11xy, perm01zw)).rgba * 4.0 - 1.0;
  float n1101 = dot(grad1101, Pf - vec4(1.0, 1.0, 0.0, 1.0));

  vec4  grad1110 = texture(gradTexture, vec2(perm11xy, perm10zw)).rgba * 4.0 - 1.0;
  float n1110 = dot(grad1110, Pf - vec4(1.0, 1.0, 1.0, 0.0));

  vec4  grad1111 = texture(gradTexture, vec2(perm11xy, perm11zw)).rgba * 4.0 - 1.0;
  float n1111 = dot(grad1111, Pf - vec4(1.0, 1.0, 1.0, 1.0));

  // Blend contributions along x
  float fadex = fade(Pf.x);
  vec4 n_x0 = mix(vec4(n0000, n0001, n0010, n0011),
                  vec4(n1000, n1001, n1010, n1011), fadex);
  vec4 n_x1 = mix(vec4(n0100, n0101, n0110, n0111),
                  vec4(n1100, n1101, n1110, n1111), fadex);

  // Blend contributions along y
  vec4 n_xy = mix(n_x0, n_x1, fade(Pf.y));

  // Blend contributions along z
  vec2 n_xyz = mix(n_xy.xy, n_xy.zw, fade(Pf.z));

  // Blend contributions along w
  float n_xyzw = mix(n_xyz.x, n_xyz.y, fade(Pf.w));

  // We're done, return the final noise value.
  return n_xyzw;
}


float snoise(vec2 P) {

// Skew and unskew factors are a bit hairy for 2D, so define them as constants
// This is (sqrt(3.0)-1.0)/2.0
#define F2 0.366025403784
// This is (3.0-sqrt(3.0))/6.0
#define G2 0.211324865405

  // Skew the (x,y) space to determine which cell of 2 simplices we're in
 	float s = (P.x + P.y) * F2;   // Hairy factor for 2D skewing
  vec2 Pi = floor(P + s);
  float t = (Pi.x + Pi.y) * G2; // Hairy factor for unskewing
  vec2 P0 = Pi - t; // Unskew the cell origin back to (x,y) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec2 Pf0 = P - P0;  // The x,y distances from the cell origin

  vec2 o1;
  if(Pf0.x > Pf0.y) o1 = vec2(1.0, 0.0);  // +x, +y traversal order
  else o1 = vec2(0.0, 1.0);               // +y, +x traversal order

  // Noise contribution from simplex origin
  vec2 grad0 = texture(permTexture, Pi).rg * 4.0 - 1.0;
  float t0 = 0.5 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from middle corner
  vec2 Pf1 = Pf0 - o1 + G2;
  vec2 grad1 = texture(permTexture, Pi + o1*ONE).rg * 4.0 - 1.0;
  float t1 = 0.5 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }
  
  // Noise contribution from last corner
  vec2 Pf2 = Pf0 - vec2(1.0-2.0*G2);
  vec2 grad2 = texture(permTexture, Pi + vec2(ONE, ONE)).rg * 4.0 - 1.0;
  float t2 = 0.5 - dot(Pf2, Pf2);
  float n2;
  if(t2 < 0.0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 70.0 * (n0 + n1 + n2);
}


/*
 * 3D simplex noise. Comparable in speed to classic noise, better looking.
 */
float snoise(vec3 P) {

// The skewing and unskewing factors are much simpler for the 3D case
#define F3 0.333333333333
#define G3 0.166666666667

  // Skew the (x,y,z) space to determine which cell of 6 simplices we're in
 	float s = (P.x + P.y + P.z) * F3; // Factor for 3D skewing
  vec3 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z) * G3;
  vec3 P0 = Pi - t; // Unskew the cell origin back to (x,y,z) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec3 Pf0 = P - P0;  // The x,y distances from the cell origin

  vec3 o1;
  vec3 o2;
  simplex(Pf0, o1, o2);

  // Noise contribution from simplex origin
  float perm0 = texture(permTexture, Pi.xy).a;
  vec3  grad0 = texture(permTexture, vec2(perm0, Pi.z)).rgb * 4.0 - 1.0;
  float t0 = 0.6 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from second corner
  vec3 Pf1 = Pf0 - o1 + G3;
  float perm1 = texture(permTexture, Pi.xy + o1.xy*ONE).a;
  vec3  grad1 = texture(permTexture, vec2(perm1, Pi.z + o1.z*ONE)).rgb * 4.0 - 1.0;
  float t1 = 0.6 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }
  
  // Noise contribution from third corner
  vec3 Pf2 = Pf0 - o2 + 2.0 * G3;
  float perm2 = texture(permTexture, Pi.xy + o2.xy*ONE).a;
  vec3  grad2 = texture(permTexture, vec2(perm2, Pi.z + o2.z*ONE)).rgb * 4.0 - 1.0;
  float t2 = 0.6 - dot(Pf2, Pf2);
  float n2;
  if (t2 < 0.0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }
  
  // Noise contribution from last corner
  vec3 Pf3 = Pf0 - vec3(1.0-3.0*G3);
  float perm3 = texture(permTexture, Pi.xy + vec2(ONE, ONE)).a;
  vec3  grad3 = texture(permTexture, vec2(perm3, Pi.z + ONE)).rgb * 4.0 - 1.0;
  float t3 = 0.6 - dot(Pf3, Pf3);
  float n3;
  if(t3 < 0.0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad3, Pf3);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 32.0 * (n0 + n1 + n2 + n3);
}


/*
 * 4D simplex noise. A lot faster than classic 4D noise, and better looking.
 */

float snoise(vec4 P) {

// The skewing and unskewing factors are hairy again for the 4D case
// This is (sqrt(5.0)-1.0)/4.0
#define F4 0.309016994375
// This is (5.0-sqrt(5.0))/20.0
#define G4 0.138196601125

 	float s = (P.x + P.y + P.z + P.w) * F4; // Factor for 4D skewing
  vec4 Pi = floor(P + s);
  float t = (Pi.x + Pi.y + Pi.z + Pi.w) * G4;
  vec4 P0 = Pi - t; // Unskew the cell origin back to (x,y,z,w) space
  Pi = Pi * ONE + ONEHALF; // Integer part, scaled and offset for texture lookup

  vec4 Pf0 = P - P0;  // The x,y distances from the cell origin

  vec4 o1;
  vec4 o2;
  vec4 o3;
  //simplex(Pf0, o1, o2, o3);

  // Noise contribution from simplex origin
  float perm0xy = texture(permTexture, Pi.xy).a;
  float perm0zw = texture(permTexture, Pi.zw).a;
  vec4  grad0 = texture(gradTexture, vec2(perm0xy, perm0zw)).rgba * 4.0 - 1.0;
  float t0 = 0.6 - dot(Pf0, Pf0);
  float n0;
  if (t0 < 0.0) n0 = 0.0;
  else {
    t0 *= t0;
    n0 = t0 * t0 * dot(grad0, Pf0);
  }

  // Noise contribution from second corner
  vec4 Pf1 = Pf0 - o1 + G4;
  o1 = o1 * ONE;
  float perm1xy = texture(permTexture, Pi.xy + o1.xy).a;
  float perm1zw = texture(permTexture, Pi.zw + o1.zw).a;
  vec4  grad1 = texture(gradTexture, vec2(perm1xy, perm1zw)).rgba * 4.0 - 1.0;
  float t1 = 0.6 - dot(Pf1, Pf1);
  float n1;
  if (t1 < 0.0) n1 = 0.0;
  else {
    t1 *= t1;
    n1 = t1 * t1 * dot(grad1, Pf1);
  }
  
  // Noise contribution from third corner
  vec4 Pf2 = Pf0 - o2 + 2.0 * G4;
  o2 = o2 * ONE;
  float perm2xy = texture(permTexture, Pi.xy + o2.xy).a;
  float perm2zw = texture(permTexture, Pi.zw + o2.zw).a;
  vec4  grad2 = texture(gradTexture, vec2(perm2xy, perm2zw)).rgba * 4.0 - 1.0;
  float t2 = 0.6 - dot(Pf2, Pf2);
  float n2;
  if (t2 < 0.0) n2 = 0.0;
  else {
    t2 *= t2;
    n2 = t2 * t2 * dot(grad2, Pf2);
  }
  
  // Noise contribution from fourth corner
  vec4 Pf3 = Pf0 - o3 + 3.0 * G4;
  o3 = o3 * ONE;
  float perm3xy = texture(permTexture, Pi.xy + o3.xy).a;
  float perm3zw = texture(permTexture, Pi.zw + o3.zw).a;
  vec4  grad3 = texture(gradTexture, vec2(perm3xy, perm3zw)).rgba * 4.0 - 1.0;
  float t3 = 0.6 - dot(Pf3, Pf3);
  float n3;
  if (t3 < 0.0) n3 = 0.0;
  else {
    t3 *= t3;
    n3 = t3 * t3 * dot(grad3, Pf3);
  }
  
  // Noise contribution from last corner
  vec4 Pf4 = Pf0 - vec4(1.0-4.0*G4);
  float perm4xy = texture(permTexture, Pi.xy + vec2(ONE, ONE)).a;
  float perm4zw = texture(permTexture, Pi.zw + vec2(ONE, ONE)).a;
  vec4  grad4 = texture(gradTexture, vec2(perm4xy, perm4zw)).rgba * 4.0 - 1.0;
  float t4 = 0.6 - dot(Pf4, Pf4);
  float n4;
  if(t4 < 0.0) n4 = 0.0;
  else {
    t4 *= t4;
    n4 = t4 * t4 * dot(grad4, Pf4);
  }

  // Sum up and scale the result to cover the range [-1,1]
  return 27.0 * (n0 + n1 + n2 + n3 + n4);
}








const vec2 steps = vec2(0.005, 0.005);

vec2 G[9] = vec2[9](vec2(-1, -1), vec2(0, -2), vec2(1, -1),
                    vec2(-2,  0), vec2(0,  0), vec2(2, 0),
                    vec2(-1,  1), vec2(0,  2), vec2(1,  1));

vec2 offset[9] = vec2[9](vec2(-steps.x,  steps.y), vec2(0,  steps.y), vec2(steps.x,  steps.y),
                         vec2(-steps.x,  0),       vec2(0,  0),       vec2(steps.x, 0),
                         vec2(-steps.x, -steps.y), vec2(0, -steps.y), vec2(steps.x, -steps.y));

float alt(vec2 xy) {
  const float mamp = 1.0, mfreq = 100.0;
  float amp, freq;
  float n = 0.0;//1 - 2.0 * abs(noise(vec3(xy*300, cycle / 5)) - 0.5);
  for(int i = 0; i < 8; i++) {
    float mult = pow(2.2, 7.0 - float(i)), np;
    amp = mamp / mult;
    freq = mfreq * mult;
    np = amp * (2.0 * abs(noise(vec3(xy*freq, cycle / 2.5)) - 0.5));
    n += np * np;
  }
  float s = n + (1.0 + sin(xy.y * 300.0 + cycle)) / 2.0;
  return s;
}

vec2 sobel() {
  vec2 g = vec2(0);
  for(int i = 0; i < 9; i++)
    g += G[i] * alt(vsoTexCoord + offset[i]);
  return g;
}

void perturbe(inout vec3 normale) {
  const vec3 T = vec3(0, 0, -1);
  const vec3 B = vec3(1, 0, 0);
  vec2 v = sobel() / 1000.0;
  normale = normalize(normale + v.x * B + v.y * T);
}



void main(void) {
  vec3 lum = normalize(vsoModPosition.xyz - lumpos.xyz);
  fragColor = vec4(texture(degrade, (1.0 + vsoPosition.y) / 2.0).rgb * (vec3(0.1) + 0.9 * vec3(1) * dot(normalize(vsoNormal), -lum)), 1.0);

  const float SQRT2 = 1.442695, fog_density2 = 0.001;
  const vec4 fog_color = vec4(0.5, 0.5, 0.5, 1.0);
  if(sky != 0) {
    float x = 0.02 + 0.96 * abs(2.0 * vsoTexCoord.x - 1);
    float y = clamp(2.0 * (1.0 - vsoTexCoord.y), 0, 1);
    vec4 tex = vec4(texture(degrade, (1.0 + vsoPosition.y) / 2.0).rgb * (vec3(0.1) + 0.9 * vec3(1) * dot(normalize(vsoNormal), -lum)), 1.0);
     if(fog!= 0) {
      float ffactor = 1.0 - pow(y, 30);
      fragColor = mix(fog_color, tex, ffactor);
    } else{
            fragColor = tex;
    }
  }
  else{
    vec4 tex = vec4(texture(degrade, (1.0 + vsoPosition.y) / 2.0).rgb * (vec3(0.1) + 0.9 * vec3(1) * dot(normalize(vsoNormal), -lum)), 1.0);
    if(fog != 0) {
      float z = gl_FragCoord.z / gl_FragCoord.w;
      float ffactor = exp(-fog_density2 * z * z);
      ffactor = clamp(ffactor, 0.0, 1.0);
      fragColor = mix(fog_color, tex, ffactor);
      fragColor = mix(fog_color, tex, ffactor);
    } 
    else{
      fragColor = tex;
    }
  }
}
