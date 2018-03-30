#version 330

uniform vec4 couleur,lumpos;

uniform sampler2D myTexture;
uniform int hasTexture,fog;
uniform vec4 diffuse_color;
uniform vec4 specular_color;
uniform vec4 ambient_color;
uniform vec4 emission_color;
uniform float shininess;

in vec2 vsoTexCoord;
in vec3 vsoNormal;
in vec4 vsoModPosition;
in vec4 vsoMVPos;
in vec4 vsoSMCoord;

out vec4 fragColor;

void main(void) {
  const float SQRT2 = 1.442695, fog_density2 = 0.01;
  const vec4 fog_color = vec4(0.5, 0.5, 0.5, 1.0);
  /*
  vec3 N=normalize(vsoNormal.xyz);
  vec3 L=normalize(vsoMVPos.xyz - lumpos.xyz);
  vec3 projCoords=vsoSMCoord.xyz / vsoSMCoord.w;
  float diffuse=dot(N,-L);
  if(texture(myTexture,projCoords.xy).r<projCoords.z)
    diffuse*=0.5;
  fragColor=vec4((couleur.rgb * diffuse),couleur.a);*/
  vec3 lum  = normalize(vsoModPosition.xyz - lumpos.xyz);
  float diffuse = clamp(dot(normalize(vsoNormal), -lum), 0.0, 1.0);
  vec3 lightDirection = vec3(lumpos - vsoModPosition);
  vec4 specularReflection = specular_color * pow(max(0.0, dot(normalize(reflect(-lightDirection, vsoNormal)), normalize(vec3(-vsoModPosition)))), shininess);
  vec4 diffuseReflection = ambient_color*0.2 +diffuse_color * diffuse;
 
  fragColor = diffuseReflection + specularReflection;
  if(hasTexture != 0)
      fragColor *= texture(myTexture, vsoTexCoord);
    
  vec4 tex=texture(myTexture , vsoTexCoord);
  if(fog != 0){
      float z = gl_FragCoord.z / gl_FragCoord.w;
      float ffactor = exp(-fog_density2 * z * z);
      ffactor = clamp(ffactor, 0.0, 1.0);
      fragColor = mix(fog_color, tex, ffactor);
      fragColor = mix(fog_color, tex, ffactor);
    }
    
}


