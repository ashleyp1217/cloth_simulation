#version 330

uniform vec3 u_cam_pos;
uniform vec3 u_light_pos;
uniform vec3 u_light_intensity;

uniform vec4 u_color;

uniform sampler2D u_texture_2;
uniform vec2 u_texture_2_size;

uniform float u_normal_scaling;
uniform float u_height_scaling;

in vec4 v_position;
in vec4 v_normal;
in vec4 v_tangent;
in vec2 v_uv;

out vec4 out_color;

float h(vec2 uv) {
  // You may want to use this helper function...
 return texture(u_texture_2, uv)[0];
    return 0.0;
}

void main() {
  // YOUR CODE HERE
    vec3 n = v_normal.xyz;
    vec3 t = v_tangent.xyz;
    vec3 b = cross(n, t);
    mat3 TBN = mat3(t, b, n);
    
    float width = u_texture_2_size[0];
    float height = u_texture_2_size[1];
    float kh = u_height_scaling;
    float kn = u_normal_scaling;
    float u = v_uv[0];
    float v = v_uv[1];
    
    vec2 u1wv = vec2(u + (1/width), v);
    vec2 uv1h = vec2(u, v + (1/height));
    
    float dU = (h(u1wv) - h(v_uv)) * kn * kh;
    float dV = (h(uv1h) - h(v_uv)) * kn * kh;
    
    vec3 no = vec3(-dU, -dV, 1);
    vec3 nd = TBN * no;
    
    //phong
    float ka = 0.5;
    float kd = 0.5;
    float ks = 0.5;
    float Ia = 0.5;
    float p = 50;
    
    vec3 I = u_light_intensity;
    float r = distance(u_light_pos, v_position.xyz);
    vec3 n2 = normalize(nd); //replaced normal with newNormal
    vec3 l = normalize(u_light_pos - v_position.xyz);
    vec3 v2 = normalize(u_cam_pos - v_position.xyz);
    vec3 h = normalize(v2 + l);
    
    vec3 Ld = (ka * Ia) + ((kd * I / (r * r))*max(0, dot(n2,l))) + (ks * I / (r * r))* pow(max(0,dot(n2,h)),p);
    vec4 Ld4 = vec4(Ld, 1);
    out_color = u_color * Ld4;
    out_color.a = 1;
    
  // (Placeholder code. You will want to replace it.)
//  out_color = (vec4(1, 1, 1, 0) + v_normal) / 2;
//  out_color.a = 1;
}

