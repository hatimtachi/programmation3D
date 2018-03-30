#version 330

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

uniform mat4 lightprojectionMatrix;
uniform mat4 lightViewMatrix;
uniform vec2 cam;
layout (location = 0) in vec3 vsiPosition;
layout (location = 1) in vec3 vsiNormal;
layout (location = 2) in vec2 vsiTexCoord;
 
out vec2 vsoTexCoord;
out vec3 vsoNormal;
out vec4 vsoModPosition;
out vec3 vsoPosition;
out vec4 vsoMVPos;
out vec4 vsoSMCoord;

void main(void) {
  const mat4 bias=mat4(0.5,0.0,0.0,0.0,
        0.0,0.5,0.0,0.0,
        0.0,0.0,0.5,0.0,
        0.5,0.5,0.5,1.0);


  vsoNormal = (transpose(inverse(modelViewMatrix)) * vec4(vsiNormal.xyz, 0.0)).xyz;
  vsoPosition = vsiPosition;
  vsoModPosition = modelViewMatrix * vec4(vsiPosition.xyz, 1.0);
  gl_Position = projectionMatrix * modelViewMatrix * vec4(vsiPosition.xyz, 1.0);
  vsoTexCoord = vsiTexCoord+cam;
  vsoMVPos = projectionMatrix * modelViewMatrix * vec4(vsiPosition,1.0);
  vsoSMCoord = bias * lightprojectionMatrix * lightViewMatrix * modelViewMatrix * vec4(vsiPosition,1.0);
}
