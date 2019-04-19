#version 330

uniform vec4 u_color;
uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

in vec4 v_position;
in vec4 v_normal;
in vec2 v_uv;

out vec4 out_color;

void main() {
  // YOUR CODE HERE
    //Ld = kd ( I / r2 ) max (0, n.l)
//    float kd = 1.0;
//    vec3 I = u_light_intensity;
//    float r = distance(u_light_pos, v_position.xyz);
//    vec3 n = normalize(v_normal.xyz);
//    vec3 l = normalize(u_light_pos - v_position.xyz);
//    vec3 Ld = kd * (I / (r*r)) * max(0, dot(n,l));
//    vec4 Ld4 = vec4(Ld, 1);
//    out_color = u_color * Ld4;
//    out_color.a = 1;
    
    float ka = 0.7;
    float kd = 0.7;
    float ks = 0.7;
    float Ia = 0.7;
    float p = 100;
    
    vec3 I = u_light_intensity;
    float r = distance(u_light_pos, v_position.xyz);
    vec3 n = normalize(v_normal.xyz);
    vec3 l = normalize(u_light_pos - v_position.xyz);
    vec3 v = normalize(u_cam_pos - v_position.xyz);
    vec3 h = normalize(v + l);

    //entire blinn phong
    vec3 Ld = (ka * Ia) + ((kd * I / (r * r))*max(0, dot(n,l))) + (ks * I / (r * r))* pow(max(0,dot(n,h)),p);
    
    //ambient only
//    vec3 Ld = vec3(ka * Ia, ka*Ia, ka*Ia);
    
    //specular only
//    vec3 Ld = (ks * I / (r * r))* pow(max(0,dot(n,h)),p);
    
    
    vec4 Ld4 = vec4(Ld, 1);
    out_color = u_color * Ld4;
    out_color.a = 1;
    
  // (Placeholder code. You will want to replace it.)
//  out_color = (vec4(1, 1, 1, 0) + v_normal) / 2;
//  out_color.a = 1;
}

