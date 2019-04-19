#version 330

// The camera's position in world-space
uniform vec3 u_cam_pos;

// Color
uniform vec4 u_color;

// Properties of the single point light
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

// We also get the uniform texture we want to use.
uniform sampler2D u_texture_1;

// These are the inputs which are the outputs of the vertex shader.
in vec4 v_position;
in vec4 v_normal;

// This is where the final pixel color is output.
// Here, we are only interested in the first 3 dimensions (xyz).
// The 4th entry in this vector is for "alpha blending" which we
// do not require you to know about. For now, just set the alpha
// to 1.
out vec4 out_color;

void main() {
  // YOUR CODE HERE
    
//Ld = kd ( I / r2 ) max (0, n.l)
    float kd = 1.0;
    vec3 I = u_light_intensity;
    float r = distance(u_light_pos, v_position.xyz);
    vec3 n = normalize(v_normal.xyz);
    vec3 l = normalize(u_light_pos - v_position.xyz);
    vec3 Ld = kd * (I / (r*r)) * max(0, dot(n,l));
    vec4 Ld4 = vec4(Ld, 1);
    out_color = u_color * Ld4;
    out_color.a = 1;
    
    
  // (Placeholder code. You will want to replace it.)
//  out_color = (vec4(1, 1, 1, 0) + v_normal) / 2;
//  out_color.a = 1;
}
