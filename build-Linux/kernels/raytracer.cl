// Enhanced OpenCL kernel for Sirius ray tracing
// A struct to hold the properties of a single light ray
typedef struct {
    float4 pos;      // Current position (t, x, y, z)
    float4 vel;      // Current velocity (dt/dλ, dx/dλ, dy/dλ, dz/dλ)
    int terminated;  // A flag to stop tracing
    int sx, sy;      // Screen pixel coordinates
    int padding1, padding2; // Padding for alignment
} Ray;

// Get the metric tensor diagonal components at a given position
// The metric components are injected as compiler definitions from C++
float4 get_metric_diagonal(float4 pos) {
    // For now, we use constant metric values (Minkowski or other)
    // In the future, this could depend on position for curved spacetime
    return (float4)(METRIC_G00, METRIC_G11, METRIC_G22, METRIC_G33);
}

// Simple function to create interesting visual patterns
float3 compute_color(Ray ray, float4 metric_diag) {
    float3 color = (float3)(0.0f, 0.0f, 0.0f);
    
    // Get normalized direction
    float3 dir = normalize(ray.vel.yzw); // spatial components only
    
    // Create a gradient based on ray direction and metric
    float metric_factor = (metric_diag.x + metric_diag.y + metric_diag.z + metric_diag.w) * 0.25f;
    
    // Base color from direction
    color.x = 0.5f + 0.5f * dir.x; // Red channel
    color.y = 0.5f + 0.5f * dir.y; // Green channel  
    color.z = 0.7f + 0.3f * dir.z; // Blue channel (sky-like)
    
    // Modulate by metric to show spacetime curvature effects
    color *= (0.8f + 0.2f * metric_factor);
    
    // Add some grid lines for reference
    float2 grid_coord = ray.vel.yz * 10.0f;
    float grid_lines = 0.0f;
    
    if (fmod(grid_coord.x, 1.0f) > 0.95f || fmod(grid_coord.y, 1.0f) > 0.95f) {
        grid_lines = 0.3f;
    }
    
    // Add grid as white lines
    color += (float3)(grid_lines, grid_lines, grid_lines);
    
    // Add some time-based animation (using position as proxy for time)
    float time_factor = sin(ray.pos.x * 0.1f) * 0.1f + 1.0f;
    color *= time_factor;
    
    // Clamp color to valid range
    color = clamp(color, 0.0f, 1.0f);
    
    return color;
}

// Simple ray marching step (placeholder for geodesic integration)
void integrate_ray_step(Ray* ray, float4 metric_diag, float step_size) {
    // For Minkowski space, this is trivial (straight lines)
    // For curved spacetime, this would involve Christoffel symbols
    
    // Simple forward integration for now
    ray->pos += ray->vel * step_size;
    
    // In curved spacetime, velocity would also change:
    // ray->vel += acceleration * step_size;
    // where acceleration comes from the geodesic equation
}

// Check if ray should terminate (hit boundary, black hole, etc.)
bool should_terminate_ray(Ray ray) {
    // Terminate if ray goes too far
    float distance = length(ray.pos.yzw);
    if (distance > 100.0f) {
        return true;
    }
    
    // For black holes, check if inside event horizon
    // (this would be metric-specific)
    
    return false;
}

// The main kernel that runs for every pixel on the screen
__kernel void trace_rays(
    __global Ray* rays,           // Input array of rays, one for each pixel
    __write_only image2d_t outputImage // The output image texture
) {
    int id = get_global_id(0);
    
    // Bounds check
    int total_pixels = get_global_size(0);
    if (id >= total_pixels) {
        return;
    }
    
    Ray ray = rays[id];
    
    // Get metric at current position
    float4 metric_diag = get_metric_diagonal(ray.pos);
    
    // Perform ray tracing (simplified for now)
    const int max_steps = 10;
    const float step_size = 0.1f;
    
    for (int step = 0; step < max_steps && !ray.terminated; ++step) {
        // Check termination conditions
        if (should_terminate_ray(ray)) {
            ray.terminated = 1;
            break;
        }
        
        // Integrate ray forward one step
        integrate_ray_step(&ray, metric_diag, step_size);
        
        // Update metric for new position
        metric_diag = get_metric_diagonal(ray.pos);
    }
    
    // Compute final color based on ray state
    float3 color = compute_color(ray, metric_diag);
    
    // Write the final color to the output image
    int2 coords = (int2)(ray.sx, ray.sy);
    write_imagef(outputImage, coords, (float4)(color, 1.0f));
}