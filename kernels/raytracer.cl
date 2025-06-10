// Ray struct with standard alignment
typedef struct {
    float4 pos;      // Current position (t, x, y, z)
    float4 vel;      // Current velocity (dt/dλ, dx/dλ, dy/dλ, dz/dλ)
    int terminated;  // A flag to stop tracing
    int sx, sy;      // Screen pixel coordinates
    int padding1, padding2; // Padding for memory alignment
} Ray;

// Standard math constants
#define PI_F 3.14159265358979323846f

// Get the metric tensor diagonal components
inline float4 get_metric_diagonal(float4 pos) {
    return (float4)(METRIC_G00, METRIC_G11, METRIC_G22, METRIC_G33);
}

// Standard color computation
float3 compute_color(Ray ray, float4 metric_diag) {
    float3 color = (float3)(0.0f, 0.0f, 0.0f);
    
    // Get normalized direction using standard normalize
    float3 dir = normalize(ray.vel.yzw);
    
    // Create a gradient based on ray direction and metric
    float metric_factor = (metric_diag.x + metric_diag.y + metric_diag.z + metric_diag.w) * 0.25f;
    
    // Base color from direction
    color.x = 0.5f + 0.5f * dir.x;
    color.y = 0.5f + 0.5f * dir.y;  
    color.z = 0.7f + 0.3f * dir.z;
    
    // Modulate by metric
    color *= (0.8f + 0.2f * metric_factor);
    
    // Add grid lines using standard functions
    float2 grid_coord = ray.vel.yz * 10.0f;
    float grid_lines = 0.0f;
    
    // Use standard floor function instead of fmod/fract
    float2 grid_floor = floor(grid_coord);
    float2 grid_frac = grid_coord - grid_floor;
    
    if (grid_frac.x > 0.95f || grid_frac.y > 0.95f) {
        grid_lines = 0.3f;
    }
    
    color += (float3)(grid_lines, grid_lines, grid_lines);
    
    // Add time-based animation using standard sin
    float time_factor = sin(ray.pos.x * 0.1f) * 0.1f + 1.0f;
    color *= time_factor;
    
    // Enhanced visualization for different metrics
    if (metric_diag.x < -0.5f) {
        float dot_product = dot(dir, dir);
        float gamma_factor = 1.0f / sqrt(max(0.1f, 1.0f - dot_product * 0.1f));
        color *= (1.0f + 0.1f * gamma_factor);
    }
    
    // Clamp to valid range
    color = clamp(color, 0.0f, 1.0f);
    
    return color;
}

// Standard ray integration
void integrate_ray_step(Ray* ray, float4 metric_diag, float step_size) {
    // Simple forward integration
    ray->pos += ray->vel * step_size;
    
    // Add curvature effects for non-Minkowski metrics
    if (metric_diag.x != -1.0f || metric_diag.y != 1.0f) {
        float3 pos_3d = ray->pos.yzw;
        float r = length(pos_3d);
        if (r > 0.1f) {
            float3 normalized_pos = pos_3d / r;
            float3 accel = -normalized_pos * (0.001f / (r * r));
            ray->vel.yzw += accel * step_size;
        }
    }
}

// Standard termination check
bool should_terminate_ray(Ray ray) {
    float distance = length(ray.pos.yzw);
    if (distance > 100.0f) {
        return true;
    }
    
    float schwarzschild_radius = 2.0f;
    if (distance < schwarzschild_radius) {
        return true;
    }
    
    float vel_magnitude = length(ray.vel);
    if (vel_magnitude < 0.001f) {
        return true;
    }
    
    return false;
}

// Main kernel using only standard OpenCL 3.0 features
__kernel void trace_rays(
    __global Ray* rays,
    __write_only image2d_t outputImage
) {
    int id = get_global_id(0);
    int total_pixels = get_global_size(0);
    
    if (id >= total_pixels) {
        return;
    }
    
    Ray ray = rays[id];
    float4 metric_diag = get_metric_diagonal(ray.pos);
    
    // Ray tracing loop with reasonable complexity
    const int max_steps = 12;
    const float step_size = 0.1f;
    
    for (int step = 0; step < max_steps && !ray.terminated; ++step) {
        if (should_terminate_ray(ray)) {
            ray.terminated = 1;
            break;
        }
        
        integrate_ray_step(&ray, metric_diag, step_size);
        metric_diag = get_metric_diagonal(ray.pos);
    }
    
    // Compute final color
    float3 color = compute_color(ray, metric_diag);
    
    // Standard gamma correction using pow
    color = pow(color, 1.0f / 2.2f);
    
    // Write result
    int2 coords = (int2)(ray.sx, ray.sy);
    write_imagef(outputImage, coords, (float4)(color, 1.0f));
}