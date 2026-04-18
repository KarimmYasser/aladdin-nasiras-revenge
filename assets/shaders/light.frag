#version 330 core

// from light.vert
in Varyings {
    vec3 frag_pos;
    vec3 normal;
    vec2 tex_coord;
    vec4 color;
} fs_in;

out vec4 frag_color;

struct Material {
    sampler2D albedo_map;    // unit 0 — base color
    sampler2D specular_map;  // unit 1 — specular intensity mask
    sampler2D emission_map;  // unit 2 — self-glow

    vec3  albedo_tint;   // Tint multiplied over the albedo texture
    float shininess;     // Blinn-Phong exponent (higher = sharper highlight)
    float ambient;       // Constant ambient factor (avoids pitch-black shadows)
};

uniform Material material;

// Light struct — one entry per active light in the scene.
// The ForwardRenderer fills this array and uploads light_count.
#define MAX_LIGHTS 8

struct Light {
    int   type;       // 0 = Directional, 1 = Point, 2 = Spot
    vec3  position;   // for point & spot
    vec3  direction;  // for directional & spot
    vec3  color;
    float intensity;  // Brightness multiplier

    // for point & spot: 1 / (constant + linear*d + quadratic*d²)
    float att_constant;
    float att_linear;
    float att_quadratic;

    // for spot: angles stored as cosines for cheap dot-product comparison
    float inner_cutoff; // full intensity inside this cone
    float outer_cutoff; // zero intensity outside this cone
};

uniform Light lights[MAX_LIGHTS];
uniform int   light_count;  // Number of active lights (<= MAX_LIGHTS)

// Camera position in world space — needed for the view direction (specular)
uniform vec3 eye_pos;

// ---------------------------------------------------------------------------
// Blinn-Phong lighting function
// Computes the contribution of one light to the fragment color.
//
// N       — surface normal (world space, normalized)
// V       — view direction vector (fragment → camera, normalized)
// albedo  — sampled + tinted base color of the surface
// specular— sampled specular mask (1.0 = full specular, 0.0 = none)
// light   — the light to evaluate
// ---------------------------------------------------------------------------
vec3 computeLight(vec3 N, vec3 V, vec3 albedo, float specular, Light light) {
    vec3 L;           // Direction from fragment toward the light source
    float attenuation = 1.0;
    float spot_factor = 1.0;

    if (light.type == 0) {
        // --- Directional light ---
        // Direction is uniform; no attenuation; the light.direction points FROM the light,
        // so we negate it to get the direction toward the light.
        L = normalize(-light.direction);

    } else {
        // --- Point or Spot light ---
        vec3 offset = light.position - fs_in.frag_pos;
        float dist  = length(offset);
        L           = normalize(offset);

        // Physically-based inverse-square attenuation
        attenuation = 1.0 / (light.att_constant +
                              light.att_linear    * dist +
                              light.att_quadratic * dist * dist);

        if (light.type == 2) {
            // --- Spot light cone ---
            // dot(L, -light.direction) gives the cosine of the angle between L and the spot axis.
            // smoothstep creates a soft edge between the inner and outer cone boundaries.
            float cos_angle = dot(L, normalize(-light.direction));
            spot_factor = smoothstep(light.outer_cutoff, light.inner_cutoff, cos_angle);
        }
    }

    // --- Ambient term ---
    // A small constant contribution so surfaces never go completely black.
    vec3 ambient_color = albedo * material.ambient;

    // --- Diffuse term (Lambertian) ---
    // max(dot,0) prevents lighting from "behind" the surface.
    float diff         = max(dot(N, L), 0.0);
    vec3  diffuse_color = albedo * diff;

    // --- Specular term (Blinn-Phong) ---
    // H is the halfway vector between L and V; pow() controls the sharpness.
    vec3  H      = normalize(L + V);
    float spec   = pow(max(dot(N, H), 0.0), material.shininess);
    vec3  specular_color = specular * spec * vec3(1.0); // white spec highlight

    // Combine: ambient is added once (no light-color scaling),
    // diffuse & specular are scaled by attenuation, spot, intensity and color.
    return ambient_color
         + (diffuse_color + specular_color) * light.color * light.intensity * attenuation * spot_factor;
}

void main() {
    vec3 N = normalize(fs_in.normal);
    // View direction (fragment → camera)
    vec3 V = normalize(eye_pos - fs_in.frag_pos);

    vec4 albedo_sample   = texture(material.albedo_map,  fs_in.tex_coord) * fs_in.color;
    vec3 albedo          = albedo_sample.rgb * material.albedo_tint;
    float specular_value = texture(material.specular_map, fs_in.tex_coord).r;
    vec3  emission       = texture(material.emission_map, fs_in.tex_coord).rgb;

    // Accumulate contribution from every active light
    vec3 lighting = vec3(0.0);
    for (int i = 0; i < light_count; i++) {
        lighting += computeLight(N, V, albedo, specular_value, lights[i]);
    }

    // Objects with no lights still show their emission glow
    vec3 final_color = lighting + emission;

    frag_color = vec4(final_color, albedo_sample.a);
}
